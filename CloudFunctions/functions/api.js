
/**
 * GreenOS - API Functions
 * HTTP callable functions for frontend applications
 */

const admin = require('firebase-admin');
const functions = require('firebase-functions/v1');
const { BigQuery } = require('@google-cloud/bigquery');

// Lazy load Firestore and BigQuery to prevent deployment timeouts
let db;
let bigquery;

function getDb() {
  if (!db) {
    db = admin.firestore();
  }
  return db;
}

function getBigQuery() {
  if (!bigquery) {
    bigquery = new BigQuery();
  }
  return bigquery;
}

/**
 * Generate a custom authentication token for a device
 */
exports.generateAuthToken = async (data, context) => {
  const { deviceId, greenhouseId } = data;

  if (!deviceId) {
    throw new functions.https.HttpsError('invalid-argument', 'The function must be called with a "deviceId" argument.');
  }

  if (!greenhouseId) {
    throw new functions.https.HttpsError('invalid-argument', 'The function must be called with a "greenhouseId" argument.');
  }

  try {
    // Validate that the device is registered and belongs to the specified greenhouse
    const deviceRef = getDb().collection('devices').doc(deviceId);
    const deviceDoc = await deviceRef.get();
    
    if (!deviceDoc.exists) {
      console.warn(`Token request for unregistered device: ${deviceId}`);
      throw new functions.https.HttpsError('not-found', 'Device not registered');
    }
    
    const deviceData = deviceDoc.data();
    if (deviceData.greenhouseId !== greenhouseId) {
      console.warn(`Device ${deviceId} does not belong to greenhouse ${greenhouseId}`);
      throw new functions.https.HttpsError('permission-denied', 'Device not authorized for this greenhouse');
    }
    
    // Create custom token with greenhouse claim for security rules
    const customToken = await admin.auth().createCustomToken(deviceId, {
      greenhouseId: greenhouseId,
      isDevice: true
    });
    console.log(`Generated custom token for device: ${deviceId} (greenhouse: ${greenhouseId})`);

    return { token: customToken };
  } catch (error) {
    console.error('Error creating custom token:', error);
    throw new functions.https.HttpsError('internal', 'Unable to create custom token.');
  }
};


/**
 * Get historical sensor data from BigQuery
 */
exports.getHistoricalData = async (data, context) => {
  // Check authentication
  if (!context.auth) {
    throw new functions.https.HttpsError('unauthenticated', 'User must be authenticated');
  }
  
  const { greenhouseId, sensorType, startDate, endDate, aggregation = 'hourly' } = data;
  
  console.log(`Fetching historical data for ${greenhouseId}, sensor: ${sensorType}`);
  
  try {
    // Verify user has access to this greenhouse
    const greenhouseDoc = await getDb().collection('greenhouses').doc(greenhouseId).get();
    
    if (!greenhouseDoc.exists) {
      throw new functions.https.HttpsError('not-found', 'Greenhouse not found');
    }
    
    const greenhouse = greenhouseDoc.data();
    if (!greenhouse.users || !greenhouse.users.includes(context.auth.uid)) {
      throw new functions.https.HttpsError('permission-denied', 'Access denied');
    }
    
    // Query BigQuery
    let query;
    
    if (aggregation === 'raw') {
      query = `
        SELECT timestamp, sensor_type, value
        FROM \`greenos.sensor_data\`
        WHERE greenhouse_id = @greenhouseId
          AND sensor_type = @sensorType
          AND timestamp BETWEEN @startDate AND @endDate
        ORDER BY timestamp ASC
        LIMIT 10000
      `;
    } else {
      // Aggregated query
      const timeFormat = aggregation === 'hourly' 
        ? 'TIMESTAMP_TRUNC(timestamp, HOUR)'
        : 'TIMESTAMP_TRUNC(timestamp, DAY)';
      
      query = `
        SELECT 
          ${timeFormat} as time_bucket,
          sensor_type,
          AVG(value) as avg_value,
          MIN(value) as min_value,
          MAX(value) as max_value,
          COUNT(*) as count
        FROM \`greenos.sensor_data\`
        WHERE greenhouse_id = @greenhouseId
          AND sensor_type = @sensorType
          AND timestamp BETWEEN @startDate AND @endDate
        GROUP BY time_bucket, sensor_type
        ORDER BY time_bucket ASC
      `;
    }
    
    const options = {
      query: query,
      params: {
        greenhouseId: greenhouseId,
        sensorType: sensorType,
        startDate: startDate,
        endDate: endDate
      }
    };
    
    const [rows] = await getBigQuery().query(options);
    
    return {
      success: true,
      data: rows,
      count: rows.length
    };
    
  } catch (error) {
    console.error('Error fetching historical data:', error);
    throw new functions.https.HttpsError('internal', error.message);
  }
};

/**
 * Get analytics and statistics
 */
