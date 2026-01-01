/**
 * GreenOS - ESP32-WROOM-32E Main Firmware
 * 
 * Hardware: ESP32-WROOM-32E (Dual-core Xtensa LX6 @ 240MHz)
 * Features:
 * - FreeRTOS multi-core task management
 * - Hardware Watchdog Timer (WDT) for auto-recovery
 * - WiFi with auto-reconnection and WiFiManager fallback
 * - Firebase real-time sync with offline buffering
 * - SPIFFS/SD card for persistent storage
 * - OTA firmware updates
 * - Deep sleep support for power saving
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <time.h>

#include "config.h"
#include "sensor_manager.h"
#include "actuator_manager.h"
#include "network_manager.h"
#include "data_logger.h"

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

SensorManager sensors;
ActuatorManager actuators;
NetworkManager network;
DataLogger dataLogger;
Preferences preferences;

// ============================================================================
// SYSTEM STATE MACHINE
// ============================================================================

volatile SystemState currentState = STATE_BOOT;
volatile SystemState previousState = STATE_BOOT;
unsigned long stateEntryTime = 0;
uint8_t bootFailCount = 0;

// ============================================================================
// FREERTOS TASK HANDLES
// ============================================================================

TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;
TaskHandle_t actuatorTaskHandle = NULL;

// ============================================================================
// SYNCHRONIZATION PRIMITIVES
// ============================================================================

SemaphoreHandle_t sensorDataMutex = NULL;
SemaphoreHandle_t stateMutex = NULL;
QueueHandle_t alertQueue = NULL;

// ============================================================================
// SHARED SENSOR DATA (Protected by mutex)
// ============================================================================

SensorData latestSensorData;
SensorHealthReport sensorHealth;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

void sensorTask(void* parameter);
void networkTask(void* parameter);
void actuatorTask(void* parameter);
void transitionState(SystemState newState);
void handleStateTransition();
void setupWatchdog();
void blinkStatusLED(int count, int delayMs);
void printSystemInfo();
void checkEmergencyConditions();

// ============================================================================
// SETUP - INITIALIZATION
// ============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);  // Allow serial to stabilize
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════════╗");
  Serial.println("║   GreenOS - Intelligent Greenhouse         ║");
  Serial.println("║   ESP32-WROOM-32E Firmware v2.0            ║");
  Serial.println("╚════════════════════════════════════════════╝");
  Serial.println();
  
  printSystemInfo();
  
  // Initialize status LED
  pinMode(STATUS_LED_PIN, OUTPUT);
  blinkStatusLED(3, 200);  // 3 quick blinks = boot
  
  // Initialize SPIFFS for data logging
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] SPIFFS mount failed!");
  } else {
    Serial.printf("[OK] SPIFFS mounted: %d bytes used / %d bytes total\n",
                  SPIFFS.usedBytes(), SPIFFS.totalBytes());
  }
  
  // Initialize NVS preferences
  preferences.begin(NVS_NAMESPACE, false);
  
  // Create synchronization primitives
  sensorDataMutex = xSemaphoreCreateMutex();
  stateMutex = xSemaphoreCreateMutex();
  alertQueue = xQueueCreate(10, sizeof(AlertPriority));
  
  if (sensorDataMutex == NULL || stateMutex == NULL || alertQueue == NULL) {
    Serial.println("[ERROR] Failed to create synchronization primitives!");
    transitionState(STATE_SAFE_MODE);
  }
  
  // Initialize hardware watchdog
  setupWatchdog();
  
  // Initialize subsystems
  Serial.println("\n[INIT] Initializing subsystems...");
  
  sensors.init();
  actuators.init();
  network.init();
  dataLogger.init();
  
  // Create FreeRTOS tasks on appropriate cores
  Serial.println("[INIT] Creating FreeRTOS tasks...");
  
  xTaskCreatePinnedToCore(
    sensorTask,
    "SensorTask",
    SENSOR_TASK_STACK_SIZE,
    NULL,
    SENSOR_TASK_PRIORITY,
    &sensorTaskHandle,
    SENSOR_TASK_CORE
  );
  
  xTaskCreatePinnedToCore(
    networkTask,
    "NetworkTask",
    NETWORK_TASK_STACK_SIZE,
    NULL,
    NETWORK_TASK_PRIORITY,
    &networkTaskHandle,
    NETWORK_TASK_CORE
  );
  
  xTaskCreatePinnedToCore(
    actuatorTask,
    "ActuatorTask",
    ACTUATOR_TASK_STACK_SIZE,
    NULL,
    ACTUATOR_TASK_PRIORITY,
    &actuatorTaskHandle,
    ACTUATOR_TASK_CORE
  );
  
  // Transition to WiFi connect state
  transitionState(STATE_WIFI_CONNECT);
  
  Serial.println("\n[OK] GreenOS initialization complete!");
  Serial.println("════════════════════════════════════════════════\n");
}

// ============================================================================
// MAIN LOOP (Minimal - most work done in FreeRTOS tasks)
// ============================================================================

void loop() {
  // Feed the watchdog
  esp_task_wdt_reset();
  
  // Handle state transitions
  handleStateTransition();
  
  // Check for emergency conditions
  checkEmergencyConditions();
  
  // Status LED heartbeat
  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat > 1000) {
    digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
    lastHeartbeat = millis();
  }
  
  // Yield to other tasks
  vTaskDelay(pdMS_TO_TICKS(100));
}

// ============================================================================
// FREERTOS TASKS
// ============================================================================

/**
 * Sensor Task - Runs on Core 1
 * Reads all sensors at defined intervals
 */
void sensorTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(SENSOR_READ_INTERVAL);
  
  Serial.println("[TASK] Sensor task started on Core " + String(xPortGetCoreID()));
  
  while (true) {
    // Wait for the next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    
    // Only read sensors in appropriate states
    if (currentState == STATE_NORMAL_OPERATION || 
        currentState == STATE_SAFE_MODE ||
        currentState == STATE_EMERGENCY) {
      
      // Read all sensors
      sensors.readAll();
      
      // Update shared data with mutex protection
      if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        latestSensorData = sensors.getData();
        sensorHealth = sensors.getHealthReport();
        xSemaphoreGive(sensorDataMutex);
      }
      
      // Print readings to serial (debug)
      sensors.printReadings();
    }
    
    // Feed task watchdog
    esp_task_wdt_reset();
  }
}

/**
 * Network Task - Runs on Core 0
 * Handles WiFi, Firebase sync, and OTA updates
 */
void networkTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // Check every second
  
  Serial.println("[TASK] Network task started on Core " + String(xPortGetCoreID()));
  
  unsigned long lastFirebaseSync = 0;
  unsigned long lastNTPSync = 0;
  
  while (true) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    
    // Maintain WiFi connection
    network.maintainConnection();
    
    // Handle state-specific network operations
    switch (currentState) {
      case STATE_WIFI_CONNECT:
        if (network.isConnected()) {
          transitionState(STATE_SENSOR_INIT);
        }
        break;
        
      case STATE_SENSOR_INIT:
        // Wait for sensors to initialize, then authenticate
        if (sensors.isInitialized()) {
          transitionState(STATE_FIREBASE_AUTH);
        }
        break;
        
      case STATE_FIREBASE_AUTH:
        if (network.authenticateFirebase()) {
          transitionState(STATE_NORMAL_OPERATION);
        }
        break;
        
      case STATE_NORMAL_OPERATION:
        // Sync to Firebase periodically
        if (millis() - lastFirebaseSync > FIREBASE_SYNC_INTERVAL) {
          if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            network.syncSensorData(latestSensorData);
            xSemaphoreGive(sensorDataMutex);
          }
          lastFirebaseSync = millis();
        }
        
        // Sync NTP time periodically
        if (millis() - lastNTPSync > NTP_SYNC_INTERVAL) {
          network.syncTime();
          lastNTPSync = millis();
        }
        
        // Check for remote commands
        network.checkForCommands(actuators);
        
        // Handle OTA updates
        network.handleOTA();
        break;
        
      case STATE_SAFE_MODE:
      case STATE_EMERGENCY:
        // Still try to send alerts even in emergency
        network.sendPendingAlerts();
        break;
        
      default:
        break;
    }
    
    esp_task_wdt_reset();
  }
}

