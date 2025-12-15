/**
 * GreenOS - Firebase Communication
 *
 * Handles real-time data synchronization with Firebase using
 * a secure, token-based authentication method.
 */

 #include "firebase_comm.h"
 #include "config.h"
 #include <ArduinoJson.h>
 #include <WiFi.h>
 #include <Firebase_ESP_Client.h>
 
 // Firebase App and Auth objects
 FirebaseData fbdo;
 FirebaseAuth auth;
 FirebaseConfig config;
 
 // WiFi client for HTTPS requests
 WiFiClient client;
 
 // Define the Cloud Function URL.
 // IMPORTANT: Replace <YOUR_REGION> and <YOUR_PROJECT_ID> with your Firebase project details.
 #define TOKEN_GEN_URL "https://us-west3-greenos-24311.cloudfunctions.net/generateAuthToken"
 
 
 FirebaseComm::FirebaseComm() {
   this->connected = false;
   this->lastConnectionAttempt = 0;
   this->deviceId = GREENHOUSE_ID;
 }
 
 /**
  * Initializes the Firebase communication module.
  * It sets up the secure client and attempts to connect to Firebase.
  */
 void FirebaseComm::init() {
   Serial.println("Initializing Firebase...");
 
   // Note: WiFiClient on Arduino UNO R4 handles SSL/TLS automatically when connecting to HTTPS
   // No need to call setInsecure() or setCACert() with the standard WiFi library
 
   // Configure Firebase
   config.api_key = API_KEY;
   config.database_url = FIREBASE_HOST;
 
   if (this->connect()) {
     this->connected = true;
     Serial.println("Firebase initialized successfully.");
   } else {
     this->connected = false;
     Serial.println("Firebase initialization failed.");
   }
 }
 
 /**
  * Connects to Firebase by first fetching a custom auth token from a
  * Cloud Function and then using it to sign in.
  */
 bool FirebaseComm::connect() {
   Serial.println("Attempting to connect to Firebase...");
   this->lastConnectionAttempt = millis();
 
   // 1. Fetch Custom Auth Token from Cloud Function
   HTTPClient http;
   http.begin(client, TOKEN_GEN_URL);
   http.addHeader("Content-Type", "application/json");
 
   // Prepare the request body
   StaticJsonDocument<128> requestDoc;
   JsonObject data = requestDoc.createNestedObject("data");
   data["deviceId"] = this->deviceId;
 
   String requestBody;
   serializeJson(requestDoc, requestBody);
 
   Serial.println("Requesting auth token...");
   int httpCode = http.POST(requestBody);
 
   String token = "";
   if (httpCode == HTTP_CODE_OK) {
     StaticJsonDocument<512> responseDoc;
     DeserializationError error = deserializeJson(responseDoc, http.getStream());
 
     if (error) {
       Serial.print("deserializeJson() failed: ");
       Serial.println(error.c_str());
       http.end();
       return false;
     }
 
     token = responseDoc["result"]["token"].as<String>();
     Serial.println("Successfully fetched auth token.");
   } else {
     Serial.printf("Failed to fetch token. HTTP Code: %d\n", httpCode);
     String payload = http.getString();
     Serial.println("Error response: " + payload);
     http.end();
     return false;
   }
   http.end();
 
 
   if (token.length() == 0) {
       Serial.println("Auth token is empty.");
       return false;
   }
 
   // 2. Sign in to Firebase with the custom token
   config.token_status_callback = tokenStatusCallback; // Required for token processing
   Firebase.begin(&config, &auth);
   Serial.println("Signing in with custom token...");
 
   Firebase.signInWithCustomToken(token);
 
   if (Firebase.ready()) {
     Serial.println("Firebase sign-in successful.");
     return true;
   } else {
     Serial.printf("Firebase sign-in failed: %s\n", Firebase.errorReason().c_str());
     return false;
   }
 }
 
 /**
  * Sends sensor data to the Firebase Realtime Database.
  */
 bool FirebaseComm::syncSensorData(SensorData data) {
   if (!this->isConnected()) {
     Serial.println("Not connected to Firebase. Skipping data sync.");
     return false;
   }
 
   String path = "greenhouses/" + this->deviceId + "/sensors/latest";
 
   // Create a JSON object with the sensor data
   JsonDocument json;
   json["timestamp"] = data.timestamp;
   json["airTemp"] = data.airTemp;
   json["airHumidity"] = data.airHumidity;
   json["soilMoisture"] = data.soilMoisture;
   json["co2"] = data.co2;
   json["lightIntensity"] = data.lightIntensity;
   json["vwc"] = data.vwc;
 
   Serial.println("Syncing sensor data to Firebase...");
   if (Firebase.RTDB.setJSON(&fbdo, path, &json)) {
     Serial.println("Data sync successful.");
     return true;
   } else {
     Serial.printf("Data sync failed: %s\n", fbdo.errorReason().c_str());
     return false;
   }
 }
 
 /**
  * Sends an alert message to the Firebase Realtime Database.
  */
 bool FirebaseComm::sendAlert(String alertDetails) {
     if (!this->isConnected()) return false;
 
     String path = "greenhouses/" + this->deviceId + "/alerts";
     JsonDocument json;
     json["timestamp"] = millis();
     json["details"] = alertDetails;
     json["acknowledged"] = false;
 
     Serial.println("Sending alert to Firebase...");
     // Use pushJSON to create a new entry with a unique ID
     if (Firebase.RTDB.pushJSON(&fbdo, path, &json)) {
         Serial.println("Alert sent successfully.");
         return true;
     } else {
         Serial.printf("Failed to send alert: %s\n", fbdo.errorReason().c_str());
         return false;
     }
 }
 
 
 void FirebaseComm::checkForCommands(ActuatorManager& actuators) {
   // This function would typically listen to a 'commands' path in Firebase
   // and act on new commands. Implementation depends on the chosen RealtimeDB structure.
   // For simplicity, this is left as a placeholder.
 }
 
 void FirebaseComm::handleRealtimeUpdates(ActuatorManager& actuators) {
   // This function would handle any real-time updates from Firebase,
   // for example, for actuator state changes.
   // Implementation depends on the chosen RealtimeDB structure.
 }
 
 
 bool FirebaseComm::fetchConfig() {
   // This function would fetch configuration data from Firebase.
   // Placeholder implementation.
   return true;
 }
 
 bool FirebaseComm::updateConfig(String key, String value) {
   // This function would update configuration data in Firebase.
   // Placeholder implementation.
   return true;
 }
 
 
 bool FirebaseComm::isConnected() {
   // Check both WiFi status and Firebase readiness.
   return (WiFi.status() == WL_CONNECTED && Firebase.ready());
 }
 
 // Private helper functions would be defined here if needed.
 