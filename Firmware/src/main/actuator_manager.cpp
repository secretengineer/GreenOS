/**
 * GreenOS - Actuator Manager Implementation
 * 
 * Handles control of all greenhouse actuators with safety features:
 * - Heaters (Primary + Secondary backup)
 * - Ventilation Fans (Exhaust + Circulation)
 * - Irrigation Pump
 * - Grow Lights
 * 
 * Safety Features:
 * - Relay state tracking and validation
 * - Interlocks to prevent conflicting operations
 * - Emergency protocols for critical conditions
 * - Duty cycle limiting to prevent equipment damage
 */

#include "actuator_manager.h"
#include "config.h"

// ============================================================================
// ACTUATOR STATE TRACKING
// ============================================================================

struct ActuatorState {
  bool heaterPrimary;
  bool heaterSecondary;
  bool fanExhaust;
  bool fanCirculation;
  bool pumpIrrigation;
  bool lightGrow;
  
  // Duty cycle tracking (prevent excessive cycling)
  unsigned long heaterOnTime;
  unsigned long fanOnTime;
  unsigned long pumpOnTime;
  unsigned long lastHeaterChange;
  unsigned long lastFanChange;
  unsigned long lastPumpChange;
};

ActuatorState state;

// Safety limits
#define MIN_CYCLE_TIME_MS 60000      // Minimum 1 minute between state changes
#define MAX_HEATER_DUTY_CYCLE 0.8    // Maximum 80% duty cycle
#define MAX_PUMP_RUN_TIME_MS 600000  // Maximum 10 minutes continuous run

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ActuatorManager::ActuatorManager() {
  // Initialize all actuators to OFF state
  state.heaterPrimary = false;
  state.heaterSecondary = false;
  state.fanExhaust = false;
  state.fanCirculation = false;
  state.pumpIrrigation = false;
  state.lightGrow = false;
  
  state.heaterOnTime = 0;
  state.fanOnTime = 0;
  state.pumpOnTime = 0;
  state.lastHeaterChange = 0;
  state.lastFanChange = 0;
  state.lastPumpChange = 0;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ActuatorManager::init() {
  Serial.println("=== Initializing Actuators ===");
  
  // Configure relay pins as outputs
  pinMode(HEATER_PRIMARY_PIN, OUTPUT);
  pinMode(HEATER_SECONDARY_PIN, OUTPUT);
  pinMode(FAN_EXHAUST_PIN, OUTPUT);
  pinMode(FAN_CIRCULATION_PIN, OUTPUT);
  pinMode(PUMP_IRRIGATION_PIN, OUTPUT);
  pinMode(LIGHT_GROW_PIN, OUTPUT);
  
  // Initialize all relays to OFF (LOW = OFF for active-high relays)
  // Note: Adjust based on your relay module (some are active-low)
  digitalWrite(HEATER_PRIMARY_PIN, LOW);
  digitalWrite(HEATER_SECONDARY_PIN, LOW);
  digitalWrite(FAN_EXHAUST_PIN, LOW);
  digitalWrite(FAN_CIRCULATION_PIN, LOW);
  digitalWrite(PUMP_IRRIGATION_PIN, LOW);
  digitalWrite(LIGHT_GROW_PIN, LOW);
  
  Serial.println("✓ All actuators initialized to OFF state");
  Serial.println("=== Actuator Initialization Complete ===\n");
}

// ============================================================================
// INDIVIDUAL ACTUATOR CONTROL
// ============================================================================

void ActuatorManager::setHeater(bool primary, bool turnOn) {
  unsigned long now = millis();
  
  // Check minimum cycle time to prevent rapid switching
  if (now - state.lastHeaterChange < MIN_CYCLE_TIME_MS) {
    Serial.println("⚠️ Heater: Minimum cycle time not met, ignoring command");
    return;
  }
  
  // Safety interlock: Don't run heater and exhaust fan simultaneously
  if (turnOn && state.fanExhaust) {
    Serial.println("⚠️ Heater: Cannot enable while exhaust fan is running");
    return;
  }
  
  int pin = primary ? HEATER_PRIMARY_PIN : HEATER_SECONDARY_PIN;
  bool* stateVar = primary ? &state.heaterPrimary : &state.heaterSecondary;
  
  if (*stateVar != turnOn) {
    digitalWrite(pin, turnOn ? HIGH : LOW);
    *stateVar = turnOn;
    state.lastHeaterChange = now;
    
    if (turnOn) {
      state.heaterOnTime = now;
    }
    
    Serial.print("✓ Heater ");
    Serial.print(primary ? "Primary" : "Secondary");
    Serial.print(": ");
    Serial.println(turnOn ? "ON" : "OFF");
  }
}

void ActuatorManager::setFan(bool exhaust, bool turnOn) {
  unsigned long now = millis();
  
  // Check minimum cycle time
  if (now - state.lastFanChange < MIN_CYCLE_TIME_MS) {
    Serial.println("⚠️ Fan: Minimum cycle time not met, ignoring command");
    return;
  }
  
  // Safety interlock: Turn off heaters before enabling exhaust fan
  if (exhaust && turnOn && (state.heaterPrimary || state.heaterSecondary)) {
    Serial.println("⚠️ Exhaust Fan: Disabling heaters first");
    setHeater(true, false);
    setHeater(false, false);
    delay(1000);  // Wait for heaters to fully disengage
  }
  
  int pin = exhaust ? FAN_EXHAUST_PIN : FAN_CIRCULATION_PIN;
  bool* stateVar = exhaust ? &state.fanExhaust : &state.fanCirculation;
  
  if (*stateVar != turnOn) {
    digitalWrite(pin, turnOn ? HIGH : LOW);
    *stateVar = turnOn;
    state.lastFanChange = now;
    
    if (turnOn) {
      state.fanOnTime = now;
    }
    
    Serial.print("✓ Fan ");
    Serial.print(exhaust ? "Exhaust" : "Circulation");
    Serial.print(": ");
    Serial.println(turnOn ? "ON" : "OFF");
  }
}

void ActuatorManager::setPump(bool turnOn) {
  unsigned long now = millis();
  
  // Check minimum cycle time
  if (now - state.lastPumpChange < MIN_CYCLE_TIME_MS) {
    Serial.println("⚠️ Pump: Minimum cycle time not met, ignoring command");
    return;
  }
  
  // Safety: Limit maximum continuous run time
  if (turnOn && state.pumpIrrigation) {
    unsigned long runTime = now - state.pumpOnTime;
    if (runTime > MAX_PUMP_RUN_TIME_MS) {
      Serial.println("⚠️ Pump: Maximum run time exceeded, forcing OFF");
      turnOn = false;
    }
  }
  
  if (state.pumpIrrigation != turnOn) {
    digitalWrite(PUMP_IRRIGATION_PIN, turnOn ? HIGH : LOW);
    state.pumpIrrigation = turnOn;
    state.lastPumpChange = now;
    
    if (turnOn) {
      state.pumpOnTime = now;
    }
    
    Serial.print("✓ Irrigation Pump: ");
    Serial.println(turnOn ? "ON" : "OFF");
  }
}

void ActuatorManager::setLight(bool turnOn) {
  if (state.lightGrow != turnOn) {
    digitalWrite(LIGHT_GROW_PIN, turnOn ? HIGH : LOW);
    state.lightGrow = turnOn;
    Serial.print("✓ Grow Lights: ");
    Serial.println(turnOn ? "ON" : "OFF");
  }
}

// ============================================================================
// EMERGENCY RESPONSE PROTOCOLS
// ============================================================================

void ActuatorManager::handleEmergency(EmergencyType type) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║      EMERGENCY PROTOCOL ACTIVATED      ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  switch (type) {
    case LOW_TEMP:
      emergencyLowTemperature();
      break;
      
    case HIGH_TEMP:
      emergencyHighTemperature();
      break;
      
    case SECURITY_BREACH:
      emergencySecurityBreach();
      break;
      
    case WATER_LEAK:
      emergencyWaterLeak();
      break;
      
    case POWER_FAILURE:
      emergencyPowerFailure();
      break;
  }
  
  Serial.println("Emergency protocol complete.\n");
}

