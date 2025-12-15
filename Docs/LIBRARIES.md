# GreenOS - Arduino Libraries and Dependencies

## Overview
This document lists all required Arduino libraries for the GreenOS firmware running on the **Arduino UNO Q** (Renesas RA4M1 + ESP32-S3).

---

## Installation Instructions

### Using Arduino IDE 2.x

1. Open Arduino IDE 2.x
2. Go to **Tools** → **Manage Libraries...**
3. Search for each library by name
4. Install the specified version (or latest compatible version)

Alternatively, use the Library Manager's search feature or install via ZIP from GitHub.

---

## Required Libraries

### Core Communication Libraries

#### 1. **Adafruit SCD30** - CO2/Temperature/Humidity Sensor
- **Version**: 1.0.11 or later
- **Author**: Adafruit
- **Purpose**: Interface with Adafruit SCD-30 NDIR CO2 sensor via I2C
- **Installation**: Arduino Library Manager → Search "Adafruit SCD30"
- **Dependencies**: 
  - Adafruit BusIO
  - Wire (built-in)
- **GitHub**: https://github.com/adafruit/Adafruit_SCD30

#### 2. **ModbusMaster** - Modbus RTU Communication
- **Version**: 2.0.1 or later
- **Author**: Doc Walker
- **Purpose**: Communicate with Modbus RS485 soil EC/pH sensor
- **Installation**: Arduino Library Manager → Search "ModbusMaster"
- **Dependencies**: None
- **GitHub**: https://github.com/4-20ma/ModbusMaster

#### 3. **ArduinoJson** - JSON Parsing and Serialization
- **Version**: 6.21.3 or later (v6.x, NOT v7)
- **Author**: Benoit Blanchon
- **Purpose**: Parse Firebase responses, serialize sensor data
- **Installation**: Arduino Library Manager → Search "ArduinoJson"
- **Documentation**: https://arduinojson.org/
- **GitHub**: https://github.com/bblanchon/ArduinoJson

#### 4. **Firebase Arduino Client Library for ESP32**
- **Version**: 4.4.7 or later
- **Author**: Mobizt
- **Purpose**: Firebase Realtime Database and Cloud Firestore integration
- **Installation**: Arduino Library Manager → Search "Firebase ESP Client"
- **Dependencies**: 
  - WiFiClientSecure (built-in ESP32)
  - HTTPClient (built-in ESP32)
- **GitHub**: https://github.com/mobizt/Firebase-ESP-Client

---

### Utility Libraries

#### 5. **Adafruit BusIO** - I2C/SPI Abstraction
- **Version**: 1.14.1 or later
- **Author**: Adafruit
- **Purpose**: Required dependency for Adafruit sensors
- **Installation**: Automatically installed with Adafruit SCD30
- **GitHub**: https://github.com/adafruit/Adafruit_BusIO

#### 6. **SD** - SD Card Management
- **Version**: Built-in (Arduino core library)
- **Purpose**: Offline data buffering to SD card
- **Installation**: Pre-installed with Arduino IDE
- **Note**: Uses SPI interface

#### 7. **EEPROM** - Non-Volatile Storage
- **Version**: Built-in (Arduino core library)
- **Purpose**: Store ADC calibration, sensor calibration data
- **Installation**: Pre-installed with Arduino IDE

---

### Built-in Libraries (No Installation Needed)

These libraries come pre-installed with the Arduino UNO Q board support:

- **Wire** - I2C communication
- **SPI** - SPI communication for SD card
- **WiFi** - ESP32-S3 WiFi functionality
- **WiFiClientSecure** - HTTPS/TLS support
- **HTTPClient** - HTTP requests
- **EEPROM** - Non-volatile memory access
- **SD** - SD card file system

---

## Hardware-Specific Requirements

### Arduino UNO Q Board Support

**IMPORTANT**: Install the Arduino UNO Q board support package:

1. Open Arduino IDE 2.x
2. Go to **File** → **Preferences**
3. Add this URL to "Additional Boards Manager URLs":
   ```
   https://github.com/arduino/ArduinoCore-renesas/releases/latest/download/package_renesas_index.json
   ```
4. Go to **Tools** → **Board** → **Boards Manager**
5. Search for "Renesas" or "Arduino UNO R4"
6. Install **Arduino UNO R4 Boards** package
7. Select **Tools** → **Board** → **Arduino UNO R4** → **Arduino UNO R4 WiFi**

---

## Library Versions Summary Table

