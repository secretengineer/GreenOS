/**
 * GreenOS - Sensor Manager Implementation
 * 
 * ESP32-WROOM-32E Sensor Management
 */

#include "sensor_manager.h"
#include <Preferences.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

// Global reference for Modbus callbacks
SensorManager* g_sensorManager = nullptr;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SensorManager::SensorManager() 
  : _modbusSerial(2),  // Use UART2
    _initialized(false),
    _scd30Ready(false),
    _modbusReady(false),
    _mq135StartTime(0),
    _scd30ReadCount(0),
    _scd30ErrorCount(0),
    _modbusReadCount(0),
    _modbusErrorCount(0),
    _scd30LastRead(0),
    _modbusLastRead(0),
    _mq135R0(10.0)  // Default R0
{
  g_sensorManager = this;
  memset(&_data, 0, sizeof(SensorData));
  memset(&_lastGoodData, 0, sizeof(SensorData));
  memset(&_adcCal, 0, sizeof(ADCCalibration));
  _adcCal.scale = 1.0;
  _adcCal.vRef = ADC_VREF_NOMINAL;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void SensorManager::init() {
  Serial.println("[SENSOR] Initializing sensor manager...");
  
  // Configure ADC
  configureADC();
  
  // Load saved calibration
  loadCalibration();
  
  // Initialize I2C bus
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_FREQUENCY);
  Serial.printf("[SENSOR] I2C initialized on SDA=%d, SCL=%d\n", I2C_SDA_PIN, I2C_SCL_PIN);
  
  // Initialize SCD-30 CO2 sensor
  Serial.println("[SENSOR] Initializing SCD-30 CO2 sensor...");
  if (_scd30.begin(Wire)) {
    _scd30Ready = true;
    
    // Configure SCD-30
    _scd30.setMeasurementInterval(SCD30_MEASUREMENT_INTERVAL);
    _scd30.setAutoSelfCalibration(SCD30_AUTO_CALIBRATION);
    
    if (SCD30_ALTITUDE_COMPENSATION) {
      _scd30.setAltitudeCompensation(GREENHOUSE_ALTITUDE_M);
    }
    
    if (SCD30_TEMP_OFFSET != 0.0) {
      _scd30.setTemperatureOffset(SCD30_TEMP_OFFSET);
    }
    
    Serial.println("[OK] SCD-30 initialized");
    Serial.printf("     Firmware version: 0x%04X\n", _scd30.readFirmwareVersion());
  } else {
    Serial.println("[WARN] SCD-30 not found!");
    _scd30Ready = false;
  }
  
  // Initialize Modbus for soil sensor
  Serial.println("[SENSOR] Initializing Modbus soil sensor...");
  _modbusSerial.begin(MODBUS_BAUD_RATE, SERIAL_8N1, MODBUS_RX_PIN, MODBUS_TX_PIN);
  
  // Configure DE/RE pin for MAX485
  pinMode(MODBUS_DE_RE_PIN, OUTPUT);
  digitalWrite(MODBUS_DE_RE_PIN, LOW);  // Receive mode by default
  
  // Initialize ModbusMaster
  _modbus.begin(MODBUS_SLAVE_ID, _modbusSerial);
  _modbus.preTransmission(preTransmission);
  _modbus.postTransmission(postTransmission);
  
  // Test Modbus communication
  uint8_t result = _modbus.readHoldingRegisters(MODBUS_REG_MOISTURE, 1);
  if (result == _modbus.ku8MBSuccess) {
    _modbusReady = true;
    Serial.println("[OK] Modbus soil sensor detected");
  } else {
    Serial.printf("[WARN] Modbus communication failed (error: %d)\n", result);
    _modbusReady = false;
  }
  
  // Configure analog input pins
  Serial.println("[SENSOR] Configuring analog inputs...");
  pinMode(MQ135_SENSOR_PIN, INPUT);
  pinMode(VWC_SENSOR_PIN, INPUT);
  pinMode(MICROPHONE_PIN, INPUT);
  pinMode(LIGHT_LEVEL_PIN, INPUT);
  
  // Configure digital input pins
  pinMode(PIR_SENSOR_PIN, INPUT);
  pinMode(UPS_STATUS_PIN, INPUT_PULLUP);
  
  // Record MQ135 start time for preheat tracking
  _mq135StartTime = millis();
  
  _initialized = true;
  Serial.println("[OK] Sensor manager initialized\n");
}

