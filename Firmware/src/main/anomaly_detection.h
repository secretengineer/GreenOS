/**
 * GreenOS - Anomaly Detection
 * 
 * On-device anomaly detection for rapid response to critical conditions
 */

#ifndef ANOMALY_DETECTION_H
#define ANOMALY_DETECTION_H

#include <Arduino.h>
#include "sensor_manager.h"

enum AnomalyType {
  NONE,
  TEMP_TOO_LOW,
  TEMP_TOO_HIGH,
  HUMIDITY_TOO_LOW,
  HUMIDITY_TOO_HIGH,
  MOTION_OFF_HOURS,
  LOUD_NOISE,
  RAPID_TEMP_DROP,
  SENSOR_MALFUNCTION
};

class AnomalyDetection {
private:
  AnomalyType currentAnomaly;
  String anomalyDetails;
  float lastTemp;
  unsigned long lastCheckTime;
  
public:
  AnomalyDetection();
  void init();
  
  bool detectAnomalies(SensorData data);
  AnomalyType getAnomalyType();
  String getAnomalyDetails();
  
private:
  bool checkTemperature(float temp);
  bool checkHumidity(float humidity);
  bool checkMotion(bool motion);
  bool checkRapidChange(float currentTemp);
  bool checkSensorHealth(SensorData data);
};

#endif // ANOMALY_DETECTION_H
