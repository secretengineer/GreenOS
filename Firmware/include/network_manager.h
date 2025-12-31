/**
 * GreenOS - Network Manager Header
 * 
 * ESP32-WROOM-32E Network Management
 * Handles WiFi connectivity, Firebase communication, NTP sync, and OTA updates
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoOTA.h>
#include <time.h>

#include "config.h"
#include "sensor_manager.h"
#include "actuator_manager.h"

// ============================================================================
// NETWORK STATUS STRUCTURE
// ============================================================================

struct NetworkStatus {
  bool wifiConnected;
  bool firebaseConnected;
  bool ntpSynced;
  bool otaReady;
  
  int wifiRSSI;
  String wifiSSID;
  String localIP;
  String macAddress;
  
  unsigned long lastFirebaseSync;
  unsigned long lastNTPSync;
  unsigned long wifiConnectTime;
  
  uint32_t syncSuccessCount;
  uint32_t syncFailCount;
};

// ============================================================================
// PENDING ALERT STRUCTURE
// ============================================================================

struct PendingAlert {
  AlertPriority priority;
  String message;
  String type;
  unsigned long timestamp;
  bool sent;
};

// ============================================================================
// NETWORK MANAGER CLASS
// ============================================================================

class NetworkManager {
public:
  NetworkManager();
  
  /**
   * Initialize WiFi, Firebase, and OTA
   */
  void init();
  
  /**
   * Maintain WiFi connection (call periodically)
   */
  void maintainConnection();
  
  /**
   * Check if WiFi is connected
   */
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  
  /**
   * Check if Firebase is authenticated
   */
  bool isFirebaseReady() const { return _firebaseReady; }
  
  /**
   * Get current network status
   */
  NetworkStatus getStatus();
  
  // ─────────────────────────────────────────────────────────────────────────
  // Firebase Operations
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Authenticate with Firebase
   * @return True if authentication successful
   */
  bool authenticateFirebase();
  
  /**
   * Sync sensor data to Firebase Firestore
   */
  bool syncSensorData(const SensorData& data);
  
  /**
   * Queue an alert for sending
   */
  void queueAlert(AlertPriority priority, const String& message, const String& type);
  
  /**
   * Send all pending alerts
   */
  void sendPendingAlerts();
  
  /**
   * Check for remote commands and execute them
   */
  void checkForCommands(ActuatorManager& actuators);
  
  /**
   * Fetch configuration updates from Firebase
   */
  bool fetchConfigUpdates();
  
  // ─────────────────────────────────────────────────────────────────────────
  // Time Synchronization
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Sync time with NTP server
   */
  bool syncTime();
  
  /**
   * Get current Unix timestamp
   */
  time_t getUnixTime();
  
  /**
   * Get formatted time string
   */
  String getFormattedTime();
  
  // ─────────────────────────────────────────────────────────────────────────
  // OTA Updates
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Handle OTA update requests
   */
  void handleOTA();
  
  /**
   * Check if OTA update is available (from Firebase config)
   */
  bool checkForOTAUpdate();
  
private:
  // Firebase objects
  FirebaseData _fbdo;
  FirebaseAuth _auth;
  FirebaseConfig _fbConfig;
  
  // State
  bool _firebaseReady;
  bool _ntpSynced;
  unsigned long _lastWifiAttempt;
  unsigned long _lastFirebaseSync;
  unsigned long _lastNTPSync;
  unsigned long _wifiConnectTime;
  
  // Statistics
  uint32_t _syncSuccessCount;
  uint32_t _syncFailCount;
  
  // Alert queue
  static const int MAX_PENDING_ALERTS = 10;
  PendingAlert _alertQueue[MAX_PENDING_ALERTS];
  int _alertQueueHead;
  int _alertQueueTail;
  
  // Internal methods
  bool connectWiFi();
  void setupOTA();
  String sensorDataToJson(const SensorData& data);
  void processFirebaseCommand(const String& command, ActuatorManager& actuators);
};

#endif // NETWORK_MANAGER_H
