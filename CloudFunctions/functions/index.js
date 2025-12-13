/**
 * GreenOS Cloud Functions
 * Main entry point for all Firebase Cloud Functions
 */

const functions = require("firebase-functions");
const admin = require("firebase-admin");

admin.initializeApp();

// Import function modules
const triggers = require("./triggers");
const api = require("./api");
const scheduled = require("./scheduled");

// Define the region for all functions
const REGION = "us-west3";

// ============================================================================
// FIRESTORE TRIGGERS
// ============================================================================

/**
 * Triggered when new sensor data is written to Firestore
 * Performs real-time analysis and sends alerts if needed
 */
exports.onSensorData = functions.region(REGION).firestore
  .document("greenhouses/{greenhouseId}/sensors/{sensorId}")
  .onCreate(triggers.processSensorData);

/**
 * Triggered when an alert is created
 * Sends push notifications via FCM
 */
exports.onAlert = functions.region(REGION).firestore
  .document("greenhouses/{greenhouseId}/alerts/{alertId}")
  .onCreate(triggers.sendAlertNotification);

/**
 * Triggered when a user command is created
 * Validates and logs the command
 */
exports.onUserCommand = functions.region(REGION).firestore
  .document("greenhouses/{greenhouseId}/commands/{commandId}")
  .onCreate(triggers.logUserCommand);

// ============================================================================
// HTTP API ENDPOINTS
// ============================================================================

/**
 * Generate a custom authentication token for a device
 * Called by the Arduino device during initial setup
 */
exports.generateAuthToken = functions.region(REGION).https.onCall(api.generateAuthToken);

/**
 * Get historical sensor data from BigQuery
 * Called by the frontend for charts
 */
exports.getHistoricalData = functions.region(REGION).https.onCall(api.getHistoricalData);

/**
 * Get analytics data
 * Called by the frontend for the dashboard
 */
exports.getAnalytics = functions.region(REGION).https.onCall(api.getAnalytics);

/**
 * Update greenhouse configuration
 * Called by the frontend settings page
 */
exports.updateConfig = functions.region(REGION).https.onCall(api.updateConfig);

/**
 * Get greenhouse status
 * Called by the frontend dashboard
 */
exports.getGreenhouseStatus = functions.region(REGION).https.onCall(api.getGreenhouseStatus);

// ============================================================================
// SCHEDULED FUNCTIONS
// ============================================================================

/**
 * Export sensor data to BigQuery every hour
 */
exports.exportToBigQuery = functions.region(REGION).pubsub
  .schedule("every 1 hours")
  .onRun(scheduled.exportToBigQuery);

/**
 * Fetch weather data every 4 hours
 */
exports.fetchWeatherData = functions.region(REGION).pubsub
  .schedule("every 4 hours")
  .onRun(scheduled.fetchWeatherData);

/**
 * Generate daily summary report every day at midnight
 */
exports.generateDailySummary = functions.region(REGION).pubsub
  .schedule("every 24 hours")
  .onRun(scheduled.generateDailySummary);

/**
 * Check device health every 15 minutes
 */
exports.checkDeviceHealth = functions.region(REGION).pubsub
  .schedule("every 15 minutes")
  .onRun(scheduled.checkDeviceHealth);
