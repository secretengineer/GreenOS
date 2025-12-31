/**
 * GreenOS - Sensor Manager Header
 * 
 * ESP32-WROOM-32E Sensor Management
 * Handles reading and managing all greenhouse sensors:
 * - SCD-30: NDIR CO2, Temperature, Humidity (I2C)
 * - MQ135: Air Quality Sensor (Analog - ADC1)
 * - Modbus RS485: Soil EC/pH/Moisture/Temperature/NPK
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_SCD30_Arduino_Library.h>
#include <ModbusMaster.h>
#include "config.h"

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
  float lightLevel;           // Raw light level (0-4095)
  
  // Security
  bool motionDetected;        // PIR sensor
  float noiseLevel;           // Microphone level (V)
  
  // Power
  bool upsActive;             // UPS status
  float voltage;              // System voltage (estimated)
  
  // Sensor Health (for monitoring)
  float scd30ErrorRate;       // % error rate
  float mq135ErrorRate;       // % error rate
  float modbusErrorRate;      // % error rate
  
  // Metadata
  unsigned long timestamp;    // milliseconds since boot
  time_t unixTime;            // Unix timestamp (if NTP synced)
  bool ntpSynced;             // Whether time is NTP synchronized
};

// ============================================================================
// SENSOR HEALTH REPORT
// ============================================================================

struct SensorHealthReport {
  // SCD-30 Health
  bool scd30Present;
  bool scd30Valid;
  float scd30ErrorRate;
  unsigned long scd30LastRead;
  uint16_t scd30ReadCount;
  uint16_t scd30ErrorCount;
  
  // MQ135 Health
  bool mq135Valid;
  float mq135ErrorRate;
  bool mq135Preheated;
  unsigned long mq135StartTime;
  
  // Modbus Health
  bool modbusPresent;
  bool modbusValid;
  float modbusErrorRate;
  unsigned long modbusLastRead;
  uint16_t modbusReadCount;
  uint16_t modbusErrorCount;
  
  // Overall system health
  bool systemHealthy;
  uint32_t freeHeap;
  uint32_t minFreeHeap;
};

// ============================================================================
// ADC CALIBRATION STRUCTURE
// ============================================================================

struct ADCCalibration {
  float offset;               // Zero point offset voltage
  float scale;                // Gain/slope correction factor
  float vRef;                 // Measured reference voltage
  float tempCoeff;            // Temperature drift coefficient
  bool valid;                 // Calibration validity flag
};

// ============================================================================
// SENSOR MANAGER CLASS
// ============================================================================

class SensorManager {
public:
  SensorManager();
  
  /**
   * Initialize all sensors
   * Should be called once during setup
   */
  void init();
  
  /**
   * Check if initialization is complete
   */
  bool isInitialized() const { return _initialized; }
  
  /**
   * Read all sensors
   * Updates internal data structure
   */
  void readAll();
  
  /**
   * Get the latest sensor data
   */
  SensorData getData() const { return _data; }
  
  /**
   * Print current readings to Serial
   */
  void printReadings();
  
  /**
   * Get sensor health report
   */
  SensorHealthReport getHealthReport();
  
  /**
   * Update health statistics
   */
  void updateHealthStatistics();
  
  // Calibration procedures
  void performADCCalibration();
  void calibrateMQ135();
  void calibrateSCD30Temperature(float offset);
  void loadCalibration();
  void saveCalibration();
  
  // Individual sensor status
  bool isSCD30Ready() const { return _scd30Ready; }
  bool isModbusReady() const { return _modbusReady; }
  bool isMQ135Preheated() const;
  
private:
  // Sensor data
  SensorData _data;
  
  // Sensor objects
  SCD30 _scd30;
  ModbusMaster _modbus;
  
  // Hardware serial for Modbus
  HardwareSerial _modbusSerial;
  
  // State flags
  bool _initialized;
  bool _scd30Ready;
  bool _modbusReady;
  unsigned long _mq135StartTime;
  
  // Health tracking
  uint16_t _scd30ReadCount;
  uint16_t _scd30ErrorCount;
  uint16_t _modbusReadCount;
  uint16_t _modbusErrorCount;
  unsigned long _scd30LastRead;
  unsigned long _modbusLastRead;
  
  // Calibration data
  ADCCalibration _adcCal;
  float _mq135R0;  // MQ135 calibrated R0 value
  
  // Last known good values (for fallback)
  SensorData _lastGoodData;
  
  // Individual sensor readers
  void readSCD30();
  void readMQ135();
  void readModbusSensor();
  void readDigitalSensors();
  void readAnalogSensors();
  
  // ADC utilities for ESP32
  float readCalibratedADC(int pin, int samples = ADC_SAMPLES);
  void configureADC();
  
  // Modbus utilities
  static void preTransmission();
  static void postTransmission();
  
  // Validation
  bool validateReading(float value, float min, float max);
  void applyAltitudeCompensation();
  
  // NVS storage
  void loadFromNVS();
  void saveToNVS();
};

// Global reference for Modbus callbacks
extern SensorManager* g_sensorManager;

#endif // SENSOR_MANAGER_H
