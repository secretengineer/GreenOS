/**
 * GreenOS - Firestore Triggered Functions
 * Functions that respond to database changes
 */

const admin = require('firebase-admin');
const db = admin.firestore();

/**
 * Process incoming sensor data
 * Check thresholds and trigger alerts if needed
 */
exports.processSensorData = async (snap, context) => {
  const data = snap.data();
  const { greenhouseId } = context.params;
  
  console.log(`Processing sensor data for greenhouse: ${greenhouseId}`);
  
  try {
    // Get greenhouse configuration
    const configDoc = await db
      .collection('greenhouses')
      .doc(greenhouseId)
      .get();
    
    if (!configDoc.exists) {
      console.error(`Greenhouse ${greenhouseId} not found`);
      return null;
    }
    
    const config = configDoc.data();
    const thresholds = config.thresholds || {};
    
    // Check for threshold violations
    const alerts = [];
    
    // Temperature checks
    if (data.airTemp !== undefined) {
      if (data.airTemp < thresholds.tempMin) {
        alerts.push({
          type: 'ULTRA_HIGH',
          severity: 'critical',
          message: `Low temperature alert: ${data.airTemp}°C`,
          sensor: 'airTemp',
          value: data.airTemp,
          threshold: thresholds.tempMin
        });
      } else if (data.airTemp > thresholds.tempMax) {
        alerts.push({
          type: 'HIGH',
          severity: 'high',
          message: `High temperature alert: ${data.airTemp}°C`,
          sensor: 'airTemp',
          value: data.airTemp,
          threshold: thresholds.tempMax
        });
      }
    }
    
    // Humidity checks
    if (data.airHumidity !== undefined) {
      if (data.airHumidity < thresholds.humidityMin) {
        alerts.push({
          type: 'MEDIUM',
          severity: 'medium',
          message: `Low humidity: ${data.airHumidity}%`,
          sensor: 'airHumidity',
          value: data.airHumidity,
          threshold: thresholds.humidityMin
        });
      }
    }
    
    // Motion detection (security)
    if (data.motionDetected && isOffHours()) {
      alerts.push({
        type: 'HIGH',
        severity: 'high',
        message: 'Motion detected during off-hours',
        sensor: 'motion',
        value: true,
        timestamp: data.timestamp
      });
    }
    
    // Create alert documents
    if (alerts.length > 0) {
      const batch = db.batch();
      
      alerts.forEach(alert => {
        const alertRef = db
          .collection('greenhouses')
          .doc(greenhouseId)
          .collection('alerts')
          .doc();
        
        batch.set(alertRef, {
          ...alert,
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
          acknowledged: false
        });
      });
      
      await batch.commit();
      console.log(`Created ${alerts.length} alerts`);
    }
    
    return { processed: true, alertsCreated: alerts.length };
    
  } catch (error) {
    console.error('Error processing sensor data:', error);
    throw error;
  }
};

/**
 * Send push notification when alert is created
 */
exports.sendAlertNotification = async (snap, context) => {
  const alert = snap.data();
  const { greenhouseId } = context.params;
  
  console.log(`Sending notification for alert: ${alert.type}`);
  
  try {
    // Get user tokens for this greenhouse
    const greenhouseDoc = await db
      .collection('greenhouses')
      .doc(greenhouseId)
      .get();
    
    if (!greenhouseDoc.exists) {
      console.error(`Greenhouse ${greenhouseId} not found`);
      return null;
    }
    
    const greenhouse = greenhouseDoc.data();
    const userTokens = greenhouse.userTokens || [];
    
    if (userTokens.length === 0) {
      console.log('No user tokens found for notifications');
      return null;
    }
    
    // Determine notification priority based on severity
    const priority = alert.severity === 'critical' ? 'high' : 'normal';
    
    // Create FCM message
    const message = {
      notification: {
        title: `GreenOS Alert: ${alert.type}`,
        body: alert.message,
      },
      data: {
        greenhouseId: greenhouseId,
        alertId: snap.id,
        severity: alert.severity,
        type: alert.type
      },
      android: {
        priority: priority,
        notification: {
          sound: alert.severity === 'critical' ? 'urgent' : 'default',
          channelId: `greenos_${alert.severity}`
        }
      },
      apns: {
        payload: {
          aps: {
            sound: alert.severity === 'critical' ? 'urgent.wav' : 'default',
            badge: 1
          }
        }
      },
      tokens: userTokens
    };
    
    // Send multicast message
    const response = await admin.messaging().sendMulticast(message);
    
    console.log(`Notification sent. Success: ${response.successCount}, Failed: ${response.failureCount}`);
    
    return { success: response.successCount, failed: response.failureCount };
    
  } catch (error) {
    console.error('Error sending notification:', error);
    throw error;
  }
};

/**
 * Log user commands for audit trail
 */
exports.logUserCommand = async (snap, context) => {
  const command = snap.data();
  const { greenhouseId, commandId } = context.params;
  
  console.log(`Logging command: ${commandId}`);
  
  try {
    // Create audit log entry
    await db
      .collection('greenhouses')
      .doc(greenhouseId)
      .collection('logs')
      .add({
        eventType: 'user_command',
        commandId: commandId,
        command: command.action,
        target: command.target,
        userId: command.userId,
        timestamp: admin.firestore.FieldValue.serverTimestamp(),
        details: command
      });
    
    return { logged: true };
    
  } catch (error) {
    console.error('Error logging command:', error);
    throw error;
  }
};

/**
 * Helper function to check if current time is off-hours
 */
function isOffHours() {
  const now = new Date();
  const hour = now.getHours();
  // Off-hours: 10 PM to 6 AM
  return hour >= 22 || hour < 6;
}
