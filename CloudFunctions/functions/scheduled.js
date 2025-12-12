/**
 * GreenOS - Scheduled Functions
 * Functions that run on a schedule via Cloud Scheduler
 */

const admin = require('firebase-admin');
const { BigQuery } = require('@google-cloud/bigquery');
const axios = require('axios');

const db = admin.firestore();
const bigquery = new BigQuery();

/**
 * Export sensor data to BigQuery every hour
 */
exports.exportToBigQuery = async (context) => {
  console.log('Starting BigQuery export...');
  
  try {
    // Get all greenhouses
    const greenhousesSnapshot = await db.collection('greenhouses').get();
    
    let totalExported = 0;
    
    for (const greenhouseDoc of greenhousesSnapshot.docs) {
      const greenhouseId = greenhouseDoc.id;
      
      // Get sensor data from the last hour that hasn't been exported
      const oneHourAgo = new Date(Date.now() - 3600000);
      
      const sensorsSnapshot = await db
        .collection('greenhouses')
        .doc(greenhouseId)
        .collection('sensors')
        .where('timestamp', '>', oneHourAgo)
        .where('exported', '==', false)
        .get();
      
      if (sensorsSnapshot.empty) {
        console.log(`No new data for ${greenhouseId}`);
        continue;
      }
      
      // Prepare rows for BigQuery
      const rows = [];
      sensorsSnapshot.forEach(doc => {
        const data = doc.data();
        
        // Create separate rows for each sensor type
        Object.keys(data).forEach(key => {
          if (key !== 'timestamp' && key !== 'exported' && data[key] !== undefined) {
            rows.push({
              greenhouse_id: greenhouseId,
              timestamp: new Date(data.timestamp),
              sensor_type: key,
              value: data[key]
            });
          }
        });
      });
      
      // Insert into BigQuery
      await bigquery
        .dataset('greenos')
        .table('sensor_data')
        .insert(rows);
      
      // Mark as exported
      const batch = db.batch();
      sensorsSnapshot.forEach(doc => {
        batch.update(doc.ref, { exported: true });
      });
      await batch.commit();
      
      totalExported += rows.length;
      console.log(`Exported ${rows.length} rows for ${greenhouseId}`);
    }
    
    console.log(`Total exported: ${totalExported} rows`);
    return { success: true, exported: totalExported };
    
  } catch (error) {
    console.error('Error exporting to BigQuery:', error);
    throw error;
  }
};

/**
 * Fetch external weather data
 */
exports.fetchWeatherData = async (context) => {
  console.log('Fetching weather data...');
  
  try {
    // Get all greenhouses with location data
    const greenhousesSnapshot = await db
      .collection('greenhouses')
      .where('location', '!=', null)
      .get();
    
    for (const greenhouseDoc of greenhousesSnapshot.docs) {
      const greenhouse = greenhouseDoc.data();
      const location = greenhouse.location;
      
      // Fetch weather from OpenWeather API (example)
      // Replace with your actual API key and endpoint
      const apiKey = process.env.WEATHER_API_KEY;
      const url = `https://api.openweathermap.org/data/2.5/weather?lat=${location.lat}&lon=${location.lon}&appid=${apiKey}`;
      
      const response = await axios.get(url);
      const weather = response.data;
      
      // Store weather data
      await db
        .collection('greenhouses')
        .doc(greenhouseDoc.id)
        .collection('weather')
        .add({
          temperature: weather.main.temp - 273.15, // Convert to Celsius
          humidity: weather.main.humidity,
          pressure: weather.main.pressure,
          conditions: weather.weather[0].description,
          windSpeed: weather.wind.speed,
          timestamp: admin.firestore.FieldValue.serverTimestamp(),
          source: 'openweather'
        });
      
      console.log(`Weather data updated for ${greenhouseDoc.id}`);
    }
    
    return { success: true };
    
  } catch (error) {
    console.error('Error fetching weather data:', error);
    throw error;
  }
};

/**
 * Generate daily summary reports
 */
