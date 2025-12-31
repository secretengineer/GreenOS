/**
 * GreenOS - Network Manager Implementation
 * 
 * ESP32-WROOM-32E Network Management
 */

#include "network_manager.h"
#include <ArduinoJson.h>

// Firebase token helper
#include <addons/TokenHelper.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

NetworkManager::NetworkManager()
  : _firebaseReady(false),
    _ntpSynced(false),
    _lastWifiAttempt(0),
    _lastFirebaseSync(0),
    _lastNTPSync(0),
    _wifiConnectTime(0),
    _syncSuccessCount(0),
    _syncFailCount(0),
    _alertQueueHead(0),
    _alertQueueTail(0)
{
  memset(_alertQueue, 0, sizeof(_alertQueue));
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void NetworkManager::init() {
  Serial.println("[NETWORK] Initializing network manager...");
  
  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(WIFI_HOSTNAME);
  
  // Start WiFi connection
  Serial.printf("[NETWORK] Connecting to WiFi: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  _lastWifiAttempt = millis();
  
  // Wait for initial connection (with timeout)
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    _wifiConnectTime = millis();
    Serial.println("[OK] WiFi connected!");
    Serial.printf("     IP Address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("     MAC Address: %s\n", WiFi.macAddress().c_str());
    Serial.printf("     RSSI: %d dBm\n", WiFi.RSSI());
    
    // Setup OTA
    setupOTA();
    
    // Sync time
    syncTime();
  } else {
    Serial.println("[WARN] WiFi connection failed - will retry in background");
  }
  
  Serial.println("[OK] Network manager initialized\n");
}

// ============================================================================
// WIFI MANAGEMENT
// ============================================================================

void NetworkManager::maintainConnection() {
  // Check WiFi status and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - _lastWifiAttempt > WIFI_RECONNECT_INTERVAL) {
      Serial.println("[NETWORK] WiFi disconnected - attempting reconnection...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      _lastWifiAttempt = millis();
    }
  } else if (_wifiConnectTime == 0) {
    // Just reconnected
    _wifiConnectTime = millis();
    Serial.printf("[NETWORK] WiFi reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
  }
}

bool NetworkManager::connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECTION_TIMEOUT) {
    delay(100);
  }
  
  return WiFi.status() == WL_CONNECTED;
}

// ============================================================================
// FIREBASE OPERATIONS
// ============================================================================

bool NetworkManager::authenticateFirebase() {
  if (!isConnected()) {
    Serial.println("[FIREBASE] Cannot authenticate - WiFi not connected");
    return false;
  }
  
  Serial.println("[FIREBASE] Authenticating with Firebase...");
  
  // Configure Firebase
  _fbConfig.api_key = FIREBASE_API_KEY;
  _fbConfig.database_url = "";  // Not using Realtime Database
  
  // Use anonymous authentication for device
  _auth.user.email = "";
  _auth.user.password = "";
  
  // Token callback
  _fbConfig.token_status_callback = tokenStatusCallback;
  
  // Initialize Firebase
  Firebase.begin(&_fbConfig, &_auth);
  Firebase.reconnectWiFi(true);
  
  // Wait for token
  unsigned long startTime = millis();
  while (!Firebase.ready() && millis() - startTime < 10000) {
    delay(100);
  }
  
  if (Firebase.ready()) {
    _firebaseReady = true;
    Serial.println("[OK] Firebase authenticated!");
    return true;
  } else {
    Serial.println("[WARN] Firebase authentication failed");
    return false;
  }
}

