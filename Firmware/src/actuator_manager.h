/**
 * GreenOS - Actuator Manager
 * 
 * Handles control of all greenhouse actuators (heaters, fans, pumps, lights)
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>

enum ActuatorState {
  OFF = 0,
  ON = 1
};

enum EmergencyType {
  LOW_TEMP,
  HIGH_TEMP,
  SECURITY_BREACH,
  WATER_LEAK,
  POWER_FAILURE
};

class ActuatorManager {
private:
  ActuatorState heaterPrimary;
  ActuatorState heaterSecondary;
  ActuatorState fanExhaust;
  ActuatorState fanCirculation;
  ActuatorState pumpIrrigation;
  ActuatorState lightGrow;
  
public:
  ActuatorManager();
  void init();
  
  // Individual control
  void setHeater(bool primary, bool state);
  void setFan(bool exhaust, bool state);
  void setPump(bool state);
  void setLight(bool state);
  
  // Emergency responses
  void handleEmergency(EmergencyType type);
  void stopAll();
  
  // Status
  void printStatus();
  
private:
  void activateRelay(int pin, bool state);
};

#endif // ACTUATOR_MANAGER_H
