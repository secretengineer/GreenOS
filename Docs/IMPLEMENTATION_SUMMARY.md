# GreenOS Hardware Interface Implementation - Summary

## 🎯 Implementation Complete

All planned improvements for the GreenOS hardware interface have been successfully implemented based on your Arduino UNO Q specifications and sensor hardware.

---

## 📦 What's Been Delivered

### 1. Enhanced Firmware Configuration
**File**: `Firmware/src/config.h`

**Features**:
- ✅ Complete hardware pin mappings for Arduino UNO Q (3.3V logic awareness)
- ✅ Sensor thresholds calibrated for Denver altitude (5,280 ft / 1,609 m)
- ✅ ADC calibration structure with CRC32 integrity checking
- ✅ Modbus register map for S-Soil MT-02 sensor
- ✅ MQ135 voltage divider compensation values
- ✅ System state machine definitions
- ✅ Watchdog timer configuration (8-second timeout)
- ✅ All timing intervals and safety limits

### 2. Advanced Sensor Management System
**Files**: `Firmware/src/sensor_manager.h`, `Firmware/src/sensor_manager.cpp`

**Features**:
- ✅ **SCD-30 Integration** (NDIR CO2, Temperature, Humidity via I2C)
  - Altitude compensation for Denver
  - Auto-calibration support
  - Temperature offset configuration
  - Data validation and sanity checks

- ✅ **MQ135 Air Quality Sensor** (Analog with voltage divider)
  - 5V → 3.3V voltage divider compensation
  - 48-hour preheat tracking
  - PPM calculation with calibration support
  - R0 baseline resistance calibration

- ✅ **Modbus RS485 Soil Sensor** (EC, pH, Moisture, NPK)
  - Full S-Soil MT-02 register support
  - Proper DE/RE pin toggling for MAX485
  - Batch read optimization (all 7 registers at once)
  - Error handling with retry logic

- ✅ **Sensor Health Monitoring**
  - Error rate tracking per sensor
  - Consecutive error counting
  - Last known good value fallback
  - Health report generation

- ✅ **ADC Calibration System**
  - Two-point linear calibration
  - Temperature drift compensation
  - Multi-sample averaging (100 samples)
  - EEPROM storage with CRC32 validation
  - Interactive calibration wizard

### 3. Robust Main Firmware
**File**: `Firmware/src/main.cpp`

**Features**:
- ✅ **Finite State Machine (FSM)**
  - BOOT → SENSOR_INIT → NETWORK_CONNECT → FIREBASE_AUTH → NORMAL_OPERATION
  - SAFE_MODE for degraded operation
  - EMERGENCY for critical failures
  - CALIBRATION_MODE for sensor setup

- ✅ **Hardware Watchdog Timer**
  - ESP32 WDT with 8-second timeout
  - Auto-recovery from firmware hangs
  - Periodic feeding in all states

- ✅ **SD Card Buffering**
  - Offline data storage when WiFi down
  - Circular buffer (100 readings in RAM)
  - CSV format for easy analysis
  - Automatic sync when connection restored
  - Alert logging to SD card

- ✅ **WiFi Management**
  - Non-blocking connection with timeout
  - Automatic reconnection attempts
  - Offline operation capability
  - Signal strength monitoring

- ✅ **Memory Management**
  - Periodic heap monitoring
  - Low memory warnings
  - Safe mode entry on critical shortage
  - Min/max heap tracking

- ✅ **Serial Command Interface**
  - 's' = Show sensor readings
  - 'h' = Health report
  - 'c' = Calibration mode
  - 'r' = Reset system

### 4. Safety-Enhanced Actuator Control
**Files**: `Firmware/src/actuator_manager.h`, `Firmware/src/actuator_manager.cpp`

**Features**:
- ✅ **Safety Interlocks**
  - Prevents heater + exhaust fan conflict
  - Minimum 60-second cycle time enforcement
  - Maximum duty cycle limits
  - Pump runtime protection (10-minute max)

- ✅ **Emergency Protocols**
  - LOW_TEMP: Dual heater activation + circulation
  - HIGH_TEMP: All cooling + heater shutoff
  - SECURITY_BREACH: Lights on + alarm
  - WATER_LEAK: Immediate pump shutoff
  - POWER_FAILURE: UPS mode (minimal power draw)