exports.getAnalytics = async (data, context) => {
  if (!context.auth) {
    throw new functions.https.HttpsError('unauthenticated', 'User must be authenticated');
  }
  
  const { greenhouseId, period = '7d' } = data;
  
  try {
    // Verify user has access to this greenhouse
    const greenhouseDoc = await getDb().collection('greenhouses').doc(greenhouseId).get();
    
    if (!greenhouseDoc.exists) {
      throw new functions.https.HttpsError('not-found', 'Greenhouse not found');
    }
    
    const greenhouse = greenhouseDoc.data();
    if (!greenhouse.users || !greenhouse.users.includes(context.auth.uid)) {
      throw new functions.https.HttpsError('permission-denied', 'Access denied');
    }
    
    // Get recent sensor readings
    const snapshot = await getDb()
      .collection('greenhouses')
      .doc(greenhouseId)
      .collection('sensors')
      .orderBy('timestamp', 'desc')
      .limit(100)
      .get();
    
    if (snapshot.empty) {
      return { success: true, analytics: null };
    }
    
    // Calculate statistics
    const readings = [];
    snapshot.forEach(doc => readings.push(doc.data()));
    
    const analytics = {
      temperature: calculateStats(readings, 'airTemp'),
      humidity: calculateStats(readings, 'airHumidity'),
      vwc: calculateStats(readings, 'vwc'),
      co2: calculateStats(readings, 'co2'),
      totalReadings: readings.length,
      lastUpdate: readings[0].timestamp
    };
    
    return { success: true, analytics };
    
  } catch (error) {
    console.error('Error getting analytics:', error);
    throw new functions.https.HttpsError('internal', error.message);
  }
};

/**
 * Update greenhouse configuration
 */
exports.updateConfig = async (data, context) => {
  if (!context.auth) {
    throw new functions.https.HttpsError('unauthenticated', 'User must be authenticated');
  }
  
  const { greenhouseId, config } = data;
  
  try {
    // Verify user has access to this greenhouse
    const greenhouseDoc = await getDb().collection('greenhouses').doc(greenhouseId).get();
    
    if (!greenhouseDoc.exists) {
      throw new functions.https.HttpsError('not-found', 'Greenhouse not found');
    }
    
    const greenhouse = greenhouseDoc.data();
    if (!greenhouse.users || !greenhouse.users.includes(context.auth.uid)) {
      throw new functions.https.HttpsError('permission-denied', 'Access denied');
    }
    
    await getDb()
      .collection('greenhouses')
      .doc(greenhouseId)
      .update({
        thresholds: config.thresholds,
        schedules: config.schedules,
        updatedAt: admin.firestore.FieldValue.serverTimestamp(),
        updatedBy: context.auth.uid
      });
    
    // Log configuration change
    await getDb()
      .collection('greenhouses')
      .doc(greenhouseId)
      .collection('logs')
      .add({
        eventType: 'config_update',
        userId: context.auth.uid,
        changes: config,
        timestamp: admin.firestore.FieldValue.serverTimestamp()
      });
    
    return { success: true };
    
  } catch (error) {
    console.error('Error updating config:', error);
    throw new functions.https.HttpsError('internal', error.message);
  }
};

/**
 * Get current greenhouse status
 */
exports.getGreenhouseStatus = async (data, context) => {
  if (!context.auth) {
    throw new functions.https.HttpsError('unauthenticated', 'User must be authenticated');
  }
  
  const { greenhouseId } = data;
  
  try {
    // Verify user has access to this greenhouse
    const greenhouseDoc = await getDb().collection('greenhouses').doc(greenhouseId).get();
    
    if (!greenhouseDoc.exists) {
      throw new functions.https.HttpsError('not-found', 'Greenhouse not found');
    }
    
    const greenhouse = greenhouseDoc.data();
    if (!greenhouse.users || !greenhouse.users.includes(context.auth.uid)) {
      throw new functions.https.HttpsError('permission-denied', 'Access denied');
    }
    
    // Get latest sensor reading
    const sensorSnapshot = await getDb()
      .collection('greenhouses')
      .doc(greenhouseId)
      .collection('sensors')
      .orderBy('timestamp', 'desc')
      .limit(1)
      .get();
    
    // Get active alerts
    const alertsSnapshot = await getDb()
      .collection('greenhouses')
      .doc(greenhouseId)
      .collection('alerts')
      .where('acknowledged', '==', false)
      .orderBy('timestamp', 'desc')
      .limit(10)
      .get();
    
    // Get device info - already fetched above
    // const greenhouseDoc = await getDb().collection('greenhouses').doc(greenhouseId).get();
    
    const latestSensor = sensorSnapshot.empty ? null : sensorSnapshot.docs[0].data();
    const activeAlerts = [];
    alertsSnapshot.forEach(doc => activeAlerts.push({ id: doc.id, ...doc.data() }));
    
    return {
      success: true,
      status: {
        sensors: latestSensor,
        alerts: activeAlerts,
        device: greenhouseDoc.data(),
        online: latestSensor ? (Date.now() - latestSensor.timestamp) < 300000 : false // 5 min
      }
    };
    
  } catch (error) {
    console.error('Error getting status:', error);
    throw new functions.https.HttpsError('internal', error.message);
  }
};

/**
 * Helper function to calculate statistics
 */
function calculateStats(readings, field) {
  const values = readings
    .map(r => r[field])
    .filter(v => v !== undefined && v !== null);
  
  if (values.length === 0) return null;
  
  return {
    current: values[0],
    avg: values.reduce((a, b) => a + b, 0) / values.length,
    min: Math.min(...values),
    max: Math.max(...values)
  };
}
