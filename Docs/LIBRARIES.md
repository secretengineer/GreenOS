# GreenOS - PlatformIO Libraries and Dependencies

## Overview
This document lists all required libraries for the GreenOS firmware running on the **ESP32-WROOM-32E**. Libraries are automatically managed by PlatformIO via the `platformio.ini` configuration file.

---

## Installation Instructions

### Using PlatformIO (Recommended)

1. Open VS Code with PlatformIO extension installed
2. Open the `GreenOS/Firmware` folder
3. PlatformIO automatically reads `platformio.ini` and installs dependencies
4. Click **Build** to download and compile all libraries

**Manual Installation via CLI:**
```powershell
cd Firmware
pio pkg install
```

Libraries are defined in `platformio.ini` under `lib_deps`:
```ini
lib_deps = 
    mobizt/Firebase Arduino Client Library for ESP8266 and ESP32
    sparkfun/SparkFun SCD30 Arduino Library
    4-20ma/ModbusMaster
    bblanchon/ArduinoJson @ ^6
    tzapu/WiFiManager
```

---

## Required Libraries

### Core Communication Libraries

#### 1. **SparkFun SCD30 Arduino Library** - CO2/Temperature/Humidity Sensor
- **PlatformIO ID**: `sparkfun/SparkFun SCD30 Arduino Library`
- **Version**: Latest stable
- **Author**: SparkFun Electronics
- **Purpose**: Interface with SCD-30 NDIR CO2 sensor via I2C
- **Dependencies**: Wire (built-in)
- **GitHub**: https://github.com/sparkfun/SparkFun_SCD30_Arduino_Library

#### 2. **ModbusMaster** - Modbus RTU Communication
- **PlatformIO ID**: `4-20ma/ModbusMaster`
- **Version**: 2.0.1 or later
- **Author**: Doc Walker
- **Purpose**: Communicate with Modbus RS485 soil EC/pH sensor
- **Dependencies**: None
- **GitHub**: https://github.com/4-20ma/ModbusMaster

#### 3. **ArduinoJson** - JSON Parsing and Serialization
- **PlatformIO ID**: `bblanchon/ArduinoJson @ ^6`
- **Version**: 6.21.3 or later (v6.x, **NOT v7**)
- **Author**: Benoit Blanchon
- **Purpose**: Parse Firebase responses, serialize sensor data
- **Documentation**: https://arduinojson.org/
- **GitHub**: https://github.com/bblanchon/ArduinoJson

#### 4. **Firebase Arduino Client Library for ESP8266 and ESP32**
- **PlatformIO ID**: `mobizt/Firebase Arduino Client Library for ESP8266 and ESP32`
- **Version**: 4.4.7 or later
- **Author**: Mobizt
- **Purpose**: Firebase Realtime Database and Cloud Firestore integration
- **Dependencies**: 
  - WiFiClientSecure (built-in ESP32)
  - HTTPClient (built-in ESP32)
- **GitHub**: https://github.com/mobizt/Firebase-ESP-Client

#### 5. **WiFiManager** - WiFi Provisioning
- **PlatformIO ID**: `tzapu/WiFiManager`
- **Version**: Latest stable
- **Author**: tzapu
- **Purpose**: Captive portal for WiFi credential configuration
- **GitHub**: https://github.com/tzapu/WiFiManager

---

### Utility Libraries

#### 6. **SPIFFS** - Internal Flash File System
- **Version**: Built-in (ESP32 core)
- **Purpose**: Offline data buffering to internal flash
- **Installation**: Pre-installed with ESP32 platform
- **Note**: Replaces SD card for offline storage

---

### Built-in Libraries (No Installation Needed)

These libraries come pre-installed with the ESP32 platform in PlatformIO:

- **Wire** - I2C communication (SDA=GPIO21, SCL=GPIO22)
- **SPI** - SPI communication
- **WiFi** - ESP32 WiFi functionality
- **WiFiClientSecure** - HTTPS/TLS support
- **HTTPClient** - HTTP requests
- **SPIFFS** - Internal flash file system
- **esp_task_wdt** - Hardware watchdog timer
- **FreeRTOS** - Real-time operating system (built into ESP32)

---

## Hardware-Specific Requirements

### ESP32 Platform in PlatformIO

**IMPORTANT**: The ESP32 platform is automatically installed when you open the project:

1. Open VS Code with PlatformIO extension
2. Open the `GreenOS/Firmware` folder
3. PlatformIO reads `platformio.ini` and installs the ESP32 platform
4. Board: `esp32dev` (ESP32-WROOM-32E)
5. Framework: `arduino`

