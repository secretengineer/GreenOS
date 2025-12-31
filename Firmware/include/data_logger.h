/**
 * GreenOS - Data Logger Header
 * 
 * ESP32-WROOM-32E Offline Data Buffering
 * Handles SPIFFS-based storage for sensor data when network is unavailable
 */

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensor_manager.h"

// ============================================================================
// DATA LOGGER CLASS
// ============================================================================

class DataLogger {
public:
  DataLogger();
  
  /**
   * Initialize the data logger
   */
  void init();
  
  /**
   * Log sensor data to SPIFFS buffer
   */
  bool logSensorData(const SensorData& data);
  
  /**
   * Get count of buffered readings
   */
  int getBufferedCount();
  
  /**
   * Get oldest buffered reading (FIFO)
   * @param data Output parameter for sensor data
   * @return True if data available
   */
  bool getNextBuffered(SensorData& data);
  
  /**
   * Remove oldest buffered reading
   */
  void removeOldestBuffered();
  
  /**
   * Clear all buffered data
   */
  void clearBuffer();
  
  /**
   * Get storage statistics
   */
  void getStorageStats(size_t& used, size_t& total);
  
  /**
   * Check if buffer is near capacity
   */
  bool isBufferNearFull();
  
private:
  String _bufferDir;
  int _nextWriteIndex;
  int _nextReadIndex;
  int _bufferedCount;
  
  String getFilePath(int index);
  void loadIndex();
  void saveIndex();
};

#endif // DATA_LOGGER_H
