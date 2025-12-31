/**
 * GreenOS - Actuator Manager Implementation
 * 
 * ESP32-WROOM-32E Actuator Control with Safety Features
 */

#include "actuator_manager.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ActuatorManager::ActuatorManager()
  : _interlockActive(false),
    _interlockReason(""),
    _emergencyMode(false),
    _currentEmergency(EMERGENCY_LOW_TEMP)
{
  memset(&_state, 0, sizeof(ActuatorState));
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ActuatorManager::init() {
  Serial.println("[ACTUATOR] Initializing actuator manager...");
  
  // Configure relay pins as outputs
  pinMode(HEATER_PRIMARY_PIN, OUTPUT);
  pinMode(HEATER_SECONDARY_PIN, OUTPUT);
  pinMode(FAN_EXHAUST_PIN, OUTPUT);
  pinMode(FAN_CIRCULATION_PIN, OUTPUT);
  pinMode(PUMP_IRRIGATION_PIN, OUTPUT);
  pinMode(LIGHT_GROW_PIN, OUTPUT);
  
  // Configure buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize all relays to OFF state
  stopAll();
  
  Serial.println("[OK] Actuator manager initialized");
  Serial.println("     All relays initialized to OFF");
  printStatus();
}

// ============================================================================
// UPDATE LOOP
// ============================================================================

void ActuatorManager::update() {
  // Enforce safety limits
  enforceMinimumCycleTime();
  enforceMaximumRuntime();
  
  // Update runtime counters for active actuators
  unsigned long now = millis();
  
  if (_state.heaterPrimary) {
    _state.heaterPrimaryOnTime = now - _state.heaterPrimaryLastToggle;
  }
  if (_state.heaterSecondary) {
    _state.heaterSecondaryOnTime = now - _state.heaterSecondaryLastToggle;
  }
  if (_state.fanExhaust) {
    _state.fanExhaustOnTime = now - _state.fanExhaustLastToggle;
  }
  if (_state.fanCirculation) {
    _state.fanCirculationOnTime = now - _state.fanCirculationLastToggle;
  }
  if (_state.pumpIrrigation) {
    _state.pumpOnTime = now - _state.pumpLastToggle;
  }
}

// ============================================================================
// RELAY CONTROL
// ============================================================================

void ActuatorManager::setRelay(int pin, bool state) {
  // Handle active-low relay logic
  if (RELAY_ACTIVE_LOW) {
    digitalWrite(pin, state ? LOW : HIGH);
  } else {
    digitalWrite(pin, state ? HIGH : LOW);
  }
}

// ============================================================================
// HEATER CONTROL
// ============================================================================

bool ActuatorManager::setHeater(bool primary, bool state) {
  int pin = primary ? HEATER_PRIMARY_PIN : HEATER_SECONDARY_PIN;
  
  // Check safety interlocks
  if (!checkSafetyInterlocks(pin, state)) {
    Serial.printf("[SAFETY] Heater %s blocked: %s\n", 
                  primary ? "primary" : "secondary",
                  _interlockReason.c_str());
    return false;
  }
  
  // Check minimum cycle time
  unsigned long lastToggle = primary ? _state.heaterPrimaryLastToggle : _state.heaterSecondaryLastToggle;
  if (millis() - lastToggle < MIN_HEATER_CYCLE_MS) {
    Serial.println("[SAFETY] Heater minimum cycle time not met");
    return false;
  }
  
  // Apply state
  setRelay(pin, state);
  
  if (primary) {
    _state.heaterPrimary = state;
    _state.heaterPrimaryLastToggle = millis();
    if (state) _state.heaterPrimaryOnTime = 0;
  } else {
    _state.heaterSecondary = state;
    _state.heaterSecondaryLastToggle = millis();
    if (state) _state.heaterSecondaryOnTime = 0;
  }
  
  Serial.printf("[ACTUATOR] Heater %s: %s\n", 
                primary ? "primary" : "secondary",
                state ? "ON" : "OFF");
  
  return true;
}

bool ActuatorManager::isHeaterOn(bool primary) const {
  return primary ? _state.heaterPrimary : _state.heaterSecondary;
}

// ============================================================================
// FAN CONTROL
// ============================================================================

bool ActuatorManager::setFan(bool exhaust, bool state) {
  int pin = exhaust ? FAN_EXHAUST_PIN : FAN_CIRCULATION_PIN;
  
  // Check safety interlocks
  if (!checkSafetyInterlocks(pin, state)) {
    Serial.printf("[SAFETY] Fan %s blocked: %s\n",
                  exhaust ? "exhaust" : "circulation",
                  _interlockReason.c_str());
    return false;
  }
  
  // Check minimum cycle time for exhaust fan
  if (exhaust) {
    if (millis() - _state.fanExhaustLastToggle < MIN_FAN_CYCLE_MS) {
      Serial.println("[SAFETY] Exhaust fan minimum cycle time not met");
      return false;
    }
  }
  
  // Apply state
  setRelay(pin, state);
  
  if (exhaust) {
    _state.fanExhaust = state;
    _state.fanExhaustLastToggle = millis();
    if (state) _state.fanExhaustOnTime = 0;
  } else {
    _state.fanCirculation = state;
    _state.fanCirculationLastToggle = millis();
    if (state) _state.fanCirculationOnTime = 0;
  }
  
  Serial.printf("[ACTUATOR] Fan %s: %s\n",
                exhaust ? "exhaust" : "circulation",
                state ? "ON" : "OFF");
  
  return true;
}

bool ActuatorManager::isFanOn(bool exhaust) const {
  return exhaust ? _state.fanExhaust : _state.fanCirculation;
}

// ============================================================================
// PUMP CONTROL
// ============================================================================

bool ActuatorManager::setPump(bool state) {
  // Check safety interlocks
  if (!checkSafetyInterlocks(PUMP_IRRIGATION_PIN, state)) {
    Serial.printf("[SAFETY] Pump blocked: %s\n", _interlockReason.c_str());
    return false;
  }
  
  // Check minimum cycle time
  if (millis() - _state.pumpLastToggle < MIN_PUMP_CYCLE_MS) {
    Serial.println("[SAFETY] Pump minimum cycle time not met");
    return false;
  }
  
  // Apply state
  setRelay(PUMP_IRRIGATION_PIN, state);
  _state.pumpIrrigation = state;
  _state.pumpLastToggle = millis();
  if (state) _state.pumpOnTime = 0;
  
  Serial.printf("[ACTUATOR] Irrigation pump: %s\n", state ? "ON" : "OFF");
  
  return true;
}

bool ActuatorManager::isPumpOn() const {
  return _state.pumpIrrigation;
}

// ============================================================================
// LIGHT CONTROL
// ============================================================================

bool ActuatorManager::setLight(bool state) {
  setRelay(LIGHT_GROW_PIN, state);
  _state.lightGrow = state;
  
  if (state) {
    _state.lightOnTime = 0;
  }
  
  Serial.printf("[ACTUATOR] Grow lights: %s\n", state ? "ON" : "OFF");
  
  return true;
}

bool ActuatorManager::isLightOn() const {
  return _state.lightGrow;
}

bool ActuatorManager::isAnyActuatorOn() const {
  return _state.heaterPrimary || _state.heaterSecondary ||
         _state.fanExhaust || _state.fanCirculation ||
         _state.pumpIrrigation || _state.lightGrow;
}

// ============================================================================
// SAFETY INTERLOCKS
// ============================================================================

bool ActuatorManager::checkSafetyInterlocks(int actuator, bool desiredState) {
  _interlockActive = false;
  _interlockReason = "";
  
  if (!desiredState) {
    return true;  // Always allow turning things OFF
  }
  
  // Interlock: Don't run exhaust fan while heater is on (wastes heat)
  if (actuator == FAN_EXHAUST_PIN && (_state.heaterPrimary || _state.heaterSecondary)) {
    _interlockActive = true;
    _interlockReason = "Cannot run exhaust while heater is active";
    return false;
  }
  
  // Interlock: Don't run heater while exhaust fan is on
  if ((actuator == HEATER_PRIMARY_PIN || actuator == HEATER_SECONDARY_PIN) && _state.fanExhaust) {
    _interlockActive = true;
    _interlockReason = "Cannot run heater while exhaust fan is active";
    return false;
  }
  
  // Interlock: Don't run both heaters if power is limited (optional)
  // This can be configured based on electrical capacity
  
  return true;
}

void ActuatorManager::enforceMinimumCycleTime() {
  // This is checked in individual setters, but could add additional logic here
}

void ActuatorManager::enforceMaximumRuntime() {
  unsigned long now = millis();
  
  // Check heater runtime limits
  if (_state.heaterPrimary && _state.heaterPrimaryOnTime > MAX_HEATER_RUNTIME_MS) {
    Serial.println("[SAFETY] Primary heater max runtime exceeded - forcing OFF");
    setRelay(HEATER_PRIMARY_PIN, false);
    _state.heaterPrimary = false;
  }
  
  if (_state.heaterSecondary && _state.heaterSecondaryOnTime > MAX_HEATER_RUNTIME_MS) {
    Serial.println("[SAFETY] Secondary heater max runtime exceeded - forcing OFF");
    setRelay(HEATER_SECONDARY_PIN, false);
    _state.heaterSecondary = false;
  }
  
  // Check pump runtime limit (prevent flooding)
  if (_state.pumpIrrigation && _state.pumpOnTime > MAX_PUMP_RUNTIME_MS) {
    Serial.println("[SAFETY] Irrigation pump max runtime exceeded - forcing OFF");
    setRelay(PUMP_IRRIGATION_PIN, false);
    _state.pumpIrrigation = false;
  }
}

// ============================================================================
// EMERGENCY HANDLERS
// ============================================================================

void ActuatorManager::handleEmergency(EmergencyType type) {
  _emergencyMode = true;
  _currentEmergency = type;
  
  Serial.printf("\n[EMERGENCY] Type: %d\n", type);
  
  // Sound alarm
  tone(BUZZER_PIN, 2000, 500);
  
  switch (type) {
    case EMERGENCY_LOW_TEMP:
      emergencyLowTemperature();
      break;
    case EMERGENCY_HIGH_TEMP:
      emergencyHighTemperature();
      break;
    case EMERGENCY_FROST:
      emergencyFrost();
      break;
    case EMERGENCY_SECURITY_BREACH:
      emergencySecurityBreach();
      break;
    case EMERGENCY_WATER_LEAK:
      emergencyWaterLeak();
      break;
    case EMERGENCY_POWER_FAILURE:
      emergencyPowerFailure();
      break;
    case EMERGENCY_HIGH_CO2:
      emergencyHighCO2();
      break;
    case EMERGENCY_SENSOR_FAILURE:
      emergencySensorFailure();
      break;
  }
}

void ActuatorManager::emergencyLowTemperature() {
  Serial.println("[EMERGENCY] LOW TEMPERATURE - Activating all heaters");
  
  // Turn off ventilation
  setRelay(FAN_EXHAUST_PIN, false);
  _state.fanExhaust = false;
  
  // Activate both heaters (override interlocks in emergency)
  setRelay(HEATER_PRIMARY_PIN, true);
  _state.heaterPrimary = true;
  _state.heaterPrimaryLastToggle = millis();
  
  setRelay(HEATER_SECONDARY_PIN, true);
  _state.heaterSecondary = true;
  _state.heaterSecondaryLastToggle = millis();
  
  // Turn on circulation fan to distribute heat
  setRelay(FAN_CIRCULATION_PIN, true);
  _state.fanCirculation = true;
}

void ActuatorManager::emergencyHighTemperature() {
  Serial.println("[EMERGENCY] HIGH TEMPERATURE - Maximum cooling");
  
  // Turn off all heat sources
  setRelay(HEATER_PRIMARY_PIN, false);
  _state.heaterPrimary = false;
  
  setRelay(HEATER_SECONDARY_PIN, false);
  _state.heaterSecondary = false;
  
  // Turn off grow lights (they generate heat)
  setRelay(LIGHT_GROW_PIN, false);
  _state.lightGrow = false;
  
  // Maximum ventilation
  setRelay(FAN_EXHAUST_PIN, true);
  _state.fanExhaust = true;
  _state.fanExhaustLastToggle = millis();
  
  setRelay(FAN_CIRCULATION_PIN, true);
  _state.fanCirculation = true;
}

void ActuatorManager::emergencyFrost() {
  Serial.println("[EMERGENCY] FROST WARNING - Maximum heating");
  emergencyLowTemperature();  // Same response as low temp
}

void ActuatorManager::emergencySecurityBreach() {
  Serial.println("[EMERGENCY] SECURITY BREACH - Alarm activated");
  
  // Sound continuous alarm
  for (int i = 0; i < 10; i++) {
    tone(BUZZER_PIN, 2500, 200);
    delay(300);
  }
}

void ActuatorManager::emergencyWaterLeak() {
  Serial.println("[EMERGENCY] WATER LEAK - Shutting off pump");
  
  // Immediately stop pump
  setRelay(PUMP_IRRIGATION_PIN, false);
  _state.pumpIrrigation = false;
}

void ActuatorManager::emergencyPowerFailure() {
  Serial.println("[EMERGENCY] POWER FAILURE - Minimal operation mode");
  
  // Shut down non-essential equipment
  setRelay(LIGHT_GROW_PIN, false);
  _state.lightGrow = false;
  
  setRelay(PUMP_IRRIGATION_PIN, false);
  _state.pumpIrrigation = false;
  
  // Keep only essential climate control if UPS provides power
}

void ActuatorManager::emergencyHighCO2() {
  Serial.println("[EMERGENCY] HIGH CO2 - Maximum ventilation");
  
  // Maximum ventilation to purge CO2
  setRelay(FAN_EXHAUST_PIN, true);
  _state.fanExhaust = true;
  _state.fanExhaustLastToggle = millis();
  
  setRelay(FAN_CIRCULATION_PIN, true);
  _state.fanCirculation = true;
  
  // Turn off heaters (may be overridden by interlocks)
  setRelay(HEATER_PRIMARY_PIN, false);
  _state.heaterPrimary = false;
  setRelay(HEATER_SECONDARY_PIN, false);
  _state.heaterSecondary = false;
}

void ActuatorManager::emergencySensorFailure() {
  Serial.println("[EMERGENCY] SENSOR FAILURE - Safe mode");
  
  // Enter safe defaults
  // Keep circulation on, everything else off
  stopAll();
  
  setRelay(FAN_CIRCULATION_PIN, true);
  _state.fanCirculation = true;
}

// ============================================================================
// WARNING HANDLERS (Gradual responses)
// ============================================================================

void ActuatorManager::handleWarning(AnomalyType type) {
  Serial.printf("[WARNING] Anomaly type: %d\n", type);
  
  switch (type) {
    case ANOMALY_TEMP_LOW:
      warningTempLow();
      break;
    case ANOMALY_TEMP_HIGH:
      warningTempHigh();
      break;
    case ANOMALY_HUMIDITY_LOW:
      warningHumidityLow();
      break;
    case ANOMALY_HUMIDITY_HIGH:
      warningHumidityHigh();
      break;
    case ANOMALY_CO2_LOW:
      warningCO2Low();
      break;
    case ANOMALY_CO2_HIGH:
      warningCO2High();
      break;
    default:
      break;
  }
}

void ActuatorManager::warningTempLow() {
  // Gradual response - turn on primary heater only
  if (!_state.heaterPrimary) {
    setHeater(true, true);
  }
}

void ActuatorManager::warningTempHigh() {
  // Gradual response - increase ventilation
  if (!_state.fanCirculation) {
    setFan(false, true);  // Circulation first
  }
  
  if (!_state.fanExhaust && !_state.heaterPrimary && !_state.heaterSecondary) {
    setFan(true, true);  // Then exhaust if no heaters
  }
}

void ActuatorManager::warningHumidityLow() {
  // Could trigger misting system if available
  Serial.println("[WARNING] Low humidity - consider misting");
}

void ActuatorManager::warningHumidityHigh() {
  // Increase ventilation
  if (!_state.fanExhaust) {
    setFan(true, true);
  }
}

void ActuatorManager::warningCO2Low() {
  // Reduce ventilation to retain CO2
  if (_state.fanExhaust) {
    setFan(true, false);
  }
}

void ActuatorManager::warningCO2High() {
  // Increase ventilation
  if (!_state.fanExhaust) {
    setFan(true, true);
  }
}

// ============================================================================
// SYSTEM CONTROL
// ============================================================================

void ActuatorManager::stopAll() {
  Serial.println("[ACTUATOR] STOP ALL - Turning off all actuators");
  
  setRelay(HEATER_PRIMARY_PIN, false);
  setRelay(HEATER_SECONDARY_PIN, false);
  setRelay(FAN_EXHAUST_PIN, false);
  setRelay(FAN_CIRCULATION_PIN, false);
  setRelay(PUMP_IRRIGATION_PIN, false);
  setRelay(LIGHT_GROW_PIN, false);
  
  _state.heaterPrimary = false;
  _state.heaterSecondary = false;
  _state.fanExhaust = false;
  _state.fanCirculation = false;
  _state.pumpIrrigation = false;
  _state.lightGrow = false;
  
  _emergencyMode = false;
}

void ActuatorManager::printStatus() {
  Serial.println("┌─────────────────────────────────────────┐");
  Serial.println("│         ACTUATOR STATUS                 │");
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf("│ Heater Primary:    %s                   │\n", _state.heaterPrimary ? "ON " : "OFF");
  Serial.printf("│ Heater Secondary:  %s                   │\n", _state.heaterSecondary ? "ON " : "OFF");
  Serial.printf("│ Fan Exhaust:       %s                   │\n", _state.fanExhaust ? "ON " : "OFF");
  Serial.printf("│ Fan Circulation:   %s                   │\n", _state.fanCirculation ? "ON " : "OFF");
  Serial.printf("│ Irrigation Pump:   %s                   │\n", _state.pumpIrrigation ? "ON " : "OFF");
  Serial.printf("│ Grow Lights:       %s                   │\n", _state.lightGrow ? "ON " : "OFF");
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf("│ Emergency Mode:    %s                   │\n", _emergencyMode ? "YES" : "NO ");
  Serial.printf("│ Interlock Active:  %s                   │\n", _interlockActive ? "YES" : "NO ");
  Serial.println("└─────────────────────────────────────────┘");
}