// ============================================================================
// ADC CONFIGURATION
// ============================================================================

void SensorManager::configureADC() {
  // Configure ADC resolution
  analogReadResolution(ADC_RESOLUTION);
  
  // Configure ADC attenuation for full 0-3.3V range
  analogSetAttenuation(ADC_ATTENUATION);
  
  // Configure specific pins
  analogSetPinAttenuation(MQ135_SENSOR_PIN, ADC_ATTENUATION);
  analogSetPinAttenuation(VWC_SENSOR_PIN, ADC_ATTENUATION);
  analogSetPinAttenuation(MICROPHONE_PIN, ADC_ATTENUATION);
  analogSetPinAttenuation(LIGHT_LEVEL_PIN, ADC_ATTENUATION);
  
  Serial.println("[OK] ADC configured for 12-bit, 0-3.3V range");
}

float SensorManager::readCalibratedADC(int pin, int samples) {
  uint32_t sum = 0;
  
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delayMicroseconds(ADC_SAMPLE_DELAY_US);
  }
  
  float average = (float)sum / samples;
  float voltage = (average / ADC_MAX_VALUE) * _adcCal.vRef;
  
  // Apply calibration
  voltage = (voltage - _adcCal.offset) * _adcCal.scale;
  
  return voltage;
}

// ============================================================================
// SENSOR READING
// ============================================================================

void SensorManager::readAll() {
  _data.timestamp = millis();
  
  // Get Unix time if available
  time_t now;
  time(&now);
  if (now > 1609459200) {  // After Jan 1, 2021 = probably synced
    _data.unixTime = now;
    _data.ntpSynced = true;
  } else {
    _data.ntpSynced = false;
  }
  
  // Read all sensors
  readSCD30();
  readMQ135();
  readModbusSensor();
  readDigitalSensors();
  readAnalogSensors();
  
  // Apply altitude compensation
  applyAltitudeCompensation();
  
  // Update health statistics
  updateHealthStatistics();
}

void SensorManager::readSCD30() {
  if (!_scd30Ready) {
    return;
  }
  
  _scd30ReadCount++;
  
  if (_scd30.dataAvailable()) {
    float co2 = _scd30.getCO2();
    float temp = _scd30.getTemperature();
    float humidity = _scd30.getHumidity();
    
    // Validate readings
    if (validateReading(co2, 0, 10000) &&
        validateReading(temp, -40, 85) &&
        validateReading(humidity, 0, 100)) {
      
      _data.co2 = co2;
      _data.airTemp = temp;
      _data.airHumidity = humidity;
      _scd30LastRead = millis();
      
      // Store as last good data
      _lastGoodData.co2 = co2;
      _lastGoodData.airTemp = temp;
      _lastGoodData.airHumidity = humidity;
    } else {
      _scd30ErrorCount++;
      // Use last known good values
      _data.co2 = _lastGoodData.co2;
      _data.airTemp = _lastGoodData.airTemp;
      _data.airHumidity = _lastGoodData.airHumidity;
    }
  } else {
    _scd30ErrorCount++;
  }
  
  // Calculate error rate
  if (_scd30ReadCount > 0) {
    _data.scd30ErrorRate = ((float)_scd30ErrorCount / _scd30ReadCount) * 100.0;
  }
}

