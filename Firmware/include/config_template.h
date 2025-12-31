/**
 * GreenOS - ESP32-WROOM-32E Configuration File
 * 
 * Hardware: ESP32-WROOM-32E (Dual-core Xtensa LX6 @ 240MHz)
 * Features: WiFi 802.11 b/g/n, Bluetooth 4.2, 520KB SRAM, 4MB Flash
 * All GPIO pins are 3.3V logic levels
 * 
 * IMPORTANT: Copy config_template.h to config.h and update with your credentials
 * DO NOT commit config.h with real credentials to version control
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PLATFORM IDENTIFICATION
// ============================================================================

#define GREENOS_VERSION "2.0.0-dev"
#define GREENOS_PLATFORM "ESP32-WROOM-32E"
#define GREENOS_BUILD_DATE __DATE__ " " __TIME__

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// WiFi Configuration (Primary connectivity)
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_RECONNECT_INTERVAL 30000   // 30 seconds between reconnect attempts
#define WIFI_CONNECTION_TIMEOUT 15000   // 15 seconds to establish connection
#define WIFI_HOSTNAME "greenos"

// WiFi Manager fallback AP
#define WIFI_AP_SSID "GreenOS-Setup"
#define WIFI_AP_PASSWORD "greenhouse123"

// Firebase Configuration
#define FIREBASE_HOST "greenos-24311.firebaseapp.com"
#define FIREBASE_API_KEY "YOUR_FIREBASE_API_KEY"
#define FIREBASE_PROJECT_ID "greenos-24311"

// NTP Time Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_GMT_OFFSET_SEC -25200       // UTC-7 (Mountain Time)
#define NTP_DAYLIGHT_OFFSET_SEC 3600    // DST offset

// ============================================================================
// GREENHOUSE IDENTIFICATION
// ============================================================================

#define GREENHOUSE_ID "Mayfair_Greenhouse"
#define GREENHOUSE_LOCATION "Denver, CO"
#define GREENHOUSE_VOLUME_CUFT 325.0    // Approximate volume in cubic feet
#define GREENHOUSE_ALTITUDE_FT 5280     // Denver elevation in feet
#define GREENHOUSE_ALTITUDE_M 1609      // Denver elevation in meters

// ============================================================================
// SENSOR THRESHOLDS (Default values - can be overridden by Firebase)
// ============================================================================

// Temperature Thresholds (°C)
#define TEMP_MIN 10.0                   // Critical low temperature (triggers emergency heat)
#define TEMP_MAX 35.0                   // Critical high temperature (triggers cooling)
#define TEMP_OPTIMAL_MIN 18.0           // Optimal range minimum
#define TEMP_OPTIMAL_MAX 24.0           // Optimal range maximum
#define TEMP_FROST_ALERT 4.0            // Frost warning threshold
#define TEMP_RAPID_DROP_RATE 2.0        // °C per 10 minutes = anomaly

// Humidity Thresholds (%)
#define HUMIDITY_MIN 40.0               // Minimum acceptable humidity
#define HUMIDITY_MAX 80.0               // Maximum acceptable humidity
#define HUMIDITY_OPTIMAL_MIN 50.0       // Optimal range minimum
#define HUMIDITY_OPTIMAL_MAX 70.0       // Optimal range maximum

// Soil Moisture Thresholds (%)
#define VWC_MIN 20.0                    // Minimum volumetric water content
#define VWC_MAX 60.0                    // Maximum volumetric water content
#define VWC_OPTIMAL_MIN 30.0            // Optimal range minimum
#define VWC_OPTIMAL_MAX 50.0            // Optimal range maximum

// CO2 Thresholds (ppm)
#define CO2_MIN 400.0                   // Minimum CO2 (outdoor ambient)
#define CO2_MAX 1500.0                  // Maximum CO2 for optimal growth
#define CO2_OPTIMAL_MIN 800.0           // Optimal range minimum
#define CO2_OPTIMAL_MAX 1200.0          // Optimal range maximum
#define CO2_DANGER 5000.0               // Dangerous CO2 level for humans

// pH and EC Thresholds
#define PH_MIN 5.5                      // Minimum pH
#define PH_MAX 7.5                      // Maximum pH
#define PH_OPTIMAL_MIN 6.0              // Optimal range minimum
#define PH_OPTIMAL_MAX 7.0              // Optimal range maximum
#define EC_MIN 0.5                      // Minimum EC (mS/cm)
#define EC_MAX 3.0                      // Maximum EC (mS/cm)
#define EC_OPTIMAL_MIN 1.0              // Optimal range minimum
#define EC_OPTIMAL_MAX 2.0              // Optimal range maximum

// ============================================================================
// HARDWARE PIN DEFINITIONS - ESP32-WROOM-32E (3.3V Logic)
// ============================================================================
// Note: ESP32 has flexible pin mapping. Avoid strapping pins for general I/O:
//   GPIO0, GPIO2, GPIO5, GPIO12, GPIO15 (used during boot)
// ============================================================================

// I2C Bus (SCD-30 CO2 Sensor, expandable)
#define I2C_SDA_PIN 21                  // Hardware I2C SDA
#define I2C_SCL_PIN 22                  // Hardware I2C SCL
#define I2C_FREQUENCY 100000            // 100kHz I2C clock
#define SCD30_I2C_ADDR 0x61             // SCD-30 I2C address

// UART2 for Modbus RS485 (Soil EC/pH Sensor)
#define MODBUS_RX_PIN 16                // UART2 RX
#define MODBUS_TX_PIN 17                // UART2 TX
#define MODBUS_DE_RE_PIN 4              // MAX485 Driver Enable / Receiver Enable
#define MODBUS_BAUD_RATE 4800           // Modbus baud rate (per sensor datasheet)
#define MODBUS_SLAVE_ID 0x01            // Default slave address

// Analog Sensors (12-bit ADC, 0-3.3V, 0-4095 range)
// Note: ESP32 ADC2 pins cannot be used when WiFi is active
// Use ADC1 pins only: GPIO32-39
#define MQ135_SENSOR_PIN 34             // MQ135 Air Quality (ADC1_CH6)
#define VWC_SENSOR_PIN 35               // Analog VWC sensor backup (ADC1_CH7)
#define MICROPHONE_PIN 32               // Sound level sensor (ADC1_CH4)
#define LIGHT_LEVEL_PIN 33              // Ambient light sensor (ADC1_CH5)

// Digital I/O
#define PIR_SENSOR_PIN 27               // Motion sensor (digital input)
#define UPS_STATUS_PIN 26               // UPS status monitoring
#define BUZZER_PIN 25                   // Alarm buzzer (DAC capable)
#define STATUS_LED_PIN 2                // Built-in LED (GPIO2)

// SPI Bus (SD Card)
#define SD_CS_PIN 5                     // SD card chip select
#define SD_MOSI_PIN 23                  // SPI MOSI
#define SD_MISO_PIN 19                  // SPI MISO
#define SD_SCK_PIN 18                   // SPI Clock

// Actuator Relay Pins (Active LOW optoisolated relays)
#define HEATER_PRIMARY_PIN 13           // Primary heater (1500W max)
#define HEATER_SECONDARY_PIN 14         // Secondary heater backup
#define FAN_EXHAUST_PIN 12              // Exhaust ventilation fan
#define FAN_CIRCULATION_PIN 15          // Internal circulation fan
#define PUMP_IRRIGATION_PIN 0           // Irrigation pump/valve (use with caution - strapping pin)
#define LIGHT_GROW_PIN 2                // Grow lights relay (shared with status LED)

// Alternative actuator pins (if strapping pins cause issues)
#define ALT_PUMP_IRRIGATION_PIN 27      // Alternative irrigation pin
#define ALT_LIGHT_GROW_PIN 26           // Alternative grow light pin

// ============================================================================
// ADC CALIBRATION SETTINGS
// ============================================================================

#define ADC_RESOLUTION 12               // ESP32 has 12-bit ADC
#define ADC_MAX_VALUE 4095              // 2^12 - 1
#define ADC_VREF_NOMINAL 3.3            // Nominal reference voltage
#define ADC_SAMPLES 64                  // Number of samples for multisampling
#define ADC_SAMPLE_DELAY_US 100         // Delay between samples (microseconds)

// ESP32 ADC attenuation settings
// ADC_ATTEN_DB_0   = 0-1.1V range
// ADC_ATTEN_DB_2_5 = 0-1.5V range
// ADC_ATTEN_DB_6   = 0-2.2V range
// ADC_ATTEN_DB_11  = 0-3.3V range (recommended for most sensors)
#define ADC_ATTENUATION ADC_ATTEN_DB_11

// ============================================================================
// PREFERENCES / NVS STORAGE (replaces EEPROM)
// ============================================================================

#define NVS_NAMESPACE "greenos"
#define NVS_KEY_ADC_CAL "adc_cal"
#define NVS_KEY_WIFI_CONFIG "wifi_cfg"
#define NVS_KEY_SENSOR_CAL "sensor_cal"
#define NVS_KEY_THRESHOLDS "thresholds"

// ============================================================================
// WATCHDOG TIMER CONFIGURATION
// ============================================================================

#define WDT_TIMEOUT_SECONDS 30          // Watchdog timeout (ESP32 supports longer timeouts)
#define WDT_ENABLED true                // Enable hardware watchdog
#define TASK_WDT_TIMEOUT_S 10           // Task watchdog timeout

// ============================================================================
// TIMING INTERVALS
// ============================================================================

#define SENSOR_READ_INTERVAL 5000       // 5 seconds - sensor polling rate
#define FIREBASE_SYNC_INTERVAL 60000    // 1 minute - cloud sync interval
#define ANOMALY_CHECK_INTERVAL 10000    // 10 seconds - anomaly detection rate
#define MODBUS_READ_INTERVAL 15000      // 15 seconds - Modbus sensor polling
#define SPIFFS_BUFFER_FLUSH_INTERVAL 300000 // 5 minutes - flush SPIFFS buffer
#define MEMORY_CHECK_INTERVAL 60000     // 1 minute - memory health check
#define SENSOR_HEALTH_CHECK_INTERVAL 30000  // 30 seconds - sensor validation
#define NTP_SYNC_INTERVAL 3600000       // 1 hour - NTP time sync
#define OTA_CHECK_INTERVAL 86400000     // 24 hours - OTA update check

// ============================================================================
// MQ135 AIR QUALITY SENSOR CONFIGURATION
// ============================================================================

#define MQ135_LOAD_RESISTOR 10000.0     // Load resistor (Ω)
#define MQ135_PREHEAT_TIME_MS 172800000 // 48 hours preheat time
#define MQ135_CLEAN_AIR_RATIO 3.6       // RS/R0 ratio in clean air

// ============================================================================
// SCD-30 CO2 SENSOR CONFIGURATION
// ============================================================================

#define SCD30_MEASUREMENT_INTERVAL 5    // Measurement interval in seconds
#define SCD30_AUTO_CALIBRATION true     // Enable automatic self-calibration
#define SCD30_TEMP_OFFSET 0.0           // Temperature offset calibration
#define SCD30_ALTITUDE_COMPENSATION true // Enable altitude compensation

// ============================================================================
// MODBUS SENSOR REGISTER MAP (S-Soil MT-02)
// ============================================================================

#define MODBUS_REG_MOISTURE 0x0000      // Soil moisture (0.1% resolution)
#define MODBUS_REG_TEMPERATURE 0x0001   // Soil temperature (0.1°C resolution)
#define MODBUS_REG_EC 0x0002            // Electrical conductivity (1 μS/cm)
#define MODBUS_REG_PH 0x0003            // pH value (0.01 pH resolution)
#define MODBUS_REG_NITROGEN 0x0004      // Nitrogen (mg/kg)
#define MODBUS_REG_PHOSPHORUS 0x0005    // Phosphorus (mg/kg)
#define MODBUS_REG_POTASSIUM 0x0006     // Potassium (mg/kg)

// ============================================================================
// ERROR HANDLING & SAFETY
// ============================================================================

#define MAX_SENSOR_ERRORS 5             // Max consecutive errors before flagging failure
#define SAFE_MODE_TIMEOUT 300000        // 5 minutes in safe mode before retry
#define MIN_FREE_HEAP_BYTES 50000       // Minimum free memory before warning (ESP32 has more RAM)
#define MAX_BUFFERED_READINGS 500       // Max readings to buffer when offline (SPIFFS)

// ============================================================================
// DUAL-CORE TASK CONFIGURATION
// ============================================================================

// ESP32 has two cores: Core 0 (Protocol CPU) and Core 1 (Application CPU)
#define SENSOR_TASK_CORE 1              // Run sensor tasks on App CPU
#define NETWORK_TASK_CORE 0             // Run network tasks on Protocol CPU
#define ACTUATOR_TASK_CORE 1            // Run actuator control on App CPU

#define SENSOR_TASK_PRIORITY 2          // Medium priority
#define NETWORK_TASK_PRIORITY 1         // Lower priority (WiFi has its own)
#define ACTUATOR_TASK_PRIORITY 3        // Higher priority for safety

#define SENSOR_TASK_STACK_SIZE 4096     // Stack size in bytes
#define NETWORK_TASK_STACK_SIZE 8192    // Larger stack for Firebase operations
#define ACTUATOR_TASK_STACK_SIZE 2048   // Smaller stack for relay control

// ============================================================================
// SYSTEM STATE DEFINITIONS
// ============================================================================

enum SystemState {
  STATE_BOOT,
  STATE_WIFI_CONNECT,
  STATE_SENSOR_INIT,
  STATE_FIREBASE_AUTH,
  STATE_NORMAL_OPERATION,
  STATE_SAFE_MODE,
  STATE_EMERGENCY,
  STATE_CALIBRATION_MODE,
  STATE_OTA_UPDATE,
  STATE_DEEP_SLEEP
};

enum AlertPriority {
  PRIORITY_LOW,         // Informational
  PRIORITY_MEDIUM,      // Warning - requires attention
  PRIORITY_HIGH,        // Critical - immediate action needed
  PRIORITY_ULTRA        // Emergency - catastrophic failure
};

// ============================================================================
// DEEP SLEEP CONFIGURATION (Power saving)
// ============================================================================

#define DEEP_SLEEP_ENABLED false        // Enable deep sleep mode
#define DEEP_SLEEP_DURATION_US 60000000 // 60 seconds deep sleep
#define WAKE_ON_EXT0_PIN GPIO_NUM_27    // Wake on PIR motion detection

// ============================================================================
// OTA UPDATE CONFIGURATION
// ============================================================================

#define OTA_ENABLED true
#define OTA_HOSTNAME "greenos"
#define OTA_PASSWORD "greenos_ota_password"
#define OTA_PORT 3232

#endif // CONFIG_H
