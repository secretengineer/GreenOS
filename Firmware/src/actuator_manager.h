/**
 * GreenOS - Actuator Manager
 * 
 * Handles control of all greenhouse actuators with safety features
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>
#include "anomaly_detection.h"

enum EmergencyType {
  LOW_TEMP,
  HIGH_TEMP,
  SECURITY_BREACH,
  WATER_LEAK,
  POWER_FAILURE
};

class ActuatorManager {
public:
  ActuatorManager();
  void init();
  
  // Individual control
  void setHeater(bool primary, bool state);
  void setFan(bool exhaust, bool state);
  void setPump(bool state);
  void setLight(bool state);
  
  // Emergency and warning responses
  void handleEmergency(EmergencyType type);
  void handleWarning(AnomalyType type);
  
  // System control
  void stopAll();
  void printStatus();
  
  // State queries
  bool isHeaterOn(bool primary);
  bool isFanOn(bool exhaust);
  bool isPumpOn();
  bool isLightOn();
  
private:
  // Emergency protocols
  void emergencyLowTemperature();
  void emergencyHighTemperature();
  void emergencySecurityBreach();
  void emergencyWaterLeak();
  void emergencyPowerFailure();
};

#endif // ACTUATOR_MANAGER_H