void ActuatorManager::emergencyLowTemperature() {
  Serial.println("🔥 EMERGENCY: Low Temperature - Activating Heat");
  
  // Turn off cooling systems
  setFan(true, false);   // Exhaust fan OFF
  
  // Enable all heating
  setHeater(true, true);   // Primary heater ON
  setHeater(false, true);  // Secondary heater ON (backup)
  
  // Enable circulation to distribute heat
  setFan(false, true);  // Circulation fan ON
}

void ActuatorManager::emergencyHighTemperature() {
  Serial.println("❄️ EMERGENCY: High Temperature - Activating Cooling");
  
  // Turn off all heating immediately
  setHeater(true, false);
  setHeater(false, false);
  
  // Maximum ventilation
  setFan(true, true);   // Exhaust fan ON
  setFan(false, true);  // Circulation fan ON
  
  // Turn off grow lights (heat source)
  setLight(false);
}

void ActuatorManager::emergencySecurityBreach() {
  Serial.println("🚨 EMERGENCY: Security Breach Detected");
  
  // Turn on all lights
  setLight(true);
  
  // Activate alarm (if available)
  #ifdef BUZZER_PIN
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, 2000, 200);
    delay(300);
  }
  #endif
  
  // Continue normal environmental control
}

void ActuatorManager::emergencyWaterLeak() {
  Serial.println("💧 EMERGENCY: Water Leak - Disabling Irrigation");
  
  // Immediately stop irrigation
  setPump(false);
  
  // Keep other systems running
}