void SensorManager::readMQ135() {
  // Read raw voltage
  float voltage = readCalibratedADC(MQ135_SENSOR_PIN);
  
  // Calculate sensor resistance
  // RS = (VCC * RL / Vout) - RL
  float rs = ((ADC_VREF_NOMINAL * MQ135_LOAD_RESISTOR) / voltage) - MQ135_LOAD_RESISTOR;
  
  // Calculate ratio
  float ratio = rs / _mq135R0;
  
  // Convert to PPM using calibration curve
  // This is an approximation - actual values depend on target gas
  // Using CO2 approximation: ppm = 116.6020682 * ratio^(-2.769034857)
  if (ratio > 0) {
    _data.airQualityPPM = 116.6020682 * pow(ratio, -2.769034857);
  } else {
    _data.airQualityPPM = 0;
  }
  
  // Clamp to reasonable range
  if (_data.airQualityPPM > 10000) _data.airQualityPPM = 10000;
  if (_data.airQualityPPM < 0) _data.airQualityPPM = 0;
}

void SensorManager::readModbusSensor() {
  if (!_modbusReady) {
    return;
  }
  
  _modbusReadCount++;
  
  // Read all registers in one transaction (7 registers starting at 0x0000)
  uint8_t result = _modbus.readHoldingRegisters(MODBUS_REG_MOISTURE, 7);
  
  if (result == _modbus.ku8MBSuccess) {
    // Parse register values
    _data.vwc = _modbus.getResponseBuffer(0) * 0.1;           // 0.1% resolution
    _data.substrateTemp = _modbus.getResponseBuffer(1) * 0.1; // 0.1°C resolution
    _data.ec = _modbus.getResponseBuffer(2) * 0.001;          // Convert µS/cm to mS/cm
    _data.ph = _modbus.getResponseBuffer(3) * 0.01;           // 0.01 pH resolution
    _data.nitrogen = _modbus.getResponseBuffer(4);            // mg/kg
    _data.phosphorus = _modbus.getResponseBuffer(5);          // mg/kg
    _data.potassium = _modbus.getResponseBuffer(6);           // mg/kg
    
    _modbusLastRead = millis();
    
    // Store last good values
    _lastGoodData.vwc = _data.vwc;
    _lastGoodData.substrateTemp = _data.substrateTemp;
    _lastGoodData.ec = _data.ec;
    _lastGoodData.ph = _data.ph;
    _lastGoodData.nitrogen = _data.nitrogen;
    _lastGoodData.phosphorus = _data.phosphorus;
    _lastGoodData.potassium = _data.potassium;
  } else {
    _modbusErrorCount++;
    Serial.printf("[WARN] Modbus read error: %d\n", result);
    
    // Use last known good values
    _data.vwc = _lastGoodData.vwc;
    _data.substrateTemp = _lastGoodData.substrateTemp;
    _data.ec = _lastGoodData.ec;
    _data.ph = _lastGoodData.ph;
    _data.nitrogen = _lastGoodData.nitrogen;
    _data.phosphorus = _lastGoodData.phosphorus;
    _data.potassium = _lastGoodData.potassium;
  }
  
  // Calculate error rate
  if (_modbusReadCount > 0) {
    _data.modbusErrorRate = ((float)_modbusErrorCount / _modbusReadCount) * 100.0;
  }
}

void SensorManager::readDigitalSensors() {
  // PIR motion sensor
  _data.motionDetected = digitalRead(PIR_SENSOR_PIN) == HIGH;
  
  // UPS status (typically active low when on battery)
  _data.upsActive = digitalRead(UPS_STATUS_PIN) == LOW;
}

void SensorManager::readAnalogSensors() {
  // Microphone / noise level
  float micVoltage = readCalibratedADC(MICROPHONE_PIN);
  _data.noiseLevel = micVoltage;
  
  // Light level
  float lightRaw = analogRead(LIGHT_LEVEL_PIN);
  _data.lightLevel = lightRaw;
  
  // Convert to PAR if calibrated (placeholder calculation)
  // Actual conversion depends on sensor used
  _data.par = (lightRaw / ADC_MAX_VALUE) * 2000.0;  // Rough estimate, 0-2000 µmol/m²/s
}

