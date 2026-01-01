# GreenOS - Hardware Implementation Guide

## ESP32-WROOM-32E Implementation Plan

This document provides the detailed implementation plan for the GreenOS hardware interface based on the **ESP32-WROOM-32E** platform and your specific sensor hardware.

---

##  Completed Implementations

### 1. Enhanced Configuration System
- ✅ Complete hardware pin mappings for ESP32-WROOM-32E (3.3V logic)
- ✅ All sensor thresholds with optimal ranges
- ✅ Altitude compensation for Denver (5280 ft)
- ✅ SPIFFS for offline data buffering (replaces SD card)
- ✅ System state machine enumerations
- ✅ Timing intervals and safety limits
- ✅ Hardware Watchdog Timer (30-second timeout)

**File**: `Firmware/include/config.h`

### 2. Advanced Sensor Manager
- ✅ SCD-30 CO2/Temperature/Humidity integration (I2C)
- ✅ MQ135 Air Quality sensor with voltage divider compensation
- ✅ Modbus RS485 soil EC/pH/moisture sensor (NPK included)
- ✅ Sensor health monitoring with error rate tracking
- ✅ ADC calibration with multi-point correction
- ✅ Temperature compensation for ADC drift
- ✅ Sanity checking and fallback to last known good values
- ✅ CRC32 data integrity validation

**Files**: `Firmware/include/sensor_manager.h`, `Firmware/src/sensor_manager.cpp`

### 3. Robust Main Firmware
- ✅ Finite State Machine (FSM) for system control
- ✅ Hardware Watchdog Timer (ESP32 WDT, 30-second timeout)
- ✅ SPIFFS buffering for offline operation
- ✅ WiFi reconnection with non-blocking timeout
- ✅ FreeRTOS dual-core task management
- ✅ Memory health monitoring
- ✅ Emergency and safe-mode protocols
- ✅ Serial command interface for diagnostics
- ✅ Sensor health periodic checks

**File**: `Firmware/src/main.cpp`

### 4. Safety-Enhanced Actuator Control
- ✅ Relay state tracking and validation
- ✅ Minimum cycle time enforcement (60s)
- ✅ Safety interlocks (heater/exhaust fan conflict prevention)
- ✅ Duty cycle limiting
- ✅ Emergency protocols for all failure modes
- ✅ Warning-level automated responses

**Files**: `Firmware/include/actuator_manager.h`, `Firmware/src/actuator_manager.cpp`

---

##  Hardware Wiring Details

### Critical Notes for ESP32-WROOM-32E
- ⚠️ **ALL GPIO pins are 3.3V logic** - DO NOT connect 5V signals directly!
- ✅ Power supply: USB provides 5V to board (or external 5V DC)
- ✅ Use level shifters or voltage dividers for 5V sensors
- ✅ ADC1 pins only (GPIO 32-39) - ADC2 conflicts with WiFi

### I2C Bus (SCD-30 CO2 Sensor)

```
SCD-30              ESP32-WROOM-32E
───────────────────────────────────
VCC      ────────>   3.3V or 5V (SCD-30 supports both)
GND      ────────>   GND
SDA      ────────>   GPIO 21 (I2C SDA)
SCL      ────────>   GPIO 22 (I2C SCL)
```

**Notes:**
- SCD-30 operates at 3.3V or 5V
- Uses I2C address 0x61
- Enable altitude compensation in firmware (already configured for 1609m)

---

### Modbus RS485 (Soil EC/pH Sensor)

**Hardware**: MAX485 TTL to RS485 Transceiver Module

```
MAX485              ESP32-WROOM-32E         Soil Sensor
───────────────────────────────────────────────────────
VCC      ────────>   5V (or 3.3V if module supports)
GND      ────────>   GND
RO       ────────>   GPIO 16 (UART2 RX)
DI       ────────>   GPIO 17 (UART2 TX)
DE/RE    ────────>   GPIO 4 (RS485 Driver Enable)
A        ────────────────────────────────>  A (Yellow wire)
B        ────────────────────────────────>  B (Blue wire)
```

**Soil Sensor Wiring** (S-Soil MT-02):
- Red: +12V to +24V DC power
- Black: GND
- Yellow: A (RS485+)
- Blue: B (RS485-)

**Notes:**
- Baud rate: 4800 (configured in firmware)
- Default slave ID: 0x01
- Modbus RTU protocol
- DE/RE pins tied together (both control transmit/receive mode)

**Register Map** (from datasheet):
```
Register    Parameter           Unit        Resolution
0x0000      Soil Moisture       %           0.1%
0x0001      Soil Temperature    °C          0.1°C
0x0002      EC                  µS/cm       1 µS/cm
0x0003      pH                  pH          0.01 pH
0x0004      Nitrogen (N)        mg/kg       1 mg/kg
0x0005      Phosphorus (P)      mg/kg       1 mg/kg
0x0006      Potassium (K)       mg/kg       1 mg/kg
```