bool NetworkManager::syncSensorData(const SensorData& data) {
  if (!isConnected() || !_firebaseReady) {
    return false;
  }
  
  // Build document path
  String documentPath = "greenhouses/";
  documentPath += GREENHOUSE_ID;
  documentPath += "/readings/";
  documentPath += String(data.unixTime);
  
  // Create JSON payload
  FirebaseJson content;
  
  // Add sensor data
  content.set("fields/airTemp/doubleValue", data.airTemp);
  content.set("fields/airHumidity/doubleValue", data.airHumidity);
  content.set("fields/co2/doubleValue", data.co2);
  content.set("fields/airQualityPPM/doubleValue", data.airQualityPPM);
  content.set("fields/substrateTemp/doubleValue", data.substrateTemp);
  content.set("fields/vwc/doubleValue", data.vwc);
  content.set("fields/ph/doubleValue", data.ph);
  content.set("fields/ec/doubleValue", data.ec);
  content.set("fields/nitrogen/doubleValue", data.nitrogen);
  content.set("fields/phosphorus/doubleValue", data.phosphorus);
  content.set("fields/potassium/doubleValue", data.potassium);
  content.set("fields/par/doubleValue", data.par);
  content.set("fields/motionDetected/booleanValue", data.motionDetected);
  content.set("fields/upsActive/booleanValue", data.upsActive);
  content.set("fields/timestamp/integerValue", String(data.unixTime));
  content.set("fields/deviceId/stringValue", GREENHOUSE_ID);
  
  // Add health metrics
  content.set("fields/scd30ErrorRate/doubleValue", data.scd30ErrorRate);
  content.set("fields/modbusErrorRate/doubleValue", data.modbusErrorRate);
  
  // Send to Firestore
  if (Firebase.Firestore.createDocument(&_fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
    _syncSuccessCount++;
    _lastFirebaseSync = millis();
    Serial.printf("[FIREBASE] Data synced successfully (%lu successes)\n", _syncSuccessCount);
    return true;
  } else {
    _syncFailCount++;
    Serial.printf("[FIREBASE] Sync failed: %s\n", _fbdo.errorReason().c_str());
    return false;
  }
}

void NetworkManager::queueAlert(AlertPriority priority, const String& message, const String& type) {
  // Add to circular queue
  _alertQueue[_alertQueueTail].priority = priority;
  _alertQueue[_alertQueueTail].message = message;
  _alertQueue[_alertQueueTail].type = type;
  _alertQueue[_alertQueueTail].timestamp = getUnixTime();
  _alertQueue[_alertQueueTail].sent = false;
  
  _alertQueueTail = (_alertQueueTail + 1) % MAX_PENDING_ALERTS;
  
  // If queue is full, overwrite oldest
  if (_alertQueueTail == _alertQueueHead) {
    _alertQueueHead = (_alertQueueHead + 1) % MAX_PENDING_ALERTS;
  }
  
  Serial.printf("[ALERT] Queued: %s\n", message.c_str());
}

void NetworkManager::sendPendingAlerts() {
  if (!isConnected() || !_firebaseReady) {
    return;
  }
  
  while (_alertQueueHead != _alertQueueTail) {
    PendingAlert& alert = _alertQueue[_alertQueueHead];
    
    if (!alert.sent) {
      // Build alert document
      String documentPath = "greenhouses/";
      documentPath += GREENHOUSE_ID;
      documentPath += "/alerts/";
      documentPath += String(alert.timestamp);
      
      FirebaseJson content;
      content.set("fields/priority/integerValue", String((int)alert.priority));
      content.set("fields/message/stringValue", alert.message);
      content.set("fields/type/stringValue", alert.type);
      content.set("fields/timestamp/integerValue", String(alert.timestamp));
      content.set("fields/deviceId/stringValue", GREENHOUSE_ID);
      content.set("fields/acknowledged/booleanValue", false);
      
      if (Firebase.Firestore.createDocument(&_fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
        alert.sent = true;
        Serial.printf("[ALERT] Sent: %s\n", alert.message.c_str());
      } else {
        // Failed to send, keep in queue
        Serial.printf("[ALERT] Failed to send: %s\n", _fbdo.errorReason().c_str());
        break;
      }
    }
    
    _alertQueueHead = (_alertQueueHead + 1) % MAX_PENDING_ALERTS;
  }
}

void NetworkManager::checkForCommands(ActuatorManager& actuators) {
  if (!isConnected() || !_firebaseReady) {
    return;
  }
  
  // Check for pending commands in Firestore
  String documentPath = "greenhouses/";
  documentPath += GREENHOUSE_ID;
  documentPath += "/commands/pending";
  
  if (Firebase.Firestore.getDocument(&_fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    // Parse command
    FirebaseJson payload;
    payload.setJsonData(_fbdo.payload());
    
    FirebaseJsonData commandData;
    if (payload.get(commandData, "fields/command/stringValue")) {
      String command = commandData.stringValue;
      if (command.length() > 0) {
        Serial.printf("[COMMAND] Received: %s\n", command.c_str());
        processFirebaseCommand(command, actuators);
        
        // Mark command as processed by deleting it
        Firebase.Firestore.deleteDocument(&_fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str());
      }
    }
  }
}

void NetworkManager::processFirebaseCommand(const String& command, ActuatorManager& actuators) {
  // Parse command format: "actuator:action" (e.g., "heater_primary:on")
  int colonIndex = command.indexOf(':');
  if (colonIndex < 0) {
    Serial.println("[COMMAND] Invalid command format");
    return;
  }
  
  String actuator = command.substring(0, colonIndex);
  String action = command.substring(colonIndex + 1);
  bool state = (action == "on" || action == "1" || action == "true");
  
  if (actuator == "heater_primary") {
    actuators.setHeater(true, state);
  } else if (actuator == "heater_secondary") {
    actuators.setHeater(false, state);
  } else if (actuator == "fan_exhaust") {
    actuators.setFan(true, state);
  } else if (actuator == "fan_circulation") {
    actuators.setFan(false, state);
  } else if (actuator == "pump") {
    actuators.setPump(state);
  } else if (actuator == "light") {
    actuators.setLight(state);
  } else if (actuator == "stop_all") {
    actuators.stopAll();
  } else {
    Serial.printf("[COMMAND] Unknown actuator: %s\n", actuator.c_str());
  }
}

bool NetworkManager::fetchConfigUpdates() {
  if (!isConnected() || !_firebaseReady) {
    return false;
  }
  
  String documentPath = "greenhouses/";
  documentPath += GREENHOUSE_ID;
  documentPath += "/config";
  
  if (Firebase.Firestore.getDocument(&_fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    // Parse and apply configuration updates
    // (Implementation depends on what config is stored)
    return true;
  }
  
  return false;
}

// ============================================================================
// TIME SYNCHRONIZATION
// ============================================================================

bool NetworkManager::syncTime() {
  if (!isConnected()) {
    return false;
  }
  
  Serial.println("[NTP] Syncing time...");
  
  configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  // Wait for time to sync
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  
  while (now < 1609459200 && retry < 10) {  // Jan 1, 2021
    delay(500);
    time(&now);
    localtime_r(&now, &timeinfo);
    retry++;
  }
  
  if (now > 1609459200) {
    _ntpSynced = true;
    _lastNTPSync = millis();
    Serial.printf("[OK] Time synced: %s\n", getFormattedTime().c_str());
    return true;
  }
  
  Serial.println("[WARN] NTP sync failed");
  return false;
}

time_t NetworkManager::getUnixTime() {
  time_t now;
  time(&now);
  return now;
}

String NetworkManager::getFormattedTime() {
  time_t now;
  struct tm timeinfo;
  char buffer[30];
  
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  
  return String(buffer);
}

// ============================================================================
// OTA UPDATES
// ============================================================================

void NetworkManager::setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setPort(OTA_PORT);
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    Serial.println("[OTA] Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update complete!");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  if (OTA_ENABLED) {
    ArduinoOTA.begin();
    Serial.printf("[OK] OTA ready at %s.local:%d\n", OTA_HOSTNAME, OTA_PORT);
  }
}

void NetworkManager::handleOTA() {
  if (OTA_ENABLED && isConnected()) {
    ArduinoOTA.handle();
  }
}

bool NetworkManager::checkForOTAUpdate() {
  // Check Firebase for OTA update flag
  // Returns true if update available
  return false;  // Placeholder
}

// ============================================================================
// STATUS
// ============================================================================

NetworkStatus NetworkManager::getStatus() {
  NetworkStatus status;
  
  status.wifiConnected = WiFi.status() == WL_CONNECTED;
  status.firebaseConnected = _firebaseReady;
  status.ntpSynced = _ntpSynced;
  status.otaReady = OTA_ENABLED && status.wifiConnected;
  
  if (status.wifiConnected) {
    status.wifiRSSI = WiFi.RSSI();
    status.wifiSSID = WiFi.SSID();
    status.localIP = WiFi.localIP().toString();
    status.macAddress = WiFi.macAddress();
  } else {
    status.wifiRSSI = 0;
    status.wifiSSID = "";
    status.localIP = "";
    status.macAddress = WiFi.macAddress();
  }
  
  status.lastFirebaseSync = _lastFirebaseSync;
  status.lastNTPSync = _lastNTPSync;
  status.wifiConnectTime = _wifiConnectTime;
  status.syncSuccessCount = _syncSuccessCount;
  status.syncFailCount = _syncFailCount;
  
  return status;
}
