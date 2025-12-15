/**
 * GreenOS - Sensor Manager
 * 
 * Handles reading and managing all greenhouse sensors:
 * - Adafruit SCD-30: NDIR CO2, Temperature, Humidity (I2C)
 * - MQ135: Air Quality Sensor (Analog)
 * - Modbus RS485: Soil EC/pH/Moisture/Temperature/NPK
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

// ============================================================================
// SENSOR DATA STRUCTURE
// ============================================================================

struct SensorData {
  // Environmental - Air
  float airTemp;              // °C from SCD-30
  float airHumidity;          // % RH from SCD-30
  float co2;                  // ppm from SCD-30
  float airQualityPPM;        // ppm from MQ135
  
  // Environmental - Soil (from Modbus sensor)
  float substrateTemp;        // °C soil temperature
  float vwc;                  // % Volumetric Water Content
  float ph;                   // pH value
  float ec;                   // mS/cm Electrical Conductivity
  float nitrogen;             // mg/kg (ppm) N
  float phosphorus;           // mg/kg (ppm) P
  float potassium;            // mg/kg (ppm) K
  
  // Light
  float par;                  // µmol/m²/s Photosynthetically Active Radiation
  
  // Security
  bool motionDetected;        // PIR sensor
  float noiseLevel;           // Microphone level (V)
  
  // Power
  bool upsActive;             // UPS status
  float voltage;              // System voltage
  
  // Sensor Health (for monitoring)
  float scd30ErrorRate;       // % error rate
  float mq135ErrorRate;       // % error rate
  float modbusErrorRate;      // % error rate
  
  // Metadata
  unsigned long timestamp;    // milliseconds since boot
};

// ============================================================================
// SENSOR HEALTH REPORT
// ============================================================================

struct SensorHealthReport {
  // SCD-30 Health
  bool scd30Valid;
  float scd30ErrorRate;
  unsigned long scd30LastRead;
  
  // MQ135 Health
  bool mq135Valid;
  float mq135ErrorRate;
  bool mq135Preheated;
  
  // Modbus Health
  bool modbusValid;
  float modbusErrorRate;
  unsigned long modbusLastRead;
};

// ============================================================================
// SENSOR MANAGER CLASS
// ============================================================================

class SensorManager {
private:
  SensorData data;
  
public:
  SensorManager();
  void init();
  void readAll();
  SensorData getData();
  void printReadings();
  
  // Sensor health monitoring
  SensorHealthReport getHealthReport();
  void updateHealthStatistics();
  
  // Calibration procedures
  void performADCCalibration();
  void calibrateMQ135();
  void loadADCCalibration();
  void saveADCCalibration();
  
private:
  // Individual sensor readers
  void readSCD30();
  void readMQ135();
  void readModbusSensor();
  
  // ADC utilities
  float readCalibratedADC(int pin);
  
  // Utility functions
  uint32_t calculateCRC32(const uint8_t* data, size_t length);
};

#endif // SENSOR_MANAGER_H