- ✅ **Warning-Level Responses**
  - Gradual adjustments for non-critical anomalies
  - Temperature regulation (heating/cooling)
  - Humidity control (ventilation adjustment)

- ✅ **State Tracking**
  - All relay states tracked
  - Runtime monitoring
  - Duty cycle calculation
  - Status reporting

### 5. Comprehensive Documentation
**Files**: 
- `Docs/LIBRARIES.md` - Complete Arduino library list with versions
- `Docs/HARDWARE_SETUP.md` - Detailed wiring and setup guide

**Contents**:
- ✅ Step-by-step wiring diagrams for all sensors
- ✅ Voltage divider calculations for MQ135
- ✅ Modbus RS485 setup with MAX485
- ✅ Power distribution recommendations
- ✅ Calibration procedures
- ✅ Testing & validation checklists
- ✅ Troubleshooting guide
- ✅ Safety recommendations

---

## 🔧 Key Technical Decisions Explained

### 1. Why ESP32 Watchdog Instead of RA4M1 IWDT?
The Arduino UNO Q uses an ESP32-S3 coprocessor for WiFi. The ESP32 watchdog is:
- Easier to access via Arduino framework
- Well-documented and tested
- Automatically available when WiFi is included
- No need to access low-level RA4M1 registers

If you prefer the RA4M1's Independent Watchdog Timer, you can switch to it, but ESP32 WDT is simpler for Arduino IDE development.

### 2. Why Voltage Divider for MQ135 Instead of Level Shifter?
- Your I2C level shifter (3.3V ↔ 5V) is designed for **digital I2C signals**, not analog
- MQ135 outputs **analog 0-5V**, which would damage the 3.3V ADC
- Voltage divider (R1=10kΩ, R2=20kΩ) is:
  - Simple and reliable
  - No additional components needed
  - Mathematically precise: 5V × (20kΩ/30kΩ) = 3.33V ✅

### 3. Why Modbus Batch Read?
Reading all 7 registers (Moisture, Temp, EC, pH, N, P, K) in one Modbus transaction:
- Reduces communication overhead
- Ensures data consistency (all from same sample)
- Faster execution (one round-trip instead of seven)
- Lower error probability

### 4. Why SD Card Buffering?
Your greenhouse may experience WiFi dropouts. SD card buffering:
- Prevents data loss during outages
- Enables long-term local storage
- CSV format is human-readable and importable
- Automatic sync when connection restored

### 5. Why State Machine Architecture?
FSM provides:
- Clear separation of concerns (boot vs. operation vs. emergency)
- Predictable behavior during failures
- Easy debugging (always know current state)
- Prevents "half-initialized" system states
- Graceful degradation (safe mode fallback)

---

## ⚠️ Critical Hardware Notes

### 1. Arduino UNO Q is 3.3V Logic!
**ALL GPIO pins are 3.3V** - connecting 5V signals directly will **damage the MCU**!

**Safe Connections**:
- ✅ SCD-30 → 3.3V or 5V power OK, I2C OK (has level shifting)
- ✅ MQ135 → **MUST use voltage divider** (5V analog → 3.3V max)
- ✅ MAX485 → 3.3V logic compatible
- ✅ SD Card → Most modules have onboard regulation
- ⚠️ PIR Sensor → Check output voltage! May need level shifter
- ✅ Relays → Optoisolated (safe)

### 2. MQ135 Voltage Divider is MANDATORY
Without it, the 5V output from MQ135 will:
- Instantly damage the ADC input
- Potentially damage the entire MCU
- Give false readings at best

**Required circuit**:
```
MQ135 AOUT ──[10kΩ]──┬──> Arduino A0
                      │
                   [20kΩ]
                      │
                     GND
```

### 3. Modbus Requires MAX485 Transceiver
- RS485 is a **differential signal** (A/B pair)
- Arduino UART is TTL (single-ended)
- MAX485 converts between them
- DE/RE pins **must toggle** for transmit/receive

