# GreenOS - Quick Start Guide

## 🚀 Get Up and Running in 30 Minutes

This quick start guide gets your Arduino UNO Q running with GreenOS firmware for initial testing.

---

## ⚡ Prerequisites (5 minutes)

### Hardware Ready
- [ ] Arduino UNO Q with USB-C cable
- [ ] Computer with Arduino IDE 2.x installed
- [ ] At least SCD-30 sensor connected (for testing)

### Software Ready
- [ ] Arduino IDE 2.x installed
- [ ] Arduino UNO R4 board support installed

---

## 📦 Step 1: Install Board Support (5 minutes)

1. Open **Arduino IDE 2.x**
2. Go to **File → Preferences**
3. In "Additional Boards Manager URLs", add:
   ```
   https://github.com/arduino/ArduinoCore-renesas/releases/latest/download/package_renesas_index.json
   ```
4. Click **OK**
5. Go to **Tools → Board → Boards Manager**
6. Search for **"Renesas"** or **"UNO R4"**
7. Install **"Arduino UNO R4 Boards"**
8. Select **Tools → Board → Arduino UNO R4 → Arduino UNO R4 WiFi**

---

## 📚 Step 2: Install Libraries (10 minutes)

Open **Tools → Manage Libraries** and install these one-by-one:

1. Search **"Adafruit SCD30"** → Install latest
   - Will auto-install dependencies (Adafruit BusIO)
2. Search **"ModbusMaster"** → Install latest
3. Search **"ArduinoJson"** → Install v6.21.3 (or latest v6.x, **NOT v7**)
4. Search **"Firebase ESP Client"** → Install latest

**Verification**: Go to **Sketch → Include Library** and confirm all are listed.

---

## ⚙️ Step 3: Configure WiFi & Firebase (5 minutes)

1. Open `d:\GreenOS\Firmware\src\main\config.h` in Arduino IDE (or in any text editor)
2. Update these lines with **YOUR** credentials:

```cpp
// Line 15-16: Update WiFi
#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Line 19-20: Update Firebase (if you have different credentials)
#define FIREBASE_HOST "greenos-24311.firebaseapp.com"
#define API_KEY "AIzaSyAXSxzpXr0tkEgRdO8dJzufYec9yfu3cQI"
```

3. **Save** the file

---

## 📤 Step 4: Upload Firmware (5 minutes)

1. Connect Arduino UNO Q via USB-C
2. In Arduino IDE:
   - **File → Open** → Navigate to `d:\GreenOS\Firmware\src\main\main.ino`
3. Select correct port:
   - **Tools → Port** → Select the COM port with "UNO R4"
4. Click **Upload** button (→ icon)
5. Wait for compilation and upload (~2-3 minutes)
6. Look for "**Upload successful**"

---

## 🖥️ Step 5: First Boot (5 minutes)

1. Open **Serial Monitor** (icon in top-right, or Tools → Serial Monitor)
2. Set baud rate to **115200**
3. You should see:

```
╔════════════════════════════════════════╗
║   GreenOS - Intelligent Greenhouse     ║
║   Arduino UNO Q Firmware v1.0          ║
╚════════════════════════════════════════╝

Initializing Software Watchdog Timer...
✓ Software watchdog enabled (8 second timeout)
ℹ️  For production deployment, enable Renesas IWDT hardware watchdog
Initializing SD card... ✗ SD card initialization failed
⚠️ Offline buffering disabled

[STATE] Initializing Sensors...
=== Initializing Sensors ===
✓ SCD-30 CO2 sensor initialized
  Altitude compensation: 1609 meters
✓ Modbus RS485 initialized
...
```

4. Watch for successful initialization messages

---

## ✅ Verify Operation

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

## 🎛️ Serial Commands

Press these keys in Serial Monitor for quick actions:

| Key | Action |
|-----|--------|
| **s** | Show all sensor readings |
| **h** | Display sensor health report |
| **c** | Enter calibration mode |
| **r** | Reset system (reboot) |

---

## 🔧 Minimal Test Setup

For initial testing, you only need:

### Option 1: SCD-30 Only (Simplest)
```
SCD-30          Arduino UNO Q
────────────────────────────
VCC    ──────>   3.3V or 5V
GND    ──────>   GND
SDA    ──────>   Pin 20
SCL    ──────>   Pin 21
```

**Result**: You'll get CO2, temperature, and humidity readings!

### Option 2: Add MQ135 (With Voltage Divider!)
```
MQ135 AOUT ──[10kΩ]──┬──> Arduino A0
                      │
                   [20kΩ]
                      │
                     GND

MQ135 VCC ──────> 5V
MQ135 GND ──────> GND
```

**⚠️ WARNING**: Without the voltage divider, you'll damage the Arduino!

---

## 🐛 Common First-Time Issues

### Issue: "Upload failed"
**Solution**: 
- Check correct board selected (UNO R4 WiFi)
- Verify USB cable is data-capable (not charge-only)
- Try different USB port
- Press reset button on Arduino before upload

### Issue: "SCD-30 initialization failed"
**Solution**:
- Verify I2C wiring (pins 20, 21)
- Check SDA/SCL not swapped
- Ensure SCD-30 is powered (3.3V or 5V)

### Issue: "WiFi connection timeout"
**Solution**:
- Verify SSID and password in config.h
- Check WiFi network is 2.4GHz (not 5GHz - ESP32 doesn't support 5GHz)
- Move Arduino closer to router
- System will continue in offline mode (safe to proceed)

### Issue: Watchdog keeps resetting every 8 seconds
**Solution**:
- This means firmware is hanging somewhere
- Check Serial Monitor for error messages before reset
- Temporarily disable WDT: In config.h, change `#define WDT_ENABLED true` to `false`
- Report issue with Serial Monitor output

### Issue: Compile errors about missing libraries
**Solution**:
- Go back to Step 2 and install all libraries
- Make sure ArduinoJson is v6.x (not v7)
- Restart Arduino IDE after installing libraries

---

## 📊 Expected Behavior

### Normal Operation
- Status LED blinks **once slowly** (1 second) when entering normal operation
- Serial Monitor shows sensor readings every 5 seconds
- No watchdog resets
- WiFi connected (or "Operating in offline mode")

### Safe Mode (if sensors fail)
- Status LED blinks **rapidly** (5 quick blinks)
- Serial shows: "SAFE MODE ACTIVATED"
- System maintains critical protection only

### Emergency Mode (simulated failure)
- Status LED flashes rapidly
- Serial shows: "EMERGENCY MODE ACTIVATED"
- Actuators respond to emergency type

---

## 🎯 Next Steps After Successful Boot

1. **Let It Run**: Monitor for 1 hour, ensure no watchdog resets
2. **Add Sensors**: Add MQ135, Modbus sensor one-by-one
3. **Calibrate**: Run calibration procedures (press 'c')
4. **Test Offline**: Disconnect WiFi, verify SD buffering
5. **Connect Actuators**: Add relay board and test (no high-voltage loads yet!)

---

## 📖 Where to Go From Here

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

## 💡 Pro Tips

1. **Start Small**: Don't connect everything at once - add sensors incrementally
2. **Monitor Serial**: Always have Serial Monitor open during testing
3. **Save Output**: Copy-paste interesting Serial Monitor output for reference
4. **Label Wires**: Use colored tape or labels (saves hours of debugging!)
5. **Take Photos**: Photograph your wiring before disconnecting anything

---

## 🆘 Need Help?

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

## ✅ Success Criteria

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

**Last Updated**: December 15, 2025
**Estimated Time**: 30 minutes (first time)
**Difficulty**: Beginner-Intermediate
