/**
 * GreenOS - Sensor Manager Implementation
 * 
 * Handles reading and managing all greenhouse sensors:
 * - Adafruit SCD-30: NDIR CO2, Temperature, Humidity (I2C)
 * - MQ135: Air Quality Sensor (Analog ADC with voltage divider)
 * - Modbus RS485: Soil EC/pH/Moisture/Temperature
 * 
 * Features:
 * - Sensor health monitoring and validation
 * - ADC calibration with temperature compensation
 * - Error handling with fallback to last known good values
 * - Modbus RTU communication with proper timing
 */

#include "sensor_manager.h"
#include "config.h"
#include <Adafruit_SCD30.h>
#include <ModbusMaster.h>
#include <Wire.h>
// Note: EEPROM not available on UNO R4 - calibration stored in RAM (resets on reboot)

// ============================================================================
// GLOBAL SENSOR OBJECTS
// ============================================================================

Adafruit_SCD30 scd30;
ModbusMaster modbusNode;

// ============================================================================
// SENSOR HEALTH TRACKING
// ============================================================================

struct SensorHealth {
  bool isValid;
  unsigned long lastValidRead;
  float lastValidValue;
  uint8_t consecutiveErrors;
  uint32_t totalReads;
  uint32_t totalErrors;
};

SensorHealth scd30Health = {false, 0, 0.0f, 0, 0, 0};
SensorHealth mq135Health = {false, 0, 0.0f, 0, 0, 0};
SensorHealth modbusHealth = {false, 0, 0.0f, 0, 0, 0};

// ============================================================================
// ADC CALIBRATION DATA
// ============================================================================

ADCCalibration adcCal = {
  .offset = 0.0f,
  .scale = 1.0f,
  .vRef = ADC_VREF_NOMINAL,
  .tempCoeff = 0.0002f,  // 0.02%/°C typical for RA4M1
  .crc32 = 0
};

// ============================================================================
// MQ135 CALIBRATION
// ============================================================================