/**
 * Actuator Task - Runs on Core 1
 * Controls relays and handles emergency responses
 */
void actuatorTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(500);  // 500ms response time
  
  Serial.println("[TASK] Actuator task started on Core " + String(xPortGetCoreID()));
  
  while (true) {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    
    // Process alert queue for emergency responses
    AlertPriority alert;
    if (xQueueReceive(alertQueue, &alert, 0) == pdTRUE) {
      if (alert == PRIORITY_ULTRA) {
        // Handle emergency
        if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
          // Determine emergency type from sensor data
          if (latestSensorData.airTemp < TEMP_MIN) {
            actuators.handleEmergency(EMERGENCY_LOW_TEMP);
          } else if (latestSensorData.airTemp > TEMP_MAX) {
            actuators.handleEmergency(EMERGENCY_HIGH_TEMP);
          }
          xSemaphoreGive(sensorDataMutex);
        }
      }
    }
    
    // Update actuator status
    actuators.update();
    
    esp_task_wdt_reset();
  }
}

// ============================================================================
// STATE MACHINE FUNCTIONS
// ============================================================================

void transitionState(SystemState newState) {
  if (xSemaphoreTake(stateMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    if (newState != currentState) {
      previousState = currentState;
      currentState = newState;
      stateEntryTime = millis();
      
      Serial.printf("[STATE] %d -> %d\n", previousState, newState);
    }
    xSemaphoreGive(stateMutex);
  }
}

void handleStateTransition() {
  // State timeout handling
  unsigned long stateTime = millis() - stateEntryTime;
  
  switch (currentState) {
    case STATE_WIFI_CONNECT:
      if (stateTime > WIFI_CONNECTION_TIMEOUT * 3) {
        Serial.println("[WARN] WiFi connection timeout, entering safe mode");
        transitionState(STATE_SAFE_MODE);
      }
      break;
      
    case STATE_FIREBASE_AUTH:
      if (stateTime > 30000) {  // 30 second timeout
        Serial.println("[WARN] Firebase auth timeout, operating in local mode");
        transitionState(STATE_NORMAL_OPERATION);
      }
      break;
      
    case STATE_SAFE_MODE:
      if (stateTime > SAFE_MODE_TIMEOUT) {
        Serial.println("[INFO] Safe mode timeout, attempting recovery...");
        transitionState(STATE_WIFI_CONNECT);
      }
      break;
      
    default:
      break;
  }
}

void checkEmergencyConditions() {
  if (currentState == STATE_BOOT || currentState == STATE_SENSOR_INIT) {
    return;  // Skip during initialization
  }
  
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    bool emergency = false;
    
    // Check for critical temperature conditions
    if (latestSensorData.airTemp < TEMP_MIN || latestSensorData.airTemp > TEMP_MAX) {
      emergency = true;
      AlertPriority alert = PRIORITY_ULTRA;
      xQueueSend(alertQueue, &alert, 0);
    }
    
    // Check for dangerous CO2 levels
    if (latestSensorData.co2 > CO2_DANGER) {
      emergency = true;
      AlertPriority alert = PRIORITY_ULTRA;
      xQueueSend(alertQueue, &alert, 0);
    }
    
    if (emergency && currentState != STATE_EMERGENCY) {
      transitionState(STATE_EMERGENCY);
    }
    
    xSemaphoreGive(sensorDataMutex);
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

void setupWatchdog() {
  if (WDT_ENABLED) {
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true);  // Enable panic on timeout
    esp_task_wdt_add(NULL);  // Add current task
    Serial.printf("[OK] Watchdog timer enabled: %d seconds\n", WDT_TIMEOUT_SECONDS);
  }
}

void blinkStatusLED(int count, int delayMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(delayMs);
  }
}

void printSystemInfo() {
  Serial.println("System Information:");
  Serial.printf("  Chip Model: %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("  CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("  Cores: %d\n", ESP.getChipCores());
  Serial.printf("  Flash Size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("  Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  SDK Version: %s\n", ESP.getSdkVersion());
  Serial.printf("  Firmware: %s (%s)\n", GREENOS_VERSION, GREENOS_BUILD_DATE);
  Serial.println();
}