exports.generateDailySummary = async (context) => {
  console.log('Generating daily summaries...');
  
  try {
    const greenhousesSnapshot = await db.collection('greenhouses').get();
    
    for (const greenhouseDoc of greenhousesSnapshot.docs) {
      const greenhouseId = greenhouseDoc.id;
      
      // Get yesterday's data
      const yesterday = new Date();
      yesterday.setDate(yesterday.getDate() - 1);
      yesterday.setHours(0, 0, 0, 0);
      
      const today = new Date();
      today.setHours(0, 0, 0, 0);
      
      const sensorsSnapshot = await db
        .collection('greenhouses')
        .doc(greenhouseId)
        .collection('sensors')
        .where('timestamp', '>=', yesterday)
        .where('timestamp', '<', today)
        .get();
      
      if (sensorsSnapshot.empty) continue;
      
      // Calculate daily statistics
      const readings = [];
      sensorsSnapshot.forEach(doc => readings.push(doc.data()));
      
      const summary = {
        date: yesterday,
        temperature: calculateDailyStats(readings, 'airTemp'),
        humidity: calculateDailyStats(readings, 'airHumidity'),
        vwc: calculateDailyStats(readings, 'vwc'),
        co2: calculateDailyStats(readings, 'co2'),
        totalReadings: readings.length,
        alertsCount: await getAlertsCount(greenhouseId, yesterday, today),
        createdAt: admin.firestore.FieldValue.serverTimestamp()
      };
      
      // Store summary
      await db
        .collection('greenhouses')
        .doc(greenhouseId)
        .collection('daily_summaries')
        .add(summary);
      
      console.log(`Daily summary created for ${greenhouseId}`);
    }
    
    return { success: true };
    
  } catch (error) {
    console.error('Error generating daily summaries:', error);
    throw error;
  }
};

/**
 * Check device health and connectivity
 */
exports.checkDeviceHealth = async (context) => {
  console.log('Checking device health...');
  
  try {
    const greenhousesSnapshot = await db.collection('greenhouses').get();
    
    for (const greenhouseDoc of greenhousesSnapshot.docs) {
      const greenhouseId = greenhouseDoc.id;
      
      // Get last sensor reading
      const lastReadingSnapshot = await db
        .collection('greenhouses')
        .doc(greenhouseId)
        .collection('sensors')
        .orderBy('timestamp', 'desc')
        .limit(1)
        .get();
      
      if (lastReadingSnapshot.empty) continue;
      
      const lastReading = lastReadingSnapshot.docs[0].data();
      const timeSinceLastReading = Date.now() - lastReading.timestamp;
      
      // Alert if no data for more than 10 minutes
      if (timeSinceLastReading > 600000) {
        await db
          .collection('greenhouses')
          .doc(greenhouseId)
          .collection('alerts')
          .add({
            type: 'DEVICE_OFFLINE',
            severity: 'high',
            message: `Device has been offline for ${Math.round(timeSinceLastReading / 60000)} minutes`,
            timestamp: admin.firestore.FieldValue.serverTimestamp(),
            acknowledged: false
          });
        
        console.log(`Offline alert created for ${greenhouseId}`);
      }
    }
    
    return { success: true };
    
  } catch (error) {
    console.error('Error checking device health:', error);
    throw error;
  }
};

/**
 * Helper functions
 */
function calculateDailyStats(readings, field) {
  const values = readings
    .map(r => r[field])
    .filter(v => v !== undefined && v !== null);
  
  if (values.length === 0) return null;
  
  return {
    avg: values.reduce((a, b) => a + b, 0) / values.length,
    min: Math.min(...values),
    max: Math.max(...values),
    count: values.length
  };
}

async function getAlertsCount(greenhouseId, startDate, endDate) {
  const alertsSnapshot = await db
    .collection('greenhouses')
    .doc(greenhouseId)
    .collection('alerts')
    .where('timestamp', '>=', startDate)
    .where('timestamp', '<', endDate)
    .get();
  
  return alertsSnapshot.size;
}