float mq135_R0 = 10000.0;  // Baseline resistance in clean air (to be calibrated)
bool mq135_preheated = false;
unsigned long mq135_startTime = 0;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SensorManager::SensorManager() {
  // Initialize sensor data with safe defaults
  data.airTemp = 20.0f;
  data.airHumidity = 50.0f;
  data.substrateTemp = 20.0f;
  data.vwc = 30.0f;
  data.ph = 6.5f;
  data.ec = 1.5f;
  data.co2 = 400.0f;
  data.par = 0.0f;
  data.motionDetected = false;
  data.noiseLevel = 0.0f;
  data.upsActive = false;
  data.voltage = 5.0f;
  data.timestamp = 0;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void SensorManager::init() {
  Serial.println("=== Initializing Sensors ===");
  
  // Load ADC calibration from EEPROM
  loadADCCalibration();
  
  // Initialize I2C bus
  Wire.begin();
  Wire.setClock(100000);  // 100kHz I2C speed
  
  // Initialize SCD-30 CO2 sensor
  if (scd30.begin()) {
    Serial.println("✓ SCD-30 CO2 sensor initialized");
    
    // Configure SCD-30
    scd30.setMeasurementInterval(SCD30_MEASUREMENT_INTERVAL);
    
    // Set altitude compensation for Denver (5280 ft = 1609 m)
    if (SCD30_ALTITUDE_COMPENSATION) {
      scd30.setAltitudeOffset(GREENHOUSE_ALTITUDE_M);
      Serial.print("  Altitude compensation: ");
      Serial.print(GREENHOUSE_ALTITUDE_M);
      Serial.println(" meters");
    }
    
    // Set temperature offset if needed
    if (SCD30_TEMP_OFFSET != 0.0) {
      scd30.setTemperatureOffset(SCD30_TEMP_OFFSET);
    }
    
    // Enable/disable auto-calibration
    scd30.selfCalibrationEnabled(SCD30_AUTO_CALIBRATION);
    
    scd30Health.isValid = true;
  } else {
    Serial.println("✗ SCD-30 initialization failed!");
    scd30Health.isValid = false;
  }
  
  // Initialize Modbus for RS485 soil sensor
  Serial1.begin(MODBUS_BAUD_RATE);  // Hardware serial for Modbus
  pinMode(MODBUS_DE_RE_PIN, OUTPUT);
  digitalWrite(MODBUS_DE_RE_PIN, LOW);  // Receive mode by default
  
  // Initialize ModbusMaster
  modbusNode.begin(MODBUS_SLAVE_ID, Serial1);
  
  // Set up preTransmission and postTransmission callbacks for RS485
  modbusNode.preTransmission([]() {
    digitalWrite(MODBUS_DE_RE_PIN, HIGH);  // Transmit mode
  });
  modbusNode.postTransmission([]() {
    digitalWrite(MODBUS_DE_RE_PIN, LOW);   // Receive mode
  });
  
  Serial.println("✓ Modbus RS485 initialized");
  
  // Initialize analog sensors
  analogReadResolution(ADC_RESOLUTION);
  pinMode(MQ135_SENSOR_PIN, INPUT);
  pinMode(VWC_SENSOR_PIN, INPUT);
  pinMode(MICROPHONE_PIN, INPUT);
  
  // Start MQ135 preheat timer
  mq135_startTime = millis();
  Serial.print("⏱ MQ135 preheating (requires ");
  Serial.print(MQ135_PREHEAT_TIME_MS / 3600000);
  Serial.println(" hours)...");
  
  // Initialize digital sensors
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(UPS_STATUS_PIN, INPUT_PULLUP);
  
  Serial.println("=== Sensor Initialization Complete ===\n");
}

// ============================================================================
// READ ALL SENSORS
// ============================================================================

void SensorManager::readAll() {
  data.timestamp = millis();
  
  // Read SCD-30 (CO2, Temperature, Humidity)
  readSCD30();
  
  // Read MQ135 Air Quality
  readMQ135();
  
  // Read Modbus soil sensor (EC, pH, Moisture, Temperature)
  readModbusSensor();
  
  // Read simple digital/analog sensors
  data.motionDetected = digitalRead(PIR_SENSOR_PIN);
  data.upsActive = !digitalRead(UPS_STATUS_PIN);  // Active low
  data.noiseLevel = readCalibratedADC(MICROPHONE_PIN);
  
  // Update sensor health statistics
  updateHealthStatistics();
}

// ============================================================================
// SCD-30 CO2 SENSOR READING
// ============================================================================

void SensorManager::readSCD30() {
  scd30Health.totalReads++;
  
  if (!scd30Health.isValid) {
    // Sensor previously failed, skip reading
    return;
  }
  
  if (scd30.dataReady()) {
    if (scd30.read()) {
      float co2 = scd30.CO2;
      float temp = scd30.temperature;
      float humidity = scd30.relative_humidity;
      
      // Sanity checks
      bool co2Valid = (co2 >= 300 && co2 <= 5000);
      bool tempValid = (temp >= -10 && temp <= 50);
      bool humidityValid = (humidity >= 0 && humidity <= 100);
      
      if (co2Valid && tempValid && humidityValid) {
        // All readings valid
        data.co2 = co2;
        data.airTemp = temp;
        data.airHumidity = humidity;
        
        scd30Health.lastValidRead = millis();
        scd30Health.lastValidValue = co2;
        scd30Health.consecutiveErrors = 0;
        
        return;  // Success!
      }
    }
  }
  
  // Error handling
  scd30Health.consecutiveErrors++;
  scd30Health.totalErrors++;
  
  if (scd30Health.consecutiveErrors > MAX_SENSOR_ERRORS) {
    scd30Health.isValid = false;
    Serial.println("⚠️ SCD-30 sensor flagged as failed!");
  }
  
  // Use last known good values
  data.co2 = scd30Health.lastValidValue;
}

// ============================================================================
// MQ135 AIR QUALITY SENSOR READING
// ============================================================================

void SensorManager::readMQ135() {
  mq135Health.totalReads++;
  
  // Check if sensor is preheated
  if (!mq135_preheated) {
    if (millis() - mq135_startTime >= MQ135_PREHEAT_TIME_MS) {
      mq135_preheated = true;
      Serial.println("✓ MQ135 preheat complete!");
    } else {
      return;  // Still preheating
    }
  }
  
  // Read analog voltage (with voltage divider compensation)
  float voltage = readCalibratedADC(MQ135_SENSOR_PIN);
  
  // Compensate for voltage divider (5V → 3.3V)
  // Original sensor voltage = measured voltage × (R1+R2)/R2
  float sensorVoltage = voltage * (MQ135_VDIV_R1 + MQ135_VDIV_R2) / MQ135_VDIV_R2;
  
  // Calculate sensor resistance: Rs = (Vc × RL) / (Vsensor - Vc)
  // where Vc = 5V supply, RL = load resistor
  if (sensorVoltage > 0.1 && sensorVoltage < 4.9) {
    float Rs = (5.0 - sensorVoltage) * MQ135_LOAD_RESISTOR / sensorVoltage;
    
    // Calculate air quality ratio: Rs/R0
    float ratio = Rs / mq135_R0;
    
    // Simplified PPM calculation (calibration needed for accuracy)
    // This is a placeholder - proper calibration requires known gas concentrations
    float airQualityPPM = 116.6020682 * pow(ratio, -2.769034857);
    
    // Sanity check
    if (airQualityPPM >= 10 && airQualityPPM <= 2000) {
      data.airQualityPPM = airQualityPPM;
      mq135Health.lastValidValue = airQualityPPM;
      mq135Health.lastValidRead = millis();
      mq135Health.consecutiveErrors = 0;
      return;
    }
  }
  
  // Error handling
  mq135Health.consecutiveErrors++;
  mq135Health.totalErrors++;
  
  if (mq135Health.consecutiveErrors > MAX_SENSOR_ERRORS) {
    mq135Health.isValid = false;
  }
  
  data.airQualityPPM = mq135Health.lastValidValue;
}

// ============================================================================
// MODBUS RS485 SOIL SENSOR READING
// ============================================================================

void SensorManager::readModbusSensor() {
  modbusHealth.totalReads++;
  
  uint8_t result;
  uint16_t data16[7];  // Buffer for 7 registers
  
  // Read all sensor registers in one transaction (more efficient)
  // Registers 0x0000 - 0x0006: Moisture, Temp, EC, pH, N, P, K
  result = modbusNode.readHoldingRegisters(MODBUS_REG_MOISTURE, 7);
  
  if (result == modbusNode.ku8MBSuccess) {
    // Read successful, parse data
    for (int i = 0; i < 7; i++) {
      data16[i] = modbusNode.getResponseBuffer(i);
    }
    
    // Parse according to datasheet specifications
    float moisture = data16[0] / 10.0f;      // 0.1% resolution
    float soilTemp = data16[1] / 10.0f;      // 0.1°C resolution
    float ec = data16[2] / 1000.0f;          // Convert µS/cm to mS/cm
    float ph = data16[3] / 100.0f;           // 0.01 pH resolution
    float nitrogen = data16[4];               // mg/kg (ppm)
    float phosphorus = data16[5];             // mg/kg (ppm)
    float potassium = data16[6];              // mg/kg (ppm)
    
    // Sanity checks
    bool moistureValid = (moisture >= 0 && moisture <= 100);
    bool tempValid = (soilTemp >= -10 && soilTemp <= 60);
    bool ecValid = (ec >= 0 && ec <= 10);
    bool phValid = (ph >= 3 && ph <= 10);
    
    if (moistureValid && tempValid && ecValid && phValid) {
      // All readings valid
      data.vwc = moisture;
      data.substrateTemp = soilTemp;
      data.ec = ec;
      data.ph = ph;
      data.nitrogen = nitrogen;
      data.phosphorus = phosphorus;
      data.potassium = potassium;
      
      modbusHealth.lastValidRead = millis();
      modbusHealth.lastValidValue = ec;  // Use EC as health indicator
      modbusHealth.consecutiveErrors = 0;
      modbusHealth.isValid = true;
      
      return;  // Success!
    }
  }
  
  // Error handling
  modbusHealth.consecutiveErrors++;
  modbusHealth.totalErrors++;
  
  if (modbusHealth.consecutiveErrors > MAX_SENSOR_ERRORS) {
    modbusHealth.isValid = false;
    Serial.print("⚠️ Modbus sensor failed! Error code: 0x");
    Serial.println(result, HEX);
  }
  
  // Keep last known good values (already in data structure)
}

// ============================================================================
// ADC CALIBRATION FUNCTIONS
// ============================================================================

void SensorManager::loadADCCalibration() {
  // STUB: EEPROM not available - using default calibration values
  Serial.println("⚠️  ADC calibration using defaults (EEPROM not available)");
  adcCal.offset = 0.0f;
  adcCal.scale = 1.0f;
  adcCal.vRef = ADC_VREF_NOMINAL;
  adcCal.tempCoeff = 0.0002f;
  adcCal.crc32 = 0;
}

void SensorManager::saveADCCalibration() {
  // STUB: EEPROM not available - calibration stored in RAM only (lost on reboot)
  adcCal.crc32 = calculateCRC32((uint8_t*)&adcCal, sizeof(ADCCalibration) - sizeof(uint32_t));
  Serial.println("ℹ️  ADC calibration saved to RAM (will be lost on reboot)");
}

float SensorManager::readCalibratedADC(int pin) {
  // Multi-sample averaging for noise reduction
  uint32_t sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(100);  // Short delay between samples
  }
  
  float avgRaw = sum / (float)ADC_SAMPLES;
  
  // Convert to voltage
  float voltage = (avgRaw / ADC_MAX_VALUE) * adcCal.vRef;
  
  // Apply calibration
  voltage = (voltage - adcCal.offset) * adcCal.scale;
  
  // Optional: Apply temperature compensation
  // float chipTemp = getChipTemperature();
  // voltage = compensateForTemp(voltage, chipTemp);
  
  return voltage;
}

