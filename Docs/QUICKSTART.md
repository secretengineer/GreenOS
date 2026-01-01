# GreenOS - Quick Start Guide

##  Get Up and Running in 30 Minutes

This quick start guide gets your ESP32-WROOM-32E running with GreenOS firmware for initial testing.

---

##  Prerequisites (5 minutes)

### Hardware Ready
- [ ] ESP32-WROOM-32E development board with USB cable
- [ ] Computer with VS Code installed
- [ ] At least SCD-30 sensor connected (for testing)

### Software Ready
- [ ] VS Code installed
- [ ] PlatformIO IDE extension installed

---

##  Step 1: Install PlatformIO (5 minutes)

1. Open **VS Code**
2. Go to **Extensions** (Ctrl+Shift+X)
3. Search for **"PlatformIO IDE"**
4. Click **Install**
5. Wait for installation to complete
6. Restart VS Code when prompted
7. Open the **GreenOS/Firmware** folder in VS Code
8. PlatformIO will automatically detect `platformio.ini`

---

##  Step 2: Install Libraries (Automatic!)

PlatformIO automatically installs all dependencies when you build. No manual steps needed!

**Libraries installed automatically via `platformio.ini`:**
1. SparkFun SCD30 Arduino Library
2. ModbusMaster
3. ArduinoJson v6.x (**NOT v7**)
4. Firebase Arduino Client Library
5. WiFiManager

**Verification**: Click **PlatformIO Home** → **Libraries** tab to see installed libraries.

---

##  Step 3: Configure WiFi (5 minutes)

1. Open `Firmware/include/config.h` in VS Code
2. Update WiFi credentials with **YOUR** network details:

```cpp
// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
```

3. **Save** the file (Ctrl+S)

**Note**: Firebase configuration is optional for initial testing. The system will work without cloud connectivity using SPIFFS for local data buffering.

---

##  Step 4: Upload Firmware (5 minutes)

1. Connect ESP32-WROOM-32E via USB
2. In VS Code with PlatformIO:
   - Click the **PlatformIO icon** in the left sidebar
   - Under **PROJECT TASKS** → **esp32dev**:
   - Click **Build** (compiles firmware)
   - Click **Upload** (flashes to ESP32)
3. Wait for upload to complete (~1-2 minutes)
4. Look for "**SUCCESS**" in the terminal

**Alternative CLI:**
```powershell
cd Firmware
pio run --target upload
```

---

##  Step 5: First Boot (5 minutes)

1. Click **Monitor** in PlatformIO (or use `pio device monitor`)
2. Baud rate is automatically set to **115200**
3. You should see:

```
╔════════════════════════════════════════╗
║   GreenOS - Intelligent Greenhouse     ║
║   ESP32-WROOM-32E Firmware v2.0        ║
╚════════════════════════════════════════╝

Initializing Hardware Watchdog Timer...
✓ Hardware watchdog enabled (30 second timeout)
Initializing SPIFFS... ✓ SPIFFS mounted
ℹ️  Offline buffering enabled (up to 500 readings)

[STATE] Initializing Sensors...
=== Initializing Sensors ===
✓ SCD-30 CO2 sensor initialized
  Altitude compensation: 1609 meters
✓ Modbus RS485 initialized (UART2)
...
```

4. Watch for successful initialization messages

---

##  Verify Operation

### Test Sensor Readings
1. Wait 30 seconds for first readings
2. Press **'s'** in Serial Monitor
3. You should see:

```
=== Sensor Readings ===
--- Environmental ---
Air Temp:     22.4 °C
Air Humidity: 45.2 %
CO2:          487 ppm
...
```

### Test Health Monitoring
1. Press **'h'** in Serial Monitor
2. Check sensor health status:

```
=== Sensor Health Report ===
SCD-30:  OK (Error: 0.0%)
MQ135:   FAIL (Error: 0.0%, Preheated: No)
Modbus:  FAIL (Error: 100.0%)
```

**Note**: MQ135 and Modbus will show errors if not connected - this is normal!

---

##  Serial Commands

Press these keys in Serial Monitor for quick actions:

| Key | Action |
|-----|--------|
| **s** | Show all sensor readings |
| **h** | Display sensor health report |
| **c** | Enter calibration mode |
| **r** | Reset system (reboot) |

---

##  Minimal Test Setup

For initial testing, you only need:

### Option 1: SCD-30 Only (Simplest)
```
SCD-30          ESP32-WROOM-32E
────────────────────────────
VCC    ──────>   3.3V or 5V
GND    ──────>   GND
SDA    ──────>   GPIO 21
SCL    ──────>   GPIO 22
```

**Result**: You'll get CO2, temperature, and humidity readings!

### Option 2: Add MQ135 (With Voltage Divider!)
```
MQ135 AOUT ──[10kΩ]──┬──> ESP32 GPIO 34 (ADC1_CH6)
                      │
                   [20kΩ]
                      │
                     GND

MQ135 VCC ──────> 5V
MQ135 GND ──────> GND
```