**platformio.ini configuration:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.flash_mode = dio
board_build.f_flash = 80000000L
```

---

## Library Versions Summary Table

| Library Name | PlatformIO ID | Purpose | Install Method |
|-------------|---------------|---------|----------------|
| SparkFun SCD30 | `sparkfun/SparkFun SCD30 Arduino Library` | CO2/Temp/Humidity sensor | Auto (platformio.ini) |
| ModbusMaster | `4-20ma/ModbusMaster` | Modbus RTU communication | Auto (platformio.ini) |
| ArduinoJson | `bblanchon/ArduinoJson @ ^6` | JSON parsing | Auto (platformio.ini) |
| Firebase Client | `mobizt/Firebase Arduino Client Library...` | Firebase integration | Auto (platformio.ini) |
| WiFiManager | `tzapu/WiFiManager` | WiFi provisioning | Auto (platformio.ini) |
| Wire | Built-in | I2C | Pre-installed |
| WiFi | Built-in | ESP32 WiFi | Pre-installed |
| SPIFFS | Built-in | Flash storage | Pre-installed |
| FreeRTOS | Built-in | Task management | Pre-installed |

---

## PlatformIO lib_deps (Copy to platformio.ini)

For reference, here's the complete `lib_deps` section:

```ini
lib_deps = 
    mobizt/Firebase Arduino Client Library for ESP8266 and ESP32
    sparkfun/SparkFun SCD30 Arduino Library
    4-20ma/ModbusMaster
    bblanchon/ArduinoJson @ ^6
    tzapu/WiFiManager
```

---

## Verification

After opening the project in PlatformIO, verify by:

1. Open VS Code with PlatformIO
2. Open the `GreenOS/Firmware` folder
3. Check the **PIO Home** → **Libraries** tab
4. Confirm all libraries are listed:
   - SparkFun SCD30 Arduino Library
   - ModbusMaster
   - ArduinoJson
   - Firebase Arduino Client Library
   - WiFiManager

**CLI Verification:**
```powershell
cd Firmware
pio pkg list
```

---

## Hardware Wiring Summary

### I2C Devices (Wire)
- **SCD-30 CO2 Sensor**
  - SDA → GPIO 21 (ESP32)
  - SCL → GPIO 22 (ESP32)
  - VCC → 3.3V or 5V
  - GND → GND

### UART/Modbus Devices
- **MAX485 RS485 Transceiver** (Soil Sensor)
  - RO → GPIO 16 (UART2 RX)
  - DI → GPIO 17 (UART2 TX)
  - DE/RE → GPIO 4 (Driver Enable)
  - VCC → 3.3V or 5V
  - GND → GND
  - A/B → Soil sensor A/B terminals

### Analog Sensors (ADC1 Only - ADC2 conflicts with WiFi)
- **MQ135 Air Quality Sensor**
  - AOUT → GPIO 34 (ADC1_CH6) via voltage divider
  - VCC → 5V
  - GND → GND
  - **CRITICAL**: Use voltage divider (R1=10kΩ, R2=20kΩ) to protect 3.3V ADC!

### Digital I/O
- **PIR Motion Sensor** → GPIO 27
- **UPS Status** → GPIO 26 (active low, pullup)
- **Status LED** → GPIO 2 (built-in on most ESP32 dev boards)

### Relay Outputs (5V optoisolated, 15A)
- **Heater Primary** → GPIO 13
- **Heater Secondary** → GPIO 12
- **Fan Exhaust** → GPIO 14
- **Fan Circulation** → GPIO 27
- **Pump Irrigation** → GPIO 26
- **Grow Lights** → GPIO 25

---

## Troubleshooting

### Common Issues

**1. Library not found errors**
- Solution: Run `pio pkg install` in the Firmware directory
- Check `platformio.ini` has correct `lib_deps` entries

**2. Firebase connection fails**
- Solution: Check WiFi credentials in `Firmware/include/config.h`
- Verify Firebase API key and project settings
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)

**3. Modbus communication errors**
- Solution: 
  - Verify baud rate (4800 for S-Soil MT-02)
  - Check A/B wiring polarity
  - Ensure DE/RE pin (GPIO 4) is toggling correctly

**4. ADC readings out of range**
- Solution: 
  - Verify voltage divider for MQ135 (must not exceed 3.3V!)
  - Use ADC1 pins only (GPIO 32-39) - ADC2 conflicts with WiFi
  - Run ADC calibration: Press 'c' in Serial Monitor

**5. SCD-30 not detected**
- Solution:
  - Check I2C wiring (SDA=GPIO21, SCL=GPIO22)
  - Verify I2C address: 0x61
  - Try I2C scanner sketch

---

## Additional Resources

- **ESP32-WROOM-32E Documentation**: https://docs.espressif.com/projects/esp-idf/
- **PlatformIO ESP32 Platform**: https://docs.platformio.org/en/latest/platforms/espressif32.html
- **SCD-30 Datasheet**: https://sensirion.com/products/catalog/SCD30/
- **Modbus Sensor Datasheet**: See `RS485SoilMoisture&TemperatureSensor(S-SoilMT-02)-Datasheet.pdf`

---

## Version History

- **v2.0.0** (2025-12-31): Updated for ESP32-WROOM-32E with PlatformIO
- **v1.0** (2025-12-15): Initial release for Arduino UNO Q

---

## License

This library configuration is part of the GreenOS project and is provided as-is for educational and development purposes.