---

### MQ135 Air Quality Sensor (Analog)

**⚠️ CRITICAL: Voltage Divider Required!**

MQ135 outputs 0-5V, but Arduino UNO Q ADC only accepts 0-3.3V.

**Voltage Divider Circuit:**
```
MQ135 AOUT
    │
    ├── R1 (10kΩ)
    │
    ├──────────> Arduino A0 (Pin A0)
    │
    ├── R2 (20kΩ)
    │
   GND
```

**Calculation:**
- Output voltage = 5V × (R2/(R1+R2)) = 5V × (20kΩ/30kΩ) = 3.33V max ✅

**Wiring:**
```
MQ135               Voltage Divider        Arduino UNO Q
────────────────────────────────────────────────────────
VCC      ────────>   5V                     5V rail
GND      ────────>   GND                    GND
AOUT     ────────>   Input ──> Output ───>  A0
DOUT     ────────>   (not used)
```

**Alternative (Simpler)**: Use I2C Logic Level Converter
- Your I2C level converter (3.3V ↔ 5V) won't work for analog signals
- Stick with voltage divider for MQ135

**Notes:**
- Preheat time: 48 hours for accurate readings
- Load resistor: 10kΩ (typically on-board)
- Requires calibration in clean air (see firmware calibration mode)

---

### SPIFFS (Internal Flash Storage)

The ESP32-WROOM-32E uses **SPIFFS** (SPI Flash File System) for offline data buffering instead of an external SD card.

**Advantages over SD Card:**
- No additional hardware required
- More reliable (no card to fail or corrupt)
- Faster read/write operations
- Up to 500 readings buffered in internal flash

**Notes:**
- Partition size: ~1.5MB for SPIFFS
- Automatic wear leveling built-in
- Data persists across power cycles

---

### Digital Sensors

#### PIR Motion Sensor
```
PIR Sensor          ESP32-WROOM-32E
──────────────────────────────────
VCC      ────────>   5V (or 3.3V)
GND      ────────>   GND
OUT      ────────>   GPIO 27 (with level shifter if 5V output!)
```

**⚠️ Check your PIR output voltage!**
- If 5V output: Use level shifter
- If 3.3V output: Direct connection OK

#### UPS Status Monitor
```
UPS Module          ESP32-WROOM-32E
──────────────────────────────────
Status   ────────>   GPIO 26 (active-low, internal pullup enabled)
GND      ────────>   GND
```

---

### Relay Modules (Actuators)

**Hardware**: 5V Optoisolated Relay Board (15A rating)

```
Relay Module        ESP32-WROOM-32E        Load (120VAC)
──────────────────────────────────────────────────────────
VCC      ────────>   5V (external supply)
GND      ────────>   GND
IN1      ────────>   GPIO 13 (Heater Primary)    ──> 1500W Heater
IN2      ────────>   GPIO 12 (Heater Secondary)  ──> 1500W Heater
IN3      ────────>   GPIO 14 (Fan Exhaust)       ──> Exhaust Fan
IN4      ────────>   GPIO 27 (Fan Circulation)   ──> Circulation Fan
IN5      ────────>   GPIO 26 (Pump Irrigation)   ──> Water Pump
IN6      ────────>   GPIO 25 (Grow Lights)       ──> Grow Lights
```

**⚠️ Important Relay Notes:**
1. **Optoisolated**: Control circuit (3.3V Arduino) is isolated from high voltage (120VAC)
2. **Active Level**: Most relay boards are active-LOW (LOW = ON). Verify yours!
   - If active-LOW: Modify firmware (invert HIGH/LOW in digitalWrite calls)
3. **Current Rating**: Ensure relay can handle load current
   - 1500W heater @ 120VAC = 12.5A (OK for 15A relay)
4. **Snubber circuits**: Consider RC snubber for inductive loads (motors, pumps)

---

##  Power Distribution Plan

### Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| ESP32-WROOM-32E | 5V USB or DC | ~240mA (WiFi active) | Main MCU power |
| SCD-30 CO2 Sensor | 3.3V-5V | 19mA avg | From ESP32 3.3V or 5V |
| MQ135 | 5V | 150mA | Separate 5V supply recommended |
| MAX485 Transceiver | 3.3V or 5V | 10mA | From ESP32 3.3V |
| Soil Sensor | 12-24V DC | 50mA | Separate DC supply |
| PIR Sensor | 5V or 3.3V | 50mA | From ESP32 or external |
| Relay Board | 5V | 70mA/relay | Separate 5V supply |

