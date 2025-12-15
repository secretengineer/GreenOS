/**
 * GreenOS - Firebase Communication
 *
 * Handles real-time data synchronization with Firebase using
 * a secure, token-based authentication method.
 * 
 * NOTE: Firebase ESP Client library is not compatible with Arduino UNO R4.
 * This is a simplified stub implementation for initial testing.
 * For production, implement Firebase REST API calls using WiFiClient.
 */

 #include "firebase_comm.h"
 #include "config.h"
 #include <ArduinoJson.h>
 // Note: WiFi disabled due to BSP incompatibility on Arduino UNO Q
 
 FirebaseComm::FirebaseComm() {
   this->connected = false;
   this->lastConnectionAttempt = 0;
   this->deviceId = GREENHOUSE_ID;
 }
 
 /**
  * Initializes the Firebase communication module.
  * STUB: Firebase ESP Client not compatible with UNO R4 - using placeholder
  */
 void FirebaseComm::init() {
   Serial.println("Initializing Firebase (Stub Mode)...");
   Serial.println("⚠️  Firebase integration disabled for initial testing");
   Serial.println("ℹ️  System will operate in offline mode with local data logging");
   
   // Mark as "connected" for testing purposes (no actual connection)
   this->connected = false;
 }
 
 /**
  * Connects to Firebase
  * STUB: Returns false to indicate no connection
  */
 bool FirebaseComm::connect() {
   Serial.println("Firebase connection skipped (stub mode)");
   this->lastConnectionAttempt = millis();
   return false;  // Not actually connected
 }

 /**
  * Sends sensor data to the Firebase Realtime Database.
  * STUB: Not implemented - would require Firebase REST API
  */
 bool FirebaseComm::syncSensorData(SensorData data) {
   // Stub: Log locally instead of syncing
   Serial.println("📊 Sensor data logged locally (Firebase stub)");
   return false;  // Indicate no cloud sync occurred
 }
 
 /**
  * Sends an alert message to the Firebase Realtime Database.
  * STUB: Not implemented - would require Firebase REST API
  */
 bool FirebaseComm::sendAlert(String alertDetails) {
   Serial.print("🚨 ALERT (local only): ");
   Serial.println(alertDetails);
   return false;  // Indicate no cloud sync occurred
 }
 
 /**
  * Check for commands from Firebase
  * STUB: Not implemented
  */
 void FirebaseComm::checkForCommands(ActuatorManager& actuators) {
   // Stub: No commands to check
 }
 
 /**
  * Handle real-time updates from Firebase
  * STUB: Not implemented
  */
 void FirebaseComm::handleRealtimeUpdates(ActuatorManager& actuators) {
   // Stub: No updates to handle
 }
 
 /**
  * Fetch configuration from Firebase
  * STUB: Not implemented
  */
 bool FirebaseComm::fetchConfig() {
   return false;
 }
 
 /**
  * Update configuration in Firebase
  * STUB: Not implemented
  */
 bool FirebaseComm::updateConfig(String key, String value) {
   return false;
 }
 
 /**
  * Check if connected to Firebase
  * STUB: Always returns false (not connected)
  */
 bool FirebaseComm::isConnected() {
   return false;  // Stub mode - never connected
 }

 