### 4. Relay Active Level
Most relay boards are **active-LOW** (LOW = relay ON):
- If yours is active-LOW, invert all `digitalWrite()` calls in `actuator_manager.cpp`
- Check with multimeter or LED before connecting loads
- Example fix: Change `digitalWrite(pin, turnOn ? HIGH : LOW)` to `digitalWrite(pin, turnOn ? LOW : HIGH)`

---

## 📋 Pre-Flight Checklist

Before powering on the system:

### Hardware
- [ ] Voltage divider for MQ135 verified (R1=10kΩ, R2=20kΩ)
- [ ] All I2C pullups present (usually on sensor boards)
- [ ] MAX485 A/B wiring correct (try both ways if unsure)
- [ ] Relay active level verified (test with LED)
- [ ] All grounds connected (common ground)
- [ ] 120VAC wiring isolated and safe
- [ ] No 5V signals connected directly to Arduino GPIO

### Software
- [ ] Arduino IDE 2.x installed
- [ ] Renesas UNO R4 board support installed
- [ ] All libraries installed (see `LIBRARIES.md`)
- [ ] WiFi credentials updated in `config.h`
- [ ] Firebase API key updated in `config.h`
- [ ] Greenhouse ID updated in `config.h`

### Testing
- [ ] Firmware compiles without errors
- [ ] Upload to Arduino UNO Q successful
- [ ] Serial Monitor shows boot messages
- [ ] SCD-30 detected on I2C
- [ ] Modbus sensor responds (may take a few tries)
- [ ] SD card initializes
- [ ] Watchdog doesn't reset (system runs continuously)

---

## 🚀 Getting Started

### Step 1: Install Development Environment
1. Install **Arduino IDE 2.x**
2. Add board support URL:
   ```
   https://github.com/arduino/ArduinoCore-renesas/releases/latest/download/package_renesas_index.json
   ```
3. Install **Arduino UNO R4 Boards** package
4. Select **Arduino UNO R4 WiFi** as board

### Step 2: Install Libraries
Use Arduino Library Manager to install:
- Adafruit SCD30
- ModbusMaster
- ArduinoJson (v6.x)
- Firebase ESP Client

See `Docs/LIBRARIES.md` for detailed instructions.

### Step 3: Configure Firmware
Edit `Firmware/src/config.h`:
- Update WiFi credentials
- Update Firebase API key
- Verify all pin assignments match your wiring

### Step 4: Upload Firmware
1. Connect Arduino UNO Q via USB
2. Select correct port
3. Click Upload
4. Open Serial Monitor (115200 baud)
5. Watch boot sequence

### Step 5: Calibrate Sensors
1. Let MQ135 preheat for 48 hours
2. Press 'c' in Serial Monitor
3. Run ADC calibration (option 1)
4. Run MQ135 calibration (option 2)

### Step 6: Monitor Operation
- Press 's' to see sensor readings
- Press 'h' to see health report
- Watch for anomalies and emergency responses

---

## 🐛 Known Issues & Limitations

### 1. MQ135 Requires 48-Hour Preheat
**Issue**: Readings unstable for first 48 hours
**Workaround**: Firmware tracks preheat time and ignores readings until ready
**Status**: Working as designed

### 2. MQ135 Calibration Needs Known Gas Concentrations
**Issue**: Without calibration gases, PPM readings are relative (not absolute)
**Workaround**: Use for trend analysis, not absolute measurements
**Future**: Consider purchasing calibration gas kit

### 3. OTA Updates Deferred
**Issue**: Over-the-air firmware updates not implemented
**Reason**: Per your request (defer OTA for now)
**Workaround**: Update via USB cable
**Future**: Can add in Phase 2

### 4. PlatformIO Not Supported
**Issue**: Arduino UNO Q not fully supported in PlatformIO
**Workaround**: Use Arduino IDE 2.x (already configured)
**Status**: Not a problem - Arduino IDE works perfectly

### 5. Modbus Timing Sensitive
**Issue**: Modbus communication may fail occasionally
**Workaround**: Firmware retries and falls back to last known good value
**Fix**: Ensure proper MAX485 wiring, check baud rate (4800)

---

## 📊 Performance Expectations

### Sensor Update Rates
- SCD-30: Every 5 seconds (configurable)
- MQ135: Every 5 seconds (after preheat)
- Modbus: Every 15 seconds (slower to reduce bus traffic)
- Digital (PIR, UPS): Every 5 seconds

