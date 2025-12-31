/**
 * GreenOS - Actuator Manager Header
 * 
 * ESP32-WROOM-32E Actuator Control
 * Handles control of all greenhouse actuators with safety features:
 * - Heaters (primary and secondary)
 * - Fans (exhaust and circulation)
 * - Irrigation pump
 * - Grow lights
 * - Safety interlocks and duty cycle management
 */

#ifndef ACTUATOR_MANAGER_H
#define ACTUATOR_MANAGER_H

#include <Arduino.h>
#include "config.h"

// ============================================================================
// EMERGENCY TYPES
// ============================================================================

enum EmergencyType {
  EMERGENCY_LOW_TEMP,
  EMERGENCY_HIGH_TEMP,
  EMERGENCY_FROST,
  EMERGENCY_SECURITY_BREACH,
  EMERGENCY_WATER_LEAK,
  EMERGENCY_POWER_FAILURE,
  EMERGENCY_HIGH_CO2,
  EMERGENCY_SENSOR_FAILURE
};

// ============================================================================
// ANOMALY/WARNING TYPES
// ============================================================================

enum AnomalyType {
  ANOMALY_TEMP_LOW,
  ANOMALY_TEMP_HIGH,
  ANOMALY_HUMIDITY_LOW,
  ANOMALY_HUMIDITY_HIGH,
  ANOMALY_CO2_LOW,
  ANOMALY_CO2_HIGH,
  ANOMALY_VWC_LOW,
  ANOMALY_VWC_HIGH,
  ANOMALY_PH_LOW,
  ANOMALY_PH_HIGH,
  ANOMALY_EC_LOW,
  ANOMALY_EC_HIGH,
  ANOMALY_RAPID_TEMP_CHANGE
};

// ============================================================================
// ACTUATOR STATE STRUCTURE
// ============================================================================

struct ActuatorState {
  bool heaterPrimary;
  bool heaterSecondary;
  bool fanExhaust;
  bool fanCirculation;
  bool pumpIrrigation;
  bool lightGrow;
  
  // Timing for duty cycle management
  unsigned long heaterPrimaryOnTime;
  unsigned long heaterSecondaryOnTime;
  unsigned long fanExhaustOnTime;
  unsigned long fanCirculationOnTime;
  unsigned long pumpOnTime;
  unsigned long lightOnTime;
  
  // Last toggle times (for minimum cycle times)
  unsigned long heaterPrimaryLastToggle;
  unsigned long heaterSecondaryLastToggle;
  unsigned long fanExhaustLastToggle;
  unsigned long pumpLastToggle;
};

// ============================================================================
// ACTUATOR MANAGER CLASS
// ============================================================================

class ActuatorManager {
public:
  ActuatorManager();
  
  /**
   * Initialize all actuator pins
   */
  void init();
  
  /**
   * Update function - call periodically to enforce duty cycles
   */
  void update();
  
  // ─────────────────────────────────────────────────────────────────────────
  // Individual Control (with safety checks)
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Control heater relay
   * @param primary True for primary heater, false for secondary
   * @param state True to turn on, false to turn off
   * @return True if command was executed (may fail due to safety interlock)
   */
  bool setHeater(bool primary, bool state);
  
  /**
   * Control fan relay
   * @param exhaust True for exhaust fan, false for circulation fan
   * @param state True to turn on, false to turn off
   * @return True if command was executed
   */
  bool setFan(bool exhaust, bool state);
  
  /**
   * Control irrigation pump
   * @param state True to turn on, false to turn off
   * @return True if command was executed
   */
  bool setPump(bool state);
  
  /**
   * Control grow lights
   * @param state True to turn on, false to turn off
   * @return True if command was executed
   */
  bool setLight(bool state);
  
  // ─────────────────────────────────────────────────────────────────────────
  // Emergency and Warning Responses
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Handle emergency situation with automatic response
   */
  void handleEmergency(EmergencyType type);
  
  /**
   * Handle warning-level anomaly with gradual response
   */
  void handleWarning(AnomalyType type);
  
  // ─────────────────────────────────────────────────────────────────────────
  // System Control
  // ─────────────────────────────────────────────────────────────────────────
  
  /**
   * Emergency stop - turn off all actuators immediately
   */
  void stopAll();
  
  /**
   * Print current actuator status to Serial
   */
  void printStatus();
  
  /**
   * Get current actuator state
   */
  ActuatorState getState() const { return _state; }
  
  // ─────────────────────────────────────────────────────────────────────────
  // State Queries
  // ─────────────────────────────────────────────────────────────────────────
  
  bool isHeaterOn(bool primary) const;
  bool isFanOn(bool exhaust) const;
  bool isPumpOn() const;
  bool isLightOn() const;
  bool isAnyActuatorOn() const;
  
  // ─────────────────────────────────────────────────────────────────────────
  // Safety Status
  // ─────────────────────────────────────────────────────────────────────────
  
  bool isSafetyInterlockActive() const { return _interlockActive; }
  String getInterlockReason() const { return _interlockReason; }
  
private:
  ActuatorState _state;
  
  // Safety flags
  bool _interlockActive;
  String _interlockReason;
  bool _emergencyMode;
  EmergencyType _currentEmergency;
  
  // Relay polarity (true = active LOW relays, common with optoisolated modules)
  static const bool RELAY_ACTIVE_LOW = true;
  
  // Minimum cycle times (prevent rapid on/off cycling that damages equipment)
  static const unsigned long MIN_HEATER_CYCLE_MS = 60000;   // 1 minute
  static const unsigned long MIN_FAN_CYCLE_MS = 30000;      // 30 seconds
  static const unsigned long MIN_PUMP_CYCLE_MS = 10000;     // 10 seconds
  
  // Maximum runtime (safety limits)
  static const unsigned long MAX_HEATER_RUNTIME_MS = 3600000;  // 1 hour continuous
  static const unsigned long MAX_PUMP_RUNTIME_MS = 600000;     // 10 minutes continuous
  
  // ─────────────────────────────────────────────────────────────────────────
  // Internal Methods
  // ─────────────────────────────────────────────────────────────────────────
  
  void setRelay(int pin, bool state);
  bool checkSafetyInterlocks(int actuator, bool desiredState);
  void enforceMinimumCycleTime();
  void enforceMaximumRuntime();
  
  // Emergency protocols
  void emergencyLowTemperature();
  void emergencyHighTemperature();
  void emergencyFrost();
  void emergencySecurityBreach();
  void emergencyWaterLeak();
  void emergencyPowerFailure();
  void emergencyHighCO2();
  void emergencySensorFailure();
  
  // Warning responses
  void warningTempLow();
  void warningTempHigh();
  void warningHumidityLow();
  void warningHumidityHigh();
  void warningCO2Low();
  void warningCO2High();
};

#endif // ACTUATOR_MANAGER_H