| Library Name | Minimum Version | Purpose | Install Method |
|-------------|-----------------|---------|----------------|
| Adafruit SCD30 | 1.0.11 | CO2/Temp/Humidity sensor | Library Manager |
| ModbusMaster | 2.0.1 | Modbus RTU communication | Library Manager |
| ArduinoJson | 6.21.3 | JSON parsing | Library Manager |
| Firebase ESP Client | 4.4.7 | Firebase integration | Library Manager |
| Adafruit BusIO | 1.14.1 | I2C/SPI abstraction | Auto-installed |
| Wire | Built-in | I2C | Pre-installed |
| WiFi | Built-in | ESP32 WiFi | Pre-installed |
| SD | Built-in | SD card | Pre-installed |
| EEPROM | Built-in | Non-volatile storage | Pre-installed |

---

## Installation Script (All Libraries)

For convenience, here's a list to copy-paste into the Arduino Library Manager search:

```
Adafruit SCD30
ModbusMaster
ArduinoJson
Firebase ESP Client
Adafruit BusIO
```

---

## Verification

After installing all libraries, verify by:

1. Open Arduino IDE
2. Go to **Sketch** → **Include Library**
3. Confirm all libraries are listed:
   - Adafruit SCD30
   - ModbusMaster
   - ArduinoJson
   - Firebase ESP Client
   - Adafruit BusIO

---

## Hardware Wiring Summary

### I2C Devices (Wire)
- **SCD-30 CO2 Sensor**
  - SDA → Pin 20 (Arduino UNO Q)
  - SCL → Pin 21
  - VCC → 3.3V or 5V
  - GND → GND

### UART/Modbus Devices
- **MAX485 RS485 Transceiver** (Soil Sensor)
  - RO → Pin 0 (RX)
  - DI → Pin 1 (TX)
  - DE/RE → Pin 2 (Driver Enable)
  - VCC → 5V
  - GND → GND
  - A/B → Soil sensor A/B terminals

### Analog Sensors (ADC)
- **MQ135 Air Quality Sensor**
  - AOUT → A0 (via voltage divider: 5V → 3.3V)
  - VCC → 5V
  - GND → GND
  - **CRITICAL**: Use voltage divider (R1=10kΩ, R2=20kΩ) to protect 3.3V ADC!

### SPI Devices
- **SD Card Module**
  - CS → Pin 10
  - MOSI → Pin 11
  - MISO → Pin 12
  - SCK → Pin 13
  - VCC → 3.3V or 5V
  - GND → GND

### Digital I/O
- **PIR Motion Sensor** → Pin 3
- **UPS Status** → Pin 4 (active low, pullup)
- **Status LED** → Pin 13 (built-in)

### Relay Outputs (5V optoisolated, 15A)
- **Heater Primary** → Pin 6
- **Heater Secondary** → Pin 7
- **Fan Exhaust** → Pin 8
- **Fan Circulation** → Pin 9
- **Pump Irrigation** → Pin 11
- **Grow Lights** → Pin 12

---

## Troubleshooting

### Common Issues

**1. Library not found errors**
- Solution: Ensure all dependencies are installed (especially Adafruit BusIO)

**2. Firebase connection fails**
- Solution: Check WiFi credentials in config.h, verify API key

**3. Modbus communication errors**
- Solution: 
  - Verify baud rate (4800 for S-Soil MT-02)
  - Check A/B wiring polarity
  - Ensure DE/RE pin is toggling correctly

**4. ADC readings out of range**
- Solution: 
  - Verify voltage divider for MQ135 (must not exceed 3.3V!)
  - Run ADC calibration: Press 'c' in Serial Monitor

**5. SCD-30 not detected**
- Solution:
  - Check I2C wiring (SDA/SCL)
  - Verify I2C address: 0x61
  - Try I2C scanner sketch

---

## Additional Resources

- **Arduino UNO Q Documentation**: https://docs.arduino.cc/hardware/uno-r4-wifi/
- **Renesas RA4M1 Datasheet**: In project Docs folder
- **SCD-30 Datasheet**: https://www.adafruit.com/product/4867
- **Modbus Sensor Datasheet**: See `RS485SoilMoisture&TemperatureSensor(S-SoilMT-02)-Datasheet.pdf`

---

## Version History

- **v1.0** (2025-12-15): Initial release with all required libraries for GreenOS hardware interface

---

## License

This library configuration is part of the GreenOS project and is provided as-is for educational and development purposes.