// ============================================================================
// MODBUS CALLBACKS
// ============================================================================

void SensorManager::preTransmission() {
  digitalWrite(MODBUS_DE_RE_PIN, HIGH);  // Enable transmitter
  delayMicroseconds(50);
}

void SensorManager::postTransmission() {
  delayMicroseconds(50);
  digitalWrite(MODBUS_DE_RE_PIN, LOW);   // Enable receiver
}

// ============================================================================
// VALIDATION & COMPENSATION
// ============================================================================

bool SensorManager::validateReading(float value, float min, float max) {
  return !isnan(value) && !isinf(value) && value >= min && value <= max;
}

void SensorManager::applyAltitudeCompensation() {
  // Denver altitude compensation for various readings
  // Air pressure at 5280ft is approximately 83% of sea level
  // This affects CO2 sensor readings and other pressure-dependent measurements
  
  // SCD-30 handles altitude compensation internally when configured
  // Additional compensation can be applied here if needed
}

// ============================================================================
// HEALTH REPORTING
// ============================================================================

SensorHealthReport SensorManager::getHealthReport() {
  SensorHealthReport report;
  
  report.scd30Present = _scd30Ready;
  report.scd30Valid = _scd30Ready && (millis() - _scd30LastRead < 30000);
  report.scd30ErrorRate = _data.scd30ErrorRate;
  report.scd30LastRead = _scd30LastRead;
  report.scd30ReadCount = _scd30ReadCount;
  report.scd30ErrorCount = _scd30ErrorCount;
  
  report.mq135Valid = true;  // Analog sensor always "works"
  report.mq135Preheated = isMQ135Preheated();
  report.mq135StartTime = _mq135StartTime;
  report.mq135ErrorRate = 0;  // Analog doesn't have errors
  
  report.modbusPresent = _modbusReady;
  report.modbusValid = _modbusReady && (millis() - _modbusLastRead < 60000);
  report.modbusErrorRate = _data.modbusErrorRate;
  report.modbusLastRead = _modbusLastRead;
  report.modbusReadCount = _modbusReadCount;
  report.modbusErrorCount = _modbusErrorCount;
  
  report.freeHeap = ESP.getFreeHeap();
  report.minFreeHeap = ESP.getMinFreeHeap();
  report.systemHealthy = (report.freeHeap > MIN_FREE_HEAP_BYTES) &&
                          (report.scd30Valid || report.modbusValid);
  
  return report;
}

void SensorManager::updateHealthStatistics() {
  // Reset counters periodically to prevent overflow and keep stats current
  if (_scd30ReadCount > 10000) {
    _scd30ReadCount = 1000;
    _scd30ErrorCount = (_scd30ErrorCount * 1000) / 10000;
  }
  
  if (_modbusReadCount > 10000) {
    _modbusReadCount = 1000;
    _modbusErrorCount = (_modbusErrorCount * 1000) / 10000;
  }
}

bool SensorManager::isMQ135Preheated() const {
  return (millis() - _mq135StartTime) >= MQ135_PREHEAT_TIME_MS;
}

// ============================================================================
// CALIBRATION
// ============================================================================

void SensorManager::performADCCalibration() {
  Serial.println("[CAL] Performing ADC calibration...");
  
  // Use ESP32's internal calibration
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  
  _adcCal.vRef = adc_chars.vref / 1000.0;  // Convert mV to V
  _adcCal.valid = true;
  
  saveCalibration();
  Serial.printf("[CAL] ADC calibration complete. Vref = %.3f V\n", _adcCal.vRef);
}

