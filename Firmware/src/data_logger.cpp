/**
 * GreenOS - Data Logger Implementation
 * 
 * ESP32-WROOM-32E Offline Data Buffering
 */

#include "data_logger.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

DataLogger::DataLogger()
  : _bufferDir("/buffer"),
    _nextWriteIndex(0),
    _nextReadIndex(0),
    _bufferedCount(0)
{
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void DataLogger::init() {
  Serial.println("[LOGGER] Initializing data logger...");
  
  // Create buffer directory if it doesn't exist
  if (!SPIFFS.exists(_bufferDir)) {
    // SPIFFS doesn't have directories, we'll use naming convention
    Serial.println("[LOGGER] Buffer directory initialized");
  }
  
  // Load saved index
  loadIndex();
  
  Serial.printf("[OK] Data logger initialized (%d buffered readings)\n", _bufferedCount);
}

// ============================================================================
// LOGGING
// ============================================================================

bool DataLogger::logSensorData(const SensorData& data) {
  // Check if buffer is full
  if (_bufferedCount >= MAX_BUFFERED_READINGS) {
    Serial.println("[LOGGER] Buffer full - removing oldest entry");
    removeOldestBuffered();
  }
  
  // Create JSON document
  StaticJsonDocument<512> doc;
  
  doc["ts"] = data.unixTime;
  doc["airT"] = serialized(String(data.airTemp, 1));
  doc["airH"] = serialized(String(data.airHumidity, 1));
  doc["co2"] = serialized(String(data.co2, 0));
  doc["aq"] = serialized(String(data.airQualityPPM, 0));
  doc["soilT"] = serialized(String(data.substrateTemp, 1));
  doc["vwc"] = serialized(String(data.vwc, 1));
  doc["ph"] = serialized(String(data.ph, 2));
  doc["ec"] = serialized(String(data.ec, 2));
  doc["n"] = data.nitrogen;
  doc["p"] = data.phosphorus;
  doc["k"] = data.potassium;
  doc["par"] = serialized(String(data.par, 0));
  doc["motion"] = data.motionDetected;
  doc["ups"] = data.upsActive;
  
  // Write to file
  String filePath = getFilePath(_nextWriteIndex);
  File file = SPIFFS.open(filePath, FILE_WRITE);
  
  if (!file) {
    Serial.printf("[LOGGER] Failed to open file: %s\n", filePath.c_str());
    return false;
  }
  
  size_t written = serializeJson(doc, file);
  file.close();
  
  if (written > 0) {
    _nextWriteIndex = (_nextWriteIndex + 1) % MAX_BUFFERED_READINGS;
    _bufferedCount++;
    saveIndex();
    Serial.printf("[LOGGER] Logged reading #%d\n", _bufferedCount);
    return true;
  }
  
  return false;
}

bool DataLogger::getNextBuffered(SensorData& data) {
  if (_bufferedCount == 0) {
    return false;
  }
  
  String filePath = getFilePath(_nextReadIndex);
  File file = SPIFFS.open(filePath, FILE_READ);
  
  if (!file) {
    return false;
  }
  
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.printf("[LOGGER] Failed to parse: %s\n", error.c_str());
    return false;
  }
  
  // Parse JSON back to SensorData
  data.unixTime = doc["ts"] | 0;
  data.airTemp = doc["airT"] | 0.0f;
  data.airHumidity = doc["airH"] | 0.0f;
  data.co2 = doc["co2"] | 0.0f;
  data.airQualityPPM = doc["aq"] | 0.0f;
  data.substrateTemp = doc["soilT"] | 0.0f;
  data.vwc = doc["vwc"] | 0.0f;
  data.ph = doc["ph"] | 0.0f;
  data.ec = doc["ec"] | 0.0f;
  data.nitrogen = doc["n"] | 0.0f;
  data.phosphorus = doc["p"] | 0.0f;
  data.potassium = doc["k"] | 0.0f;
  data.par = doc["par"] | 0.0f;
  data.motionDetected = doc["motion"] | false;
  data.upsActive = doc["ups"] | false;
  
  return true;
}

void DataLogger::removeOldestBuffered() {
  if (_bufferedCount == 0) {
    return;
  }
  
  String filePath = getFilePath(_nextReadIndex);
  SPIFFS.remove(filePath);
  
  _nextReadIndex = (_nextReadIndex + 1) % MAX_BUFFERED_READINGS;
  _bufferedCount--;
  saveIndex();
}

int DataLogger::getBufferedCount() {
  return _bufferedCount;
}

void DataLogger::clearBuffer() {
  Serial.println("[LOGGER] Clearing buffer...");
  
  // Remove all buffer files
  for (int i = 0; i < MAX_BUFFERED_READINGS; i++) {
    String filePath = getFilePath(i);
    if (SPIFFS.exists(filePath)) {
      SPIFFS.remove(filePath);
    }
  }
  
  _nextWriteIndex = 0;
  _nextReadIndex = 0;
  _bufferedCount = 0;
  saveIndex();
  
  Serial.println("[LOGGER] Buffer cleared");
}

// ============================================================================
// STORAGE MANAGEMENT
// ============================================================================

void DataLogger::getStorageStats(size_t& used, size_t& total) {
  used = SPIFFS.usedBytes();
  total = SPIFFS.totalBytes();
}

bool DataLogger::isBufferNearFull() {
  size_t used, total;
  getStorageStats(used, total);
  
  // Consider near full if >80% used or >90% of max readings
  return (used > total * 0.8) || (_bufferedCount > MAX_BUFFERED_READINGS * 0.9);
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

String DataLogger::getFilePath(int index) {
  return "/buf_" + String(index) + ".json";
}

void DataLogger::loadIndex() {
  File file = SPIFFS.open("/buffer_idx.json", FILE_READ);
  if (!file) {
    return;
  }
  
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (!error) {
    _nextWriteIndex = doc["w"] | 0;
    _nextReadIndex = doc["r"] | 0;
    _bufferedCount = doc["c"] | 0;
  }
}

void DataLogger::saveIndex() {
  File file = SPIFFS.open("/buffer_idx.json", FILE_WRITE);
  if (!file) {
    return;
  }
  
  StaticJsonDocument<128> doc;
  doc["w"] = _nextWriteIndex;
  doc["r"] = _nextReadIndex;
  doc["c"] = _bufferedCount;
  
  serializeJson(doc, file);
  file.close();
}
