/**
 * GreenOS - Firebase Communication
 * 
 * Handles real-time data synchronization with Firebase
 */

#ifndef FIREBASE_COMM_H
#define FIREBASE_COMM_H

#include <Arduino.h>
#include "sensor_manager.h"
#include "actuator_manager.h"

class FirebaseComm {
private:
  String deviceId;
  bool connected;
  unsigned long lastConnectionAttempt;
  
public:
  FirebaseComm();
  void init();
  
  // Data sync
  bool syncSensorData(SensorData data);
  bool sendAlert(String alertDetails);
  
  // Command handling
  void checkForCommands(ActuatorManager& actuators);
  void handleRealtimeUpdates(ActuatorManager& actuators);
  
  // Configuration
  bool fetchConfig();
  bool updateConfig(String key, String value);
  
  // Status
  bool isConnected();
  
private:
  bool connect();
  bool sendData(String path, String json);
  String receiveData(String path);
};

#endif // FIREBASE_COMM_H
