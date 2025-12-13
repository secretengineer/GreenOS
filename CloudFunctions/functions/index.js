/**
 * GreenOS Cloud Functions
 * Main entry point for all Firebase Cloud Functions
 */

const { onDocumentCreated } = require("firebase-functions/v2/firestore");
const { onCall } = require("firebase-functions/v2/https");
const { onSchedule } = require("firebase-functions/v2/scheduler");
const admin = require("firebase-admin");

admin.initializeApp();

// Import function modules
const triggers = require("./triggers");
const api = require("./api");
const scheduled = require("./scheduled");

// ============================================================================
// FIRESTORE TRIGGERS
// ============================================================================

/**
 * Triggered when new sensor data is written to Firestore
 * Performs real-time analysis and sends alerts if needed
 */
exports.onSensorData_v2 = onDocumentCreated(
  { document: "greenhouses/{greenhouseId}/sensors/{sensorId}", region: "us-west3" },
  triggers.processSensorData
);

/**
 * Triggered when an alert is created
 * Sends push notifications via FCM
 */
exports.onAlert_v2 = onDocumentCreated(
  { document: "greenhouses/{greenhouseId}/alerts/{alertId}", region: "us-west3" },
  triggers.sendAlertNotification
);

/**
 * Triggered when a user command is created
 * Validates and logs the command
 */
exports.onUserCommand_v2 = onDocumentCreated(
  { document: "greenhouses/{greenhouseId}/commands/{commandId}", region: "us-west3" },
  triggers.logUserCommand
);

// ============================================================================
// HTTP API ENDPOINTS
// ============================================================================

/**
 * Generate a custom authentication token for a device
 */
exports.generateAuthToken_v2 = onCall({ region: "us-west3" }, api.generateAuthToken);

/**
 * Get historical sensor data from BigQuery
 */
exports.getHistoricalData_v2 = onCall({ region: "us-west3" }, api.getHistoricalData);

/**
 * Get analytics and statistics
 */
exports.getAnalytics_v2 = onCall({ region: "us-west3" }, api.getAnalytics);

/**
 * Update greenhouse configuration
 */
exports.updateConfig_v2 = onCall({ region: "us-west3" }, api.updateConfig);

/**
 * Get current greenhouse status
 */
exports.getGreenhouseStatus_v2 = onCall({ region: "us-west3" }, api.getGreenhouseStatus);

// ============================================================================
// SCHEDULED FUNCTIONS
// ============================================================================

/**
 * Export sensor data to BigQuery every hour
 */
exports.exportToBigQuery_v2 = onSchedule(
  { schedule: "every 1 hours", region: "us-west3" },
  scheduled.exportToBigQuery
);

/**
 * Fetch external weather data every 30 minutes
 */
exports.fetchWeatherData_v2 = onSchedule(
  { schedule: "every 30 minutes", region: "us-west3" },
  scheduled.fetchWeatherData
);

/**
 * Generate daily summary reports
 */
exports.generateDailySummary_v2 = onSchedule(
  {
    schedule: "every day 00:00",
    timeZone: "America/Denver",
    region: "us-west3",
  },
  scheduled.generateDailySummary
);

/**
 * Check device health and connectivity
 */
exports.checkDeviceHealth_v2 = onSchedule(
  { schedule: "every 5 minutes", region: "us-west3" },
  scheduled.checkDeviceHealth
);