void SensorManager::calibrateMQ135() {
  if (!isMQ135Preheated()) {
    Serial.println("[CAL] MQ135 not preheated! Wait 48 hours before calibrating.");
    return;
  }
  
  Serial.println("[CAL] Calibrating MQ135 in clean air...");
  
  // Take average reading in clean air
  float voltage = readCalibratedADC(MQ135_SENSOR_PIN, 100);
  float rs = ((ADC_VREF_NOMINAL * MQ135_LOAD_RESISTOR) / voltage) - MQ135_LOAD_RESISTOR;
  
  // In clean air, RS/R0 = CLEAN_AIR_RATIO
  _mq135R0 = rs / MQ135_CLEAN_AIR_RATIO;
  
  saveCalibration();
  Serial.printf("[CAL] MQ135 calibrated. R0 = %.2f ohms\n", _mq135R0);
}

void SensorManager::calibrateSCD30Temperature(float offset) {
  if (_scd30Ready) {
    _scd30.setTemperatureOffset(offset);
    Serial.printf("[CAL] SCD-30 temperature offset set to %.2f°C\n", offset);
  }
}

void SensorManager::loadCalibration() {
  loadFromNVS();
}

void SensorManager::saveCalibration() {
  saveToNVS();
}

void SensorManager::loadFromNVS() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, true);  // Read-only
  
  _adcCal.offset = prefs.getFloat("adc_offset", 0.0);
  _adcCal.scale = prefs.getFloat("adc_scale", 1.0);
  _adcCal.vRef = prefs.getFloat("adc_vref", ADC_VREF_NOMINAL);
  _adcCal.valid = prefs.getBool("adc_valid", false);
  
  _mq135R0 = prefs.getFloat("mq135_r0", 10.0);
  
  prefs.end();
  
  if (_adcCal.valid) {
    Serial.println("[OK] Calibration loaded from NVS");
  }
}

void SensorManager::saveToNVS() {
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, false);  // Read-write
  
  prefs.putFloat("adc_offset", _adcCal.offset);
  prefs.putFloat("adc_scale", _adcCal.scale);
  prefs.putFloat("adc_vref", _adcCal.vRef);
  prefs.putBool("adc_valid", _adcCal.valid);
  
  prefs.putFloat("mq135_r0", _mq135R0);
  
  prefs.end();
  
  Serial.println("[OK] Calibration saved to NVS");
}

// ============================================================================
// DEBUG OUTPUT
// ============================================================================

void SensorManager::printReadings() {
  Serial.println("┌─────────────────────────────────────────┐");
  Serial.println("│         SENSOR READINGS                 │");
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf("│ Air Temp:     %6.1f °C                 │\n", _data.airTemp);
  Serial.printf("│ Humidity:     %6.1f %%                  │\n", _data.airHumidity);
  Serial.printf("│ CO2:          %6.0f ppm                │\n", _data.co2);
  Serial.printf("│ Air Quality:  %6.0f ppm                │\n", _data.airQualityPPM);
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf("│ Soil Temp:    %6.1f °C                 │\n", _data.substrateTemp);
  Serial.printf("│ Soil VWC:     %6.1f %%                  │\n", _data.vwc);
  Serial.printf("│ Soil pH:      %6.2f                    │\n", _data.ph);
  Serial.printf("│ Soil EC:      %6.2f mS/cm              │\n", _data.ec);
  Serial.printf("│ N-P-K:        %3.0f-%3.0f-%3.0f mg/kg        │\n", 
                _data.nitrogen, _data.phosphorus, _data.potassium);
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf("│ Motion:       %s                     │\n", _data.motionDetected ? "YES" : "NO ");
  Serial.printf("│ UPS Active:   %s                     │\n", _data.upsActive ? "YES" : "NO ");
  Serial.printf("│ PAR:          %6.0f µmol/m²/s         │\n", _data.par);
  Serial.println("└─────────────────────────────────────────┘");
}