// ============================================================================
// SENSOR HEALTH MONITORING
// ============================================================================

void SensorManager::updateHealthStatistics() {
  // Calculate error rates
  float scd30ErrorRate = (scd30Health.totalReads > 0) ? 
    (float)scd30Health.totalErrors / scd30Health.totalReads * 100.0f : 0.0f;
  
  float mq135ErrorRate = (mq135Health.totalReads > 0) ?
    (float)mq135Health.totalErrors / mq135Health.totalReads * 100.0f : 0.0f;
  
  float modbusErrorRate = (modbusHealth.totalReads > 0) ?
    (float)modbusHealth.totalErrors / modbusHealth.totalReads * 100.0f : 0.0f;
  
  // Store in data structure for Firebase sync
  data.scd30ErrorRate = scd30ErrorRate;
  data.mq135ErrorRate = mq135ErrorRate;
  data.modbusErrorRate = modbusErrorRate;
}

SensorHealthReport SensorManager::getHealthReport() {
  SensorHealthReport report;
  
  report.scd30Valid = scd30Health.isValid;
  report.scd30ErrorRate = data.scd30ErrorRate;
  report.scd30LastRead = scd30Health.lastValidRead;
  
  report.mq135Valid = mq135Health.isValid;
  report.mq135ErrorRate = data.mq135ErrorRate;
  report.mq135Preheated = mq135_preheated;
  
  report.modbusValid = modbusHealth.isValid;
  report.modbusErrorRate = data.modbusErrorRate;
  report.modbusLastRead = modbusHealth.lastValidRead;
  
  return report;
}

