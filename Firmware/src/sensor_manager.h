/**
 * GreenOS - Sensor Manager
 * 
 * Handles reading and managing all greenhouse sensors
 */

#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>

struct SensorData {
  // Environmental
  float airTemp;
  float airHumidity;
  float substrateTemp;
  float vwc;           // Volumetric Water Content
  float ph;
  float ec;            // Electrical Conductivity
  float co2;           // CO2 ppm
  float par;           // Photosynthetically Active Radiation
  
  // Security
  bool motionDetected;
  float noiseLevel;
  
  // Power
  bool upsActive;
  float voltage;
  
  // Metadata
  unsigned long timestamp;
};

class SensorManager {
private:
  DHT dht;
  SensorData data;
  
public:
  SensorManager();
  void init();
  void readAll();
  SensorData getData();
  void printReadings();
  
private:
  float readVWC();
  float readPH();
  float readEC();
  float readCO2();
  float readPAR();
  float readSubstrateTemp();
  bool readMotion();
  float readNoise();
  bool readUPSStatus();
};

#endif // SENSOR_MANAGER_H
