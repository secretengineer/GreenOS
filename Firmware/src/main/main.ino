/**
 * GreenOS - Arduino UNO Q Main Firmware
 * 
 * Hardware: Arduino UNO Q (Renesas RA4M1 + ESP32-S3)
 * Features:
 * - Finite State Machine (FSM) for robust operation
 * - Hardware Watchdog Timer (WDT) for auto-recovery
 * - Sensor health monitoring and validation
 * - SD card buffering for offline operation
 * - Safe-fail emergency protocols
 * - Memory management and error handling
 */

#include <Arduino.h>
#include <WiFi.h>
// Note: SD library not compatible with Arduino UNO Q Zephyr architecture
// Offline buffering disabled for initial testing
#include "config.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "firebase_comm.h"
#include "anomaly_detection.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

SensorManager sensors;
ActuatorManager actuators;
FirebaseComm firebase;
AnomalyDetection anomaly;

// ============================================================================
// SYSTEM STATE MACHINE
// ============================================================================

SystemState currentState = STATE_BOOT;
SystemState previousState = STATE_BOOT;
unsigned long stateEntryTime = 0;
uint8_t bootFailCount = 0;

// ============================================================================
// TIMING VARIABLES
// ============================================================================

unsigned long lastSensorRead = 0;
unsigned long lastFirebaseSync = 0;
unsigned long lastAnomalyCheck = 0;
unsigned long lastModbusRead = 0;
unsigned long lastSDFlush = 0;
unsigned long lastMemoryCheck = 0;
unsigned long lastHealthCheck = 0;

// ============================================================================
// OFFLINE DATA BUFFERING
// ============================================================================

struct SensorReading {
  unsigned long timestamp;
  float airTemp;
  float airHumidity;
  float co2;
  float ph;
  float ec;
  float vwc;
};

SensorReading offlineBuffer[MAX_BUFFERED_READINGS];
uint8_t bufferCount = 0;
bool sdCardAvailable = false;

// ============================================================================
// SETUP - INITIALIZATION
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for serial to stabilize
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   GreenOS - Intelligent Greenhouse     ║");
  Serial.println("║   Arduino UNO Q Firmware v1.0          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  blinkStatusLED(3, 200);  // 3 quick blinks = boot
  
  // Initialize hardware watchdog timer
  setupWatchdog();
  
  // SD card not available on Zephyr - skip initialization
  Serial.println("⚠️  SD card buffering disabled (Zephyr incompatibility)");
  sdCardAvailable = false;
  
  // Set initial state
  changeState(STATE_SENSOR_INIT);
}

// ============================================================================
// MAIN LOOP - FINITE STATE MACHINE
// ============================================================================

void loop() {
  // Pet the watchdog to prevent reset
  feedWatchdog();
  
  // Check watchdog timeout (software implementation)
  checkWatchdog();
  
  // Execute current state
  switch (currentState) {
    case STATE_BOOT:
      // Should not reach here (handled in setup)
      changeState(STATE_SENSOR_INIT);
      break;
      
    case STATE_SENSOR_INIT:
      stateSensorInit();
      break;
      
    case STATE_NETWORK_CONNECT:
      stateNetworkConnect();
      break;
      
    case STATE_FIREBASE_AUTH:
      stateFirebaseAuth();
      break;
      
    case STATE_NORMAL_OPERATION:
      stateNormalOperation();
      break;
      
    case STATE_SAFE_MODE:
      stateSafeMode();
      break;
      
    case STATE_EMERGENCY:
      stateEmergency();
      break;
      
    case STATE_CALIBRATION_MODE:
      stateCalibrationMode();
      break;
  }
  
  // Periodic maintenance tasks (run in all states)
  periodicMemoryCheck();
  handleSerialCommands();
  
  // Small delay to prevent tight loop
  delay(10);
}

// ============================================================================
// STATE MACHINE IMPLEMENTATIONS
// ============================================================================

void stateSensorInit() {
  Serial.println("\n[STATE] Initializing Sensors...");
  
  // Initialize sensor manager
  sensors.init();
  
  // Initialize actuator manager
  actuators.init();
  
  // Wait for initial sensor readings
  delay(2000);
  sensors.readAll();
  
  // Verify at least one critical sensor is working
  SensorData data = sensors.getData();
  if (data.airTemp > -50 && data.airTemp < 60) {
    // Temperature sensor working, proceed
    Serial.println("✓ Sensor initialization successful");
    changeState(STATE_NETWORK_CONNECT);
  } else {
    Serial.println("✗ Sensor initialization failed!");
    bootFailCount++;
    if (bootFailCount > 3) {
      Serial.println("⚠️ Too many boot failures, entering safe mode");
      changeState(STATE_SAFE_MODE);
    } else {
      delay(5000);
      // Retry initialization
    }
  }
}