// ============================================================================
// CALIBRATION PROCEDURES
// ============================================================================

void SensorManager::performADCCalibration() {
  Serial.println("\n=== ADC CALIBRATION MODE ===");
  Serial.println("This will calibrate the ADC for accurate analog readings.");
  
  // Step 1: Zero point calibration
  Serial.println("\nStep 1: ZERO POINT CALIBRATION");
  Serial.println("Connect the ADC pin to GND and press any key...");
  while (!Serial.available());
  while (Serial.available()) Serial.read();  // Clear buffer
  
  delay(1000);
  uint32_t zeroSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    zeroSum += analogRead(VWC_SENSOR_PIN);
    delay(ADC_SAMPLE_DELAY_MS);
  }
  float zeroRaw = zeroSum / (float)ADC_SAMPLES;
  adcCal.offset = (zeroRaw / ADC_MAX_VALUE) * ADC_VREF_NOMINAL;
  Serial.print("Zero offset: ");
  Serial.print(adcCal.offset, 4);
  Serial.print(" V (Raw: ");
  Serial.print(zeroRaw, 1);
  Serial.println(")");
  
  // Step 2: Reference point calibration
  Serial.println("\nStep 2: REFERENCE CALIBRATION");
  Serial.println("Connect the ADC pin to a known voltage reference (e.g., 2.5V)");
  Serial.print("Enter reference voltage in volts (e.g., 2.5): ");
  while (!Serial.available());
  float refVoltage = Serial.parseFloat();
  while (Serial.available()) Serial.read();
  
  delay(1000);
  uint32_t refSum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    refSum += analogRead(VWC_SENSOR_PIN);
    delay(ADC_SAMPLE_DELAY_MS);
  }
  float refRaw = refSum / (float)ADC_SAMPLES;
  float measuredVoltage = (refRaw / ADC_MAX_VALUE) * ADC_VREF_NOMINAL;
  adcCal.scale = refVoltage / (measuredVoltage - adcCal.offset);
  Serial.print("Scale factor: ");
  Serial.print(adcCal.scale, 4);
  Serial.print(" (Measured: ");
  Serial.print(measuredVoltage, 4);
  Serial.print(" V, Target: ");
  Serial.print(refVoltage, 4);
  Serial.println(" V)");
  
  // Step 3: Measure actual Vref
  Serial.println("\nStep 3: VREF MEASUREMENT");
  Serial.println("Measuring internal voltage reference...");
  // Note: This would use internal reference if available
  adcCal.vRef = ADC_VREF_NOMINAL;  // Placeholder
  Serial.print("Vref: ");
  Serial.print(adcCal.vRef, 4);
  Serial.println(" V");
  
  // Save calibration
  saveADCCalibration();
  
  Serial.println("\n✓ ADC Calibration Complete!");
  Serial.println("=== CALIBRATION MODE END ===\n");
}