**⚠️ WARNING**: Without the voltage divider, you'll damage the ESP32!
**Note**: Use ADC1 pins only (GPIO 32-39) - ADC2 conflicts with WiFi

---

##  Common First-Time Issues

### Issue: "Upload failed"
**Solution**: 
- Check ESP32 is connected via USB
- Try a different USB port or cable
- Press and hold BOOT button on ESP32 while uploading
- Check PlatformIO terminal for specific error

### Issue: "SCD-30 initialization failed"
**Solution**:
- Verify I2C wiring (GPIO 21=SDA, GPIO 22=SCL)
- Check SDA/SCL not swapped
- Ensure SCD-30 is powered (3.3V or 5V)

### Issue: "WiFi connection timeout"
**Solution**:
- Verify SSID and password in `Firmware/include/config.h`
- Check WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Move ESP32 closer to router
- System will continue in offline mode (safe to proceed)

### Issue: Watchdog keeps resetting every 30 seconds
**Solution**:
- This means firmware is hanging somewhere
- Check Serial Monitor for error messages before reset
- Check for blocking code or infinite loops
- Report issue with Serial Monitor output

### Issue: Compile errors about missing libraries
**Solution**:
- Run `pio pkg install` in the Firmware directory
- Check `platformio.ini` for correct lib_deps
- Delete `.pio` folder and rebuild

---

##  Expected Behavior

### Normal Operation
- Status LED blinks **once slowly** (1 second) when entering normal operation
- Serial Monitor shows sensor readings every 5 seconds
- No watchdog resets (30-second timeout)
- WiFi connected (or "Operating in offline mode")
- FreeRTOS tasks running on both cores

### Safe Mode (if sensors fail)
- Status LED blinks **rapidly** (5 quick blinks)
- Serial shows: "SAFE MODE ACTIVATED"
- System maintains critical protection only

### Emergency Mode (simulated failure)
- Status LED flashes rapidly
- Serial shows: "EMERGENCY MODE ACTIVATED"
- Actuators respond to emergency type

---

##  Next Steps After Successful Boot

1. **Let It Run**: Monitor for 1 hour, ensure no watchdog resets
2. **Add Sensors**: Add MQ135 (GPIO 34), Modbus sensor one-by-one
3. **Calibrate**: Run calibration procedures (press 'c')
4. **Test Offline**: Disconnect WiFi, verify SPIFFS buffering
5. **Connect Actuators**: Add relay board and test (no high-voltage loads yet!)

---

##  Where to Go From Here

### Read Full Documentation
- **HARDWARE_SETUP.md**: Complete wiring guide
- **LIBRARIES.md**: Detailed library information
- **IMPLEMENTATION_SUMMARY.md**: Full feature overview

### Test Additional Features
- SD card buffering (add SD card module)
- Modbus sensor (add MAX485 and soil sensor)
- Actuator control (add relay board)
- Emergency protocols (simulate temperature failure)

### Prepare for Deployment
- Run for 24 hours on breadboard
- Verify all sensor readings are reasonable
- Test all actuators (low power first)
- Create permanent wiring (consider PCB)

---

##  Pro Tips

1. **Start Small**: Don't connect everything at once - add sensors incrementally
2. **Monitor Serial**: Always have Serial Monitor open during testing
3. **Save Output**: Copy-paste interesting Serial Monitor output for reference
4. **Label Wires**: Use colored tape or labels (saves hours of debugging!)
5. **Take Photos**: Photograph your wiring before disconnecting anything

---

##  Need Help?

### Check These First
1. Serial Monitor output (most informative!)
2. Serial command 'h' for health report
3. Serial command 's' for sensor readings
4. LED blink pattern (1 slow = OK, 5 fast = safe mode)

### Common Debug Commands
```cpp
// In Serial Monitor:
s    // Show sensor readings
h    // Show health report
c    // Calibration mode (if needed)
r    // Restart system
```

### Documentation
- `HARDWARE_SETUP.md` - Troubleshooting section
- `IMPLEMENTATION_SUMMARY.md` - Known issues
- Sensor datasheets in `Docs/` folder

---

##  Success Criteria

You've successfully completed quick start if:

- [✅] Firmware uploads without errors
- [✅] Serial Monitor shows boot sequence
- [✅] At least one sensor (SCD-30) is reading
- [✅] No watchdog resets for 5+ minutes
- [✅] Can send serial commands ('s', 'h')
- [✅] System runs continuously

---

**Congratulations! Your GreenOS system is now operational!** 🌱

Time to start building out the full sensor suite and testing in your greenhouse environment.

---

**Last Updated**: December 31, 2025
**Platform**: ESP32-WROOM-32E
**Estimated Time**: 30 minutes (first time)
**Difficulty**: Beginner-Intermediate