### Recommended Power Architecture

```
120VAC ───> Multiple 5V DC Adapters
             │
             ├─> 5V/2A ──> ESP32-WROOM-32E (USB or VIN)
             │              │
             │              ├─> 3.3V pin ──> SCD-30, MAX485
             │              └─> SPIFFS (internal, no external power)
             │
             ├─> 5V/1A ──> MQ135 Sensor (high power during preheat)
             │
             ├─> 5V/2A ──> Relay Board (6-channel)
             │
             └─> 12V/1A ──> Modbus Soil Sensor

Note: All grounds must be common (star ground configuration)
```

**⚠️ DO NOT power all sensors from ESP32 3.3V pin!**
- ESP32 3.3V pin max current: ~500mA
- Use external power supplies for high-current devices

---

##  Calibration Procedures

### ADC Calibration (One-time setup)

1. Connect to Serial Monitor (115200 baud)
2. Press **'c'** to enter calibration mode
3. Select option **1** (ADC Calibration)
4. Follow prompts:
   - **Step 1**: Connect ADC pin (GPIO 34) to GND, press Enter
   - **Step 2**: Connect ADC pin to known voltage source (e.g., 2.5V reference), enter voltage
   - **Step 3**: Firmware measures Vref automatically
5. Calibration saved to SPIFFS
6. Verify: Read ADC pins and compare to multimeter

### MQ135 Calibration (After 48-hour preheat)

1. Wait for MQ135 preheat (firmware shows countdown)
2. Place sensor in **clean outdoor air** for 30 minutes
3. Press **'c'** to enter calibration mode
4. Select option **2** (MQ135 Calibration)
5. Follow prompts
6. R0 value saved to SPIFFS

**Note**: Without known gas concentrations, MQ135 will give relative readings (suitable for trend analysis)

---

##  Testing & Validation

### Pre-Power-On Checklist

- [ ] Verify all I2C pullup resistors (usually on sensor boards)
- [ ] Check voltage divider for MQ135 (measure with multimeter: should be ≤3.3V)
- [ ] Confirm relay board active level (test with LED on control pins)
- [ ] Ensure all grounds are connected (common ground)
- [ ] Verify 120VAC wiring is correct and isolated (heaters, fans)
- [ ] Test UPS failover manually
- [ ] Confirm using ADC1 pins only (GPIO 32-39) - ADC2 conflicts with WiFi

### Initial Power-On Sequence

1. **Power ESP32-WROOM-32E only** (no sensors connected)
   - Build and upload firmware via PlatformIO
   - Open Serial Monitor (115200 baud)
   - Verify watchdog timer doesn't reset

2. **Add sensors one-by-one**:
   - Connect SCD-30 → Check I2C detection
   - Connect Modbus sensor → Check communication
   - Connect MQ135 → Check ADC readings on GPIO 34

3. **Test actuators** (NO LOAD):
   - Disconnect heaters/fans from relay outputs
   - Test each relay with LED or multimeter
   - Verify safety interlocks (heater + exhaust fan conflict)

4. **Full system test**:
   - Connect low-power loads first (fans, lights)
   - Test high-power loads (heaters) with current monitoring
   - Verify emergency protocols (unplug temperature sensor → emergency heat)

---

##  Debugging Tools

### Serial Commands (Built-in)

Press these keys in Serial Monitor:

| Key | Function |
|-----|----------|
| **s** | Read and print all sensor values |
| **h** | Show sensor health report |
| **c** | Enter calibration mode |
| **r** | Reset system (soft reboot) |

### I2C Scanner (Optional)

If SCD-30 not detected, upload I2C scanner sketch:

```cpp
#include <Wire.h>

void setup() {
  Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22 for ESP32
  Serial.begin(115200);
  Serial.println("I2C Scanner");
}

void loop() {
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", i);
    }
  }
  delay(5000);
}
```

Expected output: `Found device at 0x61` (SCD-30)

---

## ⚠️ Safety Recommendations

### Electrical Safety
1. **NEVER work on 120VAC wiring with power on**
2. Use GFCI outlets for greenhouse power
3. Keep high-voltage (relay outputs) physically separate from low-voltage (sensors)
4. Use proper wire gauge for heater loads (15A = 14 AWG minimum)
5. Consider adding fuses on high-power circuits

### Firmware Safety
1. **Watchdog timer**: Always enabled in production (prevents firmware hangs)
2. **Safe-fail defaults**: System defaults to OFF for all actuators on boot
3. **Emergency protocols**: Low temp → auto-heat, high temp → auto-cool
4. **Interlock enforcement**: Firmware prevents dangerous combinations (e.g., heat + exhaust)