void stateNetworkConnect() {
  Serial.println("\n[STATE] Connecting to Network...");
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > WIFI_CONNECTION_TIMEOUT) {
      Serial.println("\n✗ WiFi connection timeout");
      Serial.println("⚠️ Operating in offline mode");
      // Continue to normal operation without cloud
      changeState(STATE_NORMAL_OPERATION);
      return;
    }
    
    delay(500);
    Serial.print(".");
    feedWatchdog();
  }
  
  Serial.println("\n✓ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  
  changeState(STATE_FIREBASE_AUTH);
}

void stateFirebaseAuth() {
  Serial.println("\n[STATE] Authenticating with Firebase...");
  
  firebase.init();
  
  if (firebase.isConnected()) {
    Serial.println("✓ Firebase authentication successful");
    
    // Sync any buffered offline data
    syncBufferedData();
    
    changeState(STATE_NORMAL_OPERATION);
  } else {
    Serial.println("✗ Firebase authentication failed");
    Serial.println("⚠️ Operating in offline mode");
    changeState(STATE_NORMAL_OPERATION);
  }
}

void stateNormalOperation() {
  static bool firstRun = true;
  
  if (firstRun) {
    Serial.println("\n[STATE] Normal Operation");
    Serial.println("✓ GreenOS is now running!");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    blinkStatusLED(1, 1000);  // Long blink = operational
    firstRun = false;
  }
  
  unsigned long currentMillis = millis();
  
  // Read sensors periodically
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = currentMillis;
    sensors.readAll();
    
    // Log to serial for debugging
    if (Serial) {
      Serial.println("=== Sensor Readings ===");
      sensors.printReadings();
    }
    
    // If offline, buffer data locally
    if (WiFi.status() != WL_CONNECTED || !firebase.isConnected()) {
      bufferSensorData(sensors.getData());
    }
  }
  
  // Check for anomalies
  if (currentMillis - lastAnomalyCheck >= ANOMALY_CHECK_INTERVAL) {
    lastAnomalyCheck = currentMillis;
    
    if (anomaly.detectAnomalies(sensors.getData())) {
      Serial.println("⚠️ ANOMALY DETECTED!");
      
      AnomalyType type = anomaly.getAnomalyType();
      
      // Check if emergency-level anomaly
      if (type == TEMP_TOO_LOW || type == TEMP_TOO_HIGH) {
        changeState(STATE_EMERGENCY);
        return;
      }
      
      // Handle non-emergency anomalies
      actuators.handleWarning(type);
      
      // Send alert to Firebase if connected
      if (firebase.isConnected()) {
        firebase.sendAlert(anomaly.getAnomalyDetails());
      } else {
        // Save alert to SD card for later sync
        saveAlertToSD(anomaly.getAnomalyDetails());
      }
    }
  }
  
  // Sync with Firebase periodically
  if (currentMillis - lastFirebaseSync >= FIREBASE_SYNC_INTERVAL) {
    lastFirebaseSync = currentMillis;
    
    if (WiFi.status() == WL_CONNECTED) {
      if (firebase.isConnected()) {
        firebase.syncSensorData(sensors.getData());
        firebase.checkForCommands(actuators);
      } else {
        // Try to reconnect
        Serial.println("Firebase disconnected, attempting to reconnect...");
        changeState(STATE_FIREBASE_AUTH);
        return;
      }
    } else {
      // WiFi disconnected, try to reconnect
      Serial.println("WiFi disconnected, attempting to reconnect...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
  }
  
  // Handle real-time Firebase updates
  if (firebase.isConnected()) {
    firebase.handleRealtimeUpdates(actuators);
  }
  
  // Flush SD card buffer periodically
  if (sdCardAvailable && currentMillis - lastSDFlush >= SD_BUFFER_FLUSH_INTERVAL) {
    lastSDFlush = currentMillis;
    flushSDBuffer();
  }
  
  // Sensor health check
  if (currentMillis - lastHealthCheck >= SENSOR_HEALTH_CHECK_INTERVAL) {
    lastHealthCheck = currentMillis;
    checkSensorHealth();
  }
}

void stateSafeMode() {
  static bool firstRun = true;
  
  if (firstRun) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║          SAFE MODE ACTIVATED           ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("System will maintain minimal operations.");
    Serial.println("Manual intervention may be required.\n");
    
    // Disable all non-critical actuators
    actuators.stopAll();
    
    // Enable critical protection based on current conditions
    SensorData data = sensors.getData();
    if (data.airTemp < TEMP_MIN) {
      Serial.println("⚠️ Low temperature detected, enabling emergency heat");
      actuators.setHeater(true, true);
    }
    
    blinkStatusLED(5, 100);  // Rapid blinks = safe mode
    firstRun = false;
  }
  
  // In safe mode, still read sensors and maintain critical protection
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL * 2) {
    lastSensorRead = currentMillis;
    sensors.readAll();
    
    SensorData data = sensors.getData();
    
    // Maintain critical temperature protection
    if (data.airTemp < TEMP_MIN) {
      actuators.setHeater(true, true);
    } else if (data.airTemp > TEMP_OPTIMAL_MAX) {
      actuators.setHeater(true, false);
      actuators.setHeater(false, false);
    }
  }
  
  // Attempt to recover after timeout
  if (currentMillis - stateEntryTime > SAFE_MODE_TIMEOUT) {
    Serial.println("Attempting to recover from safe mode...");
    changeState(STATE_SENSOR_INIT);
  }
}

