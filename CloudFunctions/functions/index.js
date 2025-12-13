/**
 * GreenOS Cloud Functions
 * Main entry point for all Firebase Cloud Functions
 */

const functions = require('firebase-functions');
const admin = require('firebase-admin');

admin.initializeApp();

// Import function modules
const triggers = require('./triggers');
const api = require('./api');
const scheduled = require('./scheduled');

// Configure region (v5+ uses options object)
const region = 'us-west3';

// ============================================================================
// FIRESTORE TRIGGERS
// ============================================================================

/**
 * Triggered when new sensor data is written to Firestore
 * Performs real-time analysis and sends alerts if needed
 */
exports.onSensorData = functions
  .region(region)
  .firestore
  .document('greenhouses/{greenhouseId}/sensors/{sensorId}')
  .onCreate(triggers.processSensorData);

/**
 * Triggered when an alert is created
 * Sends push notifications via FCM
 */
exports.onAlert = functions
  .region(region)
  .firestore
  .document('greenhouses/{greenhouseId}/alerts/{alertId}')
  .onCreate(triggers.sendAlertNotification);

/**
 * Triggered when a user command is created
 * Validates and logs the command
 */
exports.onUserCommand = functions
  .region(region)
  .firestore
  .document('greenhouses/{greenhouseId}/commands/{commandId}')
  .onCreate(triggers.logUserCommand);

// ============================================================================
// HTTP API ENDPOINTS
// ============================================================================

/**
 * Generate a custom authentication token for a device
 */
exports.generateAuthToken = functions
  .region(region)
  .https.onCall(api.generateAuthToken);


/**
 * Get historical sensor data from BigQuery
 */
exports.getHistoricalData = functions
  .region(region)
  .https.onCall(api.getHistoricalData);

/**
 * Get analytics and statistics
 */
exports.getAnalytics = functions
  .region(region)
  .https.onCall(api.getAnalytics);

/**
 * Update greenhouse configuration
 */
exports.updateConfig = functions
  .region(region)
  .https.onCall(api.updateConfig);

/**
 * Get current greenhouse status
 */
exports.getGreenhouseStatus = functions
  .region(region)
  .https.onCall(api.getGreenhouseStatus);

// ============================================================================
// SCHEDULED FUNCTIONS
// ============================================================================

/**
 * Export sensor data to BigQuery every hour
 */
exports.exportToBigQuery = functions
  .region(region)
  .pubsub
  .schedule('every 1 hours')
  .onRun(scheduled.exportToBigQuery);

/**
 * Fetch external weather data every 30 minutes
 */
exports.fetchWeatherData = functions
  .region(region)
  .pubsub
  .schedule('every 30 minutes')
  .onRun(scheduled.fetchWeatherData);

/**
 * Generate daily summary reports
 */
exports.generateDailySummary = functions
  .region(region)
  .pubsub
  .schedule('every day 00:00')
  .timeZone('America/Denver')
  .onRun(scheduled.generateDailySummary);

/**
 * Check device health and connectivity
 */
exports.checkDeviceHealth = functions
  .region(region)
  .pubsub
  .schedule('every 5 minutes')
  .onRun(scheduled.checkDeviceHealth);