### Environmental Safety
1. **UPS backup**: Maintain critical systems during brief power outages
2. **Temperature monitoring**: Redundant temperature sensors recommended
3. **Alerts**: Configure Firebase alerts for critical events
4. **Manual override**: Always maintain physical switches for critical equipment

---

##  Next Steps for Production Deployment

### Immediate (Week 1-2)
- [ ] Assemble breadboard prototype with all sensors
- [ ] Verify each sensor independently
- [ ] Run ADC and MQ135 calibration
- [ ] Test offline buffering (disconnect WiFi, verify SD writes)
- [ ] Validate watchdog timer (hang firmware, confirm auto-reboot)

### Short-term (Week 3-4)
- [ ] Install in greenhouse environment
- [ ] Monitor sensor readings for 48 hours
- [ ] Validate threshold values for your specific conditions
- [ ] Test all emergency protocols with simulated failures
- [ ] Configure Firebase alerts and test delivery

### Medium-term (Month 2)
- [ ] Design custom PCB (optional, for reliability)
- [ ] Add redundant critical sensors (2x temperature)
- [ ] Implement OTA firmware updates (deferred for now per your request)
- [ ] Long-term data analysis and ML model training
- [ ] Seasonal optimization (different thresholds for summer/winter)

---

##  Troubleshooting Common Issues

### Issue 1: SCD-30 Not Detected
**Symptoms**: Firmware reports "SCD-30 initialization failed"
**Causes**:
- I2C wiring incorrect
- Wrong I2C address
- Sensor not powered

**Solutions**:
1. Run I2C scanner (see Debugging Tools section)
2. Check SDA/SCL wiring (GPIO 21/22 for ESP32)
3. Verify power supply (3.3V or 5V)
4. Check I2C pullup resistors (usually 4.7kΩ on SCD-30 board)

### Issue 2: Modbus Timeout Errors
**Symptoms**: Firmware reports Modbus error codes (0xE0-0xE4)
**Causes**:
- Baud rate mismatch
- A/B polarity reversed
- DE/RE pin not toggling

**Solutions**:
1. Verify baud rate: 4800 (hardcoded in firmware)
2. Swap A and B wires if all reads fail
3. Check DE/RE pin connection (should go HIGH during transmit)
4. Measure voltage on A/B lines with oscilloscope

### Issue 3: MQ135 Readings Unstable
**Symptoms**: Air quality PPM jumps erratically
**Causes**:
- Not preheated (48 hours required)
- Voltage divider incorrect
- Not calibrated

**Solutions**:
1. Wait for full preheat cycle
2. Verify voltage divider: R1=10kΩ, R2=20kΩ
3. Run MQ135 calibration in clean air
4. Check if ADC readings exceed 3.3V (damage if so!)

### Issue 4: Watchdog Resets System
**Symptoms**: System reboots unexpectedly every 30 seconds
**Causes**:
- Blocking code (e.g., infinite loop in sensor read)
- Firebase timeout too long
- Missing watchdog feed in long operations

**Solutions**:
1. Check Serial Monitor for "Watchdog reset" message
2. Ensure watchdog is fed regularly in long operations
3. Reduce Firebase timeout
4. Check for infinite loops or blocking code

### Issue 5: Low Memory Warnings
**Symptoms**: "Low memory detected!" in Serial Monitor
**Causes**:
- Too much data buffering in SPIFFS
- Memory leaks in Firebase library
- Large JSON documents

**Solutions**:
1. Reduce MAX_BUFFERED_READINGS (currently 500)
2. Flush SPIFFS buffer more frequently
3. Restart system periodically (daily auto-reboot)
4. Check for memory leaks with heap monitoring (ESP32 has 520KB SRAM)

---

##  Additional Documentation

- **[LIBRARIES.md](LIBRARIES.md)**: Complete list of required Arduino libraries
- **[README.md](../README.md)**: Project overview and architecture
- **Datasheets**: See `Docs/` folder for sensor datasheets

---

##  Pro Tips

1. **Use labeled wires**: Color-code by function (red=5V, black=GND, yellow=I2C, etc.)
2. **Ferrite beads**: Add to long sensor cables to reduce EMI
3. **Decoupling capacitors**: 0.1µF near each sensor power pin
4. **Test incrementally**: Never add all sensors at once
5. **Keep notes**: Document calibration values, wiring changes, issues

---

## Questions or Issues?

If you encounter problems not covered here:
1. Check Serial Monitor output (most informative)
2. Use serial commands ('h' for health, 's' for sensor readings)
3. Verify hardware wiring against this document
4. Check sensor datasheets for specific requirements

---

**Last Updated**: December 31, 2025
**Firmware Version**: v2.0.0-dev
**Platform**: ESP32-WROOM-32E
**Author**: Pat Ryan