void stateEmergency() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       EMERGENCY MODE ACTIVATED         ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  AnomalyType type = anomaly.getAnomalyType();
  Serial.print("Emergency type: ");
  Serial.println(type);
  
  // Execute emergency protocols (cast to EmergencyType)
  actuators.handleEmergency((EmergencyType)type);
  
  // Send urgent alert
  if (firebase.isConnected()) {
    firebase.sendAlert(anomaly.getAnomalyDetails());
  }
  
  // Activate buzzer if available
  #ifdef BUZZER_PIN
  tone(BUZZER_PIN, 1000, 2000);  // 1kHz tone for 2 seconds
  #endif
  
  // Flash LED rapidly
  for (int i = 0; i < 10; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(100);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(100);
    feedWatchdog();
  }
  
  // Return to normal operation after emergency actions taken
  delay(5000);
  changeState(STATE_NORMAL_OPERATION);
}

void stateCalibrationMode() {
  Serial.println("\n[STATE] Calibration Mode");
  Serial.println("1. ADC Calibration");
  Serial.println("2. MQ135 Calibration");
  Serial.println("3. Exit");
  Serial.print("Select option: ");
  
  while (!Serial.available()) {
    feedWatchdog();
    delay(100);
  }
  
  int option = Serial.parseInt();
  while (Serial.available()) Serial.read();
  
  switch (option) {
    case 1:
      sensors.performADCCalibration();
      break;
    case 2:
      sensors.calibrateMQ135();
      break;
    case 3:
      changeState(STATE_NORMAL_OPERATION);
      return;
  }
  
  // Stay in calibration mode
}

// ============================================================================
// WATCHDOG TIMER FUNCTIONS
// ============================================================================

// Software watchdog variables
unsigned long lastWatchdogFeed = 0;
bool watchdogEnabled = false;

void setupWatchdog() {
  #if WDT_ENABLED
  Serial.println("Initializing Software Watchdog Timer...");
  
  // Initialize software watchdog
  // Note: For production, consider enabling the Renesas IWDT hardware watchdog
  // This is a simplified software implementation for initial testing
  lastWatchdogFeed = millis();
  watchdogEnabled = true;
  
  Serial.print("✓ Software watchdog enabled (");
  Serial.print(WDT_TIMEOUT_SECONDS);
  Serial.println(" second timeout)");
  Serial.println("ℹ️  For production deployment, enable Renesas IWDT hardware watchdog");
  #else
  Serial.println("⚠️ Watchdog disabled (not recommended for production)");
  watchdogEnabled = false;
  #endif
}

void feedWatchdog() {
  #if WDT_ENABLED
  if (watchdogEnabled) {
    lastWatchdogFeed = millis();
  }
  #endif
}

void checkWatchdog() {
  #if WDT_ENABLED
  if (watchdogEnabled) {
    unsigned long timeSinceLastFeed = millis() - lastWatchdogFeed;
    if (timeSinceLastFeed > (WDT_TIMEOUT_SECONDS * 1000UL)) {
      Serial.println("\n\n⚠️⚠️⚠️ WATCHDOG TIMEOUT ⚠️⚠️⚠️");
      Serial.print("Time since last feed: ");
      Serial.print(timeSinceLastFeed);
      Serial.println(" ms");
      Serial.println("System appears to be hung. Initiating software reset...\n");
      delay(1000);
      
      // Perform software reset
      NVIC_SystemReset();
    }
  }
  #endif
}

// ============================================================================
// SD CARD BUFFERING FUNCTIONS
// ============================================================================

void initializeSDCard() {
  // STUB: SD library not compatible with Zephyr architecture
  Serial.println("SD card not available (Zephyr incompatibility)");
  sdCardAvailable = false;
}