void ActuatorManager::emergencyPowerFailure() {
  Serial.println("⚡ EMERGENCY: Power Failure - UPS Mode");
  
  // Disable high-power consumers
  setHeater(true, false);
  setHeater(false, false);
  setLight(false);
  
  // Keep only critical sensors and minimal ventilation
  setFan(false, true);  // Circulation only
}

// ============================================================================
// WARNING LEVEL RESPONSES (Non-Emergency)
// ============================================================================

void ActuatorManager::handleWarning(AnomalyType type) {
  Serial.print("⚠️ WARNING: Anomaly type ");
  Serial.print(type);
  Serial.println(" - Adjusting controls");
  
  switch (type) {
    case TEMP_TOO_LOW:
      setHeater(true, true);
      break;
      
    case TEMP_TOO_HIGH:
      setFan(true, true);
      setLight(false);
      break;
      
    case HUMIDITY_TOO_LOW:
      // Reduce ventilation
      setFan(true, false);
      break;
      
    case HUMIDITY_TOO_HIGH:
      // Increase ventilation
      setFan(true, true);
      setFan(false, true);
      break;
      
    default:
      // No automated response for other anomaly types
      break;
  }
}

// ============================================================================
// SYSTEM CONTROL
// ============================================================================

void ActuatorManager::stopAll() {
  Serial.println("⏹ Stopping all actuators...");
  
  setHeater(true, false);
  setHeater(false, false);
  setFan(true, false);
  setFan(false, false);
  setPump(false);
  setLight(false);
  
  Serial.println("✓ All actuators stopped");
}

void ActuatorManager::printStatus() {
  Serial.println("\n=== Actuator Status ===");
  Serial.print("Heater Primary:    ");
  Serial.println(state.heaterPrimary ? "ON" : "OFF");
  Serial.print("Heater Secondary:  ");
  Serial.println(state.heaterSecondary ? "ON" : "OFF");
  Serial.print("Fan Exhaust:       ");
  Serial.println(state.fanExhaust ? "ON" : "OFF");
  Serial.print("Fan Circulation:   ");
  Serial.println(state.fanCirculation ? "ON" : "OFF");
  Serial.print("Irrigation Pump:   ");
  Serial.println(state.pumpIrrigation ? "ON" : "OFF");
  Serial.print("Grow Lights:       ");
  Serial.println(state.lightGrow ? "ON" : "OFF");
  
  // Show duty cycles
  unsigned long now = millis();
  if (state.heaterPrimary) {
    Serial.print("Heater run time:   ");
    Serial.print((now - state.heaterOnTime) / 1000);
    Serial.println(" seconds");
  }
  if (state.pumpIrrigation) {
    Serial.print("Pump run time:     ");
    Serial.print((now - state.pumpOnTime) / 1000);
    Serial.println(" seconds");
  }
  Serial.println();
}

// ============================================================================
// GETTERS FOR STATE
// ============================================================================

bool ActuatorManager::isHeaterOn(bool primary) {
  return primary ? state.heaterPrimary : state.heaterSecondary;
}

bool ActuatorManager::isFanOn(bool exhaust) {
  return exhaust ? state.fanExhaust : state.fanCirculation;
}

bool ActuatorManager::isPumpOn() {
  return state.pumpIrrigation;
}

bool ActuatorManager::isLightOn() {
  return state.lightGrow;
}