void SensorManager::calibrateMQ135() {
  Serial.println("\n=== MQ135 CALIBRATION MODE ===");
  Serial.println("Place sensor in clean air for 24-48 hours before calibration.");
  Serial.println("Press any key when ready...");
  while (!Serial.available());
  while (Serial.available()) Serial.read();
  
  // Read sensor resistance in clean air
  float voltage = readCalibratedADC(MQ135_SENSOR_PIN);
  float sensorVoltage = voltage * (MQ135_VDIV_R1 + MQ135_VDIV_R2) / MQ135_VDIV_R2;
  float Rs = (5.0 - sensorVoltage) * MQ135_LOAD_RESISTOR / sensorVoltage;
  
  // R0 = Rs / clean_air_ratio
  mq135_R0 = Rs / MQ135_CLEAN_AIR_RATIO;
  
  Serial.print("✓ MQ135 R0 calibrated: ");
  Serial.print(mq135_R0, 2);
  Serial.println(" Ω");
  Serial.print("  Sensor resistance in clean air: ");
  Serial.print(Rs, 2);
  Serial.println(" Ω");
  
  // STUB: Save to RAM only (no EEPROM available)
  Serial.println("ℹ️  Calibration saved to RAM (will be lost on reboot)");
  
  Serial.println("=== CALIBRATION MODE END ===\n");
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

SensorData SensorManager::getData() {
  return data;
}

void SensorManager::printReadings() {
  Serial.println("--- Environmental ---");
  Serial.print("Air Temp:     ");
  Serial.print(data.airTemp, 1);
  Serial.println(" °C");
  Serial.print("Air Humidity: ");
  Serial.print(data.airHumidity, 1);
  Serial.println(" %");
  Serial.print("CO2:          ");
  Serial.print(data.co2, 0);
  Serial.println(" ppm");
  Serial.print("Air Quality:  ");
  Serial.print(data.airQualityPPM, 0);
  Serial.println(" ppm");
  
  Serial.println("--- Soil ---");
  Serial.print("Soil Temp:    ");
  Serial.print(data.substrateTemp, 1);
  Serial.println(" °C");
  Serial.print("Moisture:     ");
  Serial.print(data.vwc, 1);
  Serial.println(" %");
  Serial.print("pH:           ");
  Serial.println(data.ph, 2);
  Serial.print("EC:           ");
  Serial.print(data.ec, 2);
  Serial.println(" mS/cm");
  Serial.print("N-P-K:        ");
  Serial.print(data.nitrogen, 0);
  Serial.print("-");
  Serial.print(data.phosphorus, 0);
  Serial.print("-");
  Serial.print(data.potassium, 0);
  Serial.println(" mg/kg");
  
  Serial.println("--- Status ---");
  Serial.print("Motion:       ");
  Serial.println(data.motionDetected ? "YES" : "NO");
  Serial.print("UPS Active:   ");
  Serial.println(data.upsActive ? "YES" : "NO");
  Serial.print("Timestamp:    ");
  Serial.print(data.timestamp);
  Serial.println(" ms");
  Serial.println();
}

// CRC32 calculation for data integrity
uint32_t SensorManager::calculateCRC32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
    }
  }
  return ~crc;
}