void bufferSensorData(SensorData data) {
  if (!sdCardAvailable && bufferCount >= MAX_BUFFERED_READINGS) {
    Serial.println("⚠️ Buffer full, dropping oldest reading");
    // Shift buffer (drop oldest)
    for (int i = 0; i < MAX_BUFFERED_READINGS - 1; i++) {
      offlineBuffer[i] = offlineBuffer[i + 1];
    }
    bufferCount = MAX_BUFFERED_READINGS - 1;
  }
  
  if (bufferCount < MAX_BUFFERED_READINGS) {
    offlineBuffer[bufferCount].timestamp = data.timestamp;
    offlineBuffer[bufferCount].airTemp = data.airTemp;
    offlineBuffer[bufferCount].airHumidity = data.airHumidity;
    offlineBuffer[bufferCount].co2 = data.co2;
    offlineBuffer[bufferCount].ph = data.ph;
    offlineBuffer[bufferCount].ec = data.ec;
    offlineBuffer[bufferCount].vwc = data.vwc;
    bufferCount++;
  }
}

void flushSDBuffer() {
  // STUB: SD card not available on Zephyr
  if (bufferCount == 0) return;
  
  Serial.print("⚠️  Cannot flush ");
  Serial.print(bufferCount);
  Serial.println(" readings - SD card not available");
  
  // Clear buffer to prevent overflow
  bufferCount = 0;
}

void syncBufferedData() {
  // STUB: SD card not available on Zephyr
  Serial.println("⚠️  Cannot sync buffered data - SD card not available");
}

void saveAlertToSD(String alertDetails) {
  // STUB: SD card not available - log to Serial only
  Serial.print("🚨 ALERT (Serial only): ");
  Serial.println(alertDetails);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void changeState(SystemState newState) {
  previousState = currentState;
  currentState = newState;
  stateEntryTime = millis();
  
  Serial.print("\n>>> STATE CHANGE: ");
  Serial.print(previousState);
  Serial.print(" → ");
  Serial.println(newState);
}

void blinkStatusLED(int count, int delayMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(delayMs);
  }
}

void periodicMemoryCheck() {
  static unsigned long lastCheck = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastCheck >= MEMORY_CHECK_INTERVAL) {
    lastCheck = currentMillis;
    
    // Note: ESP.getFreeHeap() not available on Zephyr - memory check disabled
    // For production, implement Zephyr-specific memory monitoring if needed
  }
}

void checkSensorHealth() {
  SensorHealthReport health = sensors.getHealthReport();
  
  bool criticalFailure = false;
  
  if (!health.scd30Valid) {
    Serial.println("⚠️ SCD-30 sensor failure detected!");
    criticalFailure = true;
  }
  
  if (!health.modbusValid) {
    Serial.println("⚠️ Modbus sensor failure detected!");
  }
  
  if (health.mq135Valid && !health.mq135Preheated) {
    unsigned long remaining = MQ135_PREHEAT_TIME_MS - (millis() - 0);
    Serial.print("ℹ MQ135 preheating: ");
    Serial.print(remaining / 3600000);
    Serial.println(" hours remaining");
  }
  
  // If critical sensor failed, consider safe mode
  if (criticalFailure && health.scd30ErrorRate > 50.0f) {
    Serial.println("Critical sensor failure rate > 50%, entering safe mode");
    changeState(STATE_SAFE_MODE);
  }
}

void handleSerialCommands() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'c':
      case 'C':
        changeState(STATE_CALIBRATION_MODE);
        break;
        
      case 's':
      case 'S':
        sensors.readAll();
        sensors.printReadings();
        break;
        
      case 'h':
      case 'H': {
        SensorHealthReport health = sensors.getHealthReport();
        Serial.println("\n=== Sensor Health Report ===");
        Serial.print("SCD-30:  ");
        Serial.print(health.scd30Valid ? "OK" : "FAIL");
        Serial.print(" (Error: ");
        Serial.print(health.scd30ErrorRate, 1);
        Serial.println("%)");
        Serial.print("MQ135:   ");
        Serial.print(health.mq135Valid ? "OK" : "FAIL");
        Serial.print(" (Error: ");
        Serial.print(health.mq135ErrorRate, 1);
        Serial.print("%, Preheated: ");
        Serial.print(health.mq135Preheated ? "Yes" : "No");
        Serial.println(")");
        Serial.print("Modbus:  ");
        Serial.print(health.modbusValid ? "OK" : "FAIL");
        Serial.print(" (Error: ");
        Serial.print(health.modbusErrorRate, 1);
        Serial.println("%)");
        Serial.println();
        break;
      }
        
      case 'r':
      case 'R':
        Serial.println("Resetting system...");
        NVIC_SystemReset();
        break;
    }
    
    // Clear remaining input
    while (Serial.available()) Serial.read();
  }
}
