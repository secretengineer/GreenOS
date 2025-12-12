/**
 * GreenOS - Configuration File
 * 
 * IMPORTANT: Copy this file to config.h and update with your credentials
 * DO NOT commit config.h to version control
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Firebase Configuration
#define FIREBASE_HOST "YOUR_PROJECT_ID.firebaseio.com"
#define FIREBASE_AUTH "YOUR_FIREBASE_SECRET_OR_TOKEN"
#define API_KEY "YOUR_FIREBASE_API_KEY"

// Greenhouse Location
#define GREENHOUSE_ID "denver_greenhouse_01"
#define GREENHOUSE_LOCATION "Denver, CO"

// Sensor Thresholds (Default values - can be overridden by Firebase)
#define TEMP_MIN 10.0          // °C - Critical low temperature
#define TEMP_MAX 35.0          // °C - Critical high temperature
#define TEMP_OPTIMAL_MIN 18.0  // °C - Optimal range min
#define TEMP_OPTIMAL_MAX 24.0  // °C - Optimal range max

#define HUMIDITY_MIN 40.0      // % - Minimum humidity
#define HUMIDITY_MAX 80.0      // % - Maximum humidity

#define VWC_MIN 20.0           // % - Minimum soil moisture
#define VWC_MAX 60.0           // % - Maximum soil moisture

#define CO2_MIN 400.0          // ppm - Minimum CO2
#define CO2_MAX 1500.0         // ppm - Maximum CO2

// Pin Definitions
// Temperature & Humidity
#define DHT_PIN 4
#define DHT_TYPE DHT22

// Soil Sensors
#define VWC_SENSOR_PIN 34
#define PH_SENSOR_PIN 35
#define EC_SENSOR_PIN 32

// CO2 Sensor
#define CO2_RX_PIN 16
#define CO2_TX_PIN 17

// Motion & Security
#define PIR_SENSOR_PIN 25
#define MICROPHONE_PIN 33

// Actuator Relay Pins
#define HEATER_PRIMARY_PIN 26
#define HEATER_SECONDARY_PIN 27
#define FAN_EXHAUST_PIN 14
#define FAN_CIRCULATION_PIN 12
#define PUMP_IRRIGATION_PIN 13
#define LIGHT_GROW_PIN 15

// Power Monitoring
#define UPS_STATUS_PIN 23

#endif // CONFIG_H
