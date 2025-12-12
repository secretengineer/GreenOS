/**
 * GreenOS - Arduino UNO Q Main Firmware
 * 
 * Core loop for sensor polling, actuator control, and Firebase communication
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "firebase_comm.h"
#include "anomaly_detection.h"

// Global objects
SensorManager sensors;
ActuatorManager actuators;
FirebaseComm firebase;
AnomalyDetection anomaly;

// Timing variables
unsigned long lastSensorRead = 0;
unsigned long lastFirebaseSync = 0;
unsigned long lastAnomalyCheck = 0;

const unsigned long SENSOR_READ_INTERVAL = 5000;      // 5 seconds
const unsigned long FIREBASE_SYNC_INTERVAL = 60000;   // 1 minute
const unsigned long ANOMALY_CHECK_INTERVAL = 10000;   // 10 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("GreenOS - Initializing...");
  
  // Initialize WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize components
  sensors.init();
  actuators.init();
  firebase.init();
  anomaly.init();
  
  Serial.println("GreenOS - Ready!");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    sensors.readAll();
    
    // Log to serial for debugging
    Serial.println("=== Sensor Readings ===");
    sensors.printReadings();
  }
  
  // Check for anomalies
  if (currentMillis - lastAnomalyCheck >= ANOMALY_CHECK_INTERVAL) {
    lastAnomalyCheck = currentMillis;
    
    if (anomaly.detectAnomalies(sensors.getData())) {
      Serial.println("⚠️ ANOMALY DETECTED!");
      // Trigger automated response
      actuators.handleEmergency(anomaly.getAnomalyType());
      // Send immediate alert to Firebase
      firebase.sendAlert(anomaly.getAnomalyDetails());
    }
  }
  
  // Sync with Firebase periodically
  if (currentMillis - lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    lastFirebaseSync = currentMillis;
    
    if (WiFi.status() == WL_CONNECTED) {
      firebase.syncSensorData(sensors.getData());
      firebase.checkForCommands(actuators);
    } else {
      Serial.println("WiFi disconnected. Attempting to reconnect...");
      WiFi.reconnect();
    }
  }
  
  // Handle any incoming Firebase commands in real-time
  firebase.handleRealtimeUpdates(actuators);
  
  // Small delay to prevent watchdog issues
  delay(10);
}