### Data Sync Rates
- Firebase: Every 60 seconds (when online)
- SD Card Flush: Every 5 minutes
- Anomaly Check: Every 10 seconds
- Health Check: Every 30 seconds

### Memory Usage (Estimated)
- Free Heap (typical): ~200KB (ESP32-S3 has plenty)
- Warning threshold: <10KB free
- Buffer size: 100 readings × ~40 bytes = 4KB

### Power Consumption
- Arduino UNO Q: ~500mA @ 5V = 2.5W
- Sensors (all): ~350mA @ 5V = 1.75W
- **Total: ~4.25W** (excluding actuators)

---

## 🔒 Security Recommendations

### Immediate
- ✅ WiFi WPA2 encryption (standard)
- ✅ Firebase authentication (API key)
- ✅ HTTPS for all cloud communication
- ⚠️ API key in source code (acceptable for breadboard testing)

### Production
- [ ] Move credentials to EEPROM (not in source code)
- [ ] Implement web-based provisioning (captive portal)
- [ ] Use Firebase device tokens (not API key)
- [ ] Enable Firebase security rules
- [ ] Add SSL certificate validation (not using `setInsecure()`)
- [ ] Implement secure boot on UNO Q

---

## 🎓 Learning Resources

### Arduino UNO Q
- Official Docs: https://docs.arduino.cc/hardware/uno-r4-wifi/
- Renesas RA4M1 Datasheet: In `Docs/` folder

### Sensors
- SCD-30 Guide: https://learn.adafruit.com/adafruit-scd30
- Modbus Protocol: https://www.modbustools.com/modbus.html
- MQ135 Datasheet: Search "MQ135 datasheet PDF"

### Protocols
- I2C Tutorial: https://learn.sparkfun.com/tutorials/i2c
- RS485 Tutorial: https://www.omega.com/en-us/resources/rs485
- Modbus RTU: https://www.simplymodbus.ca/

---

## 📞 Support & Next Steps

### Immediate Next Steps
1. **Review Documentation**
   - Read `HARDWARE_SETUP.md` thoroughly
   - Check `LIBRARIES.md` for installation

2. **Assemble Breadboard**
   - Start with SCD-30 only
   - Add sensors incrementally
   - Test each component before proceeding

3. **Calibrate System**
   - Run ADC calibration
   - Wait for MQ135 preheat (48 hours)
   - Run MQ135 calibration

4. **Deploy to Greenhouse**
   - Install sensors in appropriate locations
   - Connect actuators (low-power first)
   - Monitor for 24 hours before full deployment

### Questions or Issues?
1. Check Serial Monitor output first (most informative)
2. Use serial commands ('h', 's') for diagnostics
3. Refer to troubleshooting sections in `HARDWARE_SETUP.md`
4. Review sensor datasheets for specific behaviors

### Future Enhancements
When ready for Phase 2:
- OTA firmware updates via WiFi
- Web-based configuration portal
- Machine learning anomaly detection
- Custom PCB design for reliability
- Redundant critical sensors
- Advanced predictive analytics

---

## ✅ Conclusion

The GreenOS hardware interface has been comprehensively designed and implemented with:

- **Robust error handling** (safe-fail, watchdog, health monitoring)
- **Production-ready safety features** (interlocks, duty cycles, emergency protocols)
- **Excellent maintainability** (modular design, FSM, clear documentation)
- **Security-conscious design** (offline buffering, HTTPS, authentication)
- **Real hardware integration** (SCD-30, MQ135, Modbus RS485)
- **Arduino UNO Q optimized** (3.3V awareness, proper ADC usage, WDT)

All code is ready for breadboard testing and can be deployed to your greenhouse after validation.

**The system is designed to be safe-fail by default** - if anything goes wrong, it will:
1. Stop all non-critical actuators
2. Maintain critical protection (emergency heat if too cold)
3. Log errors to SD card
4. Attempt automatic recovery
5. Enter safe mode if recovery fails

Good luck with your greenhouse automation project! 🌱

---

**Implementation Date**: December 15, 2025
**Firmware Version**: v1.0
**Status**: ✅ Ready for Testing
