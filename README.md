# GreenOS: Intelligent Greenhouse Controller

<div align="center">

![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-active%20development-brightgreen)
![Platform](https://img.shields.io/badge/platform-ESP32--WROOM--32E-blue)
![Firmware](https://img.shields.io/badge/firmware-v2.0.0--dev-orange)
![Firebase](https://img.shields.io/badge/backend-Firebase%20%7C%20GCP-yellow)

**An open-source, production-ready intelligent greenhouse control system**

[Getting Started](#-getting-started) • [Hardware Setup](#-hardware-requirements) • [Documentation](#-documentation) • [Contributing](#-contributing)

</div>

---

##  Overview

**GreenOS** is a comprehensive greenhouse automation platform designed for precision environmental management. Built on the **ESP32-WROOM-32E** microcontroller and powered by **Firebase** and **Google Cloud Platform**, GreenOS delivers industrial-grade reliability with a focus on **safe-fail operation**, **offline resilience**, and **real-time responsiveness**.

The system integrates advanced sensor fusion, predictive analytics, and automated climate control to create optimal growing conditions while maintaining robust safety protocols.

> ** Looking for the Arduino UNO Q version?**  
> The original Arduino UNO Q codebase is preserved in the [`v1.0-uno-q`](https://github.com/secretengineer/GreenOS/releases/tag/v1.0-uno-q) release and [`arduino-uno-q`](https://github.com/secretengineer/GreenOS/tree/arduino-uno-q) branch.

| Attribute | Details |
|:---|:---|
| **Project** | GreenOS |
| **Domain** | [greenos.app](https://greenos.app) |
| **Platform** | ESP32-WROOM-32E (Dual-core Xtensa LX6 @ 240MHz, WiFi + Bluetooth) |
| **Framework** | PlatformIO + Arduino Framework |
| **Backend** | Firebase (Firestore, Cloud Functions, Authentication) |
| **Target Environment** | 325 cu. ft. greenhouse in Denver, CO (5,280 ft elevation) |
| **Philosophy** | Safe-fail by design • Data-driven decisions • Offline-first operation |

---

##  Key Features

### 🔧 Edge Intelligence (ESP32-WROOM-32E)
- **Dual-Core Processing** — FreeRTOS task management across two Xtensa LX6 cores at 240MHz
- **Multi-Sensor Fusion** — Integrates NDIR CO2 (SCD-30), air quality (MQ135), and Modbus RS485 soil sensors (EC, pH, moisture, NPK)
- **Hardware Watchdog Timer** — Automatic recovery prevents firmware hangs with 30-second timeout
- **Finite State Machine** — Predictable system behavior through well-defined states
- **Offline Data Buffering** — SPIFFS storage maintains operational history during network outages
- **OTA Updates** — Over-the-air firmware updates for remote deployments

### 🌡️ Automated Climate Control
- **Safety Interlocks** — Prevents dangerous actuator combinations (e.g., simultaneous heating and exhaust ventilation)
- **Duty Cycle Management** — Protects equipment through minimum cycle times and maximum runtime limits
- **Emergency Protocols** — Automated responses to critical conditions with multi-tier alert system
- **Gradual Adjustments** — Warning-level responses provide smooth environmental transitions

###  Cloud Integration & Analytics
- **Real-time Synchronization** — Firebase Firestore provides live data visibility
- **Historical Analysis** — BigQuery enables long-term trend analysis and ML model training
- **Prioritized Alerting** — Multi-level alert system (ULTRA → HIGH → MEDIUM → LOW)
- **Comprehensive Logging** — Audit trail of all system actions and environmental events

###  User Experience
- **Diagnostic Interface** — Serial commands for real-time debugging and monitoring
- **Web Dashboard** — Real-time gauges, charts, and controls (React + Vite)
- **Responsive Design** — Professional UI adapts to desktop, tablet, and mobile

---

##  System Architecture

GreenOS is a hybrid edge-cloud system combining immediate local responsiveness with scalable cloud analytics.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           GREENHOUSE (Edge Device)                          │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        ESP32-WROOM-32E                               │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────┐   │   │
│  │  │ Core 0       │  │ Core 1       │  │ Peripherals              │   │   │
│  │  │ Network Task │  │ Sensor Task  │  │ • I2C (SCD-30)           │   │   │
│  │  │ Firebase Sync│  │ Actuator Task│  │ • RS485 (Soil Sensor)    │   │   │
│  │  │ OTA Updates  │  │ Emergency    │  │ • ADC (MQ135, Light)     │   │   │
│  │  └──────────────┘  └──────────────┘  │ • GPIO (Relays, PIR)     │   │   │
│  │                                       │ • SPI (SD Card)          │   │   │
│  └───────────────────────────────────────┴──────────────────────────────┘   │
│                                     │                                        │
│         ┌───────────────────────────┼───────────────────────────┐           │
│         ▼                           ▼                           ▼           │
│  ┌─────────────┐           ┌─────────────┐            ┌────────────────┐   │
│  │   Sensors   │           │  Actuators  │            │    Storage     │   │
│  │ SCD-30 CO2  │           │ Heaters (2) │            │ SPIFFS/SD Card │   │
│  │ MQ135 Air   │           │ Fans (2)    │            │ Offline Buffer │   │
│  │ RS485 Soil  │           │ Pump        │            │ Config/Cals    │   │
│  │ PIR Motion  │           │ Grow Lights │            └────────────────┘   │
│  └─────────────┘           └─────────────┘                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     │ WiFi (2.4GHz 802.11 b/g/n)
                                     │ HTTPS/TLS 1.2+
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        FIREBASE / GOOGLE CLOUD                              │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐  ┌─────────────┐  │
│  │   Firestore   │  │    Cloud      │  │   Firebase    │  │  BigQuery   │  │
│  │  (Real-time)  │  │   Functions   │  │     Auth      │  │ (Analytics) │  │
│  └───────────────┘  └───────────────┘  └───────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USER INTERFACES                                   │
│           ┌───────────────────┐          ┌───────────────────┐             │
│           │   Web Dashboard   │          │    Mobile App     │             │
│           │   (React + Vite)  │          │   (iOS/Android)   │             │
│           └───────────────────┘          └───────────────────┘             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

##  Project Structure

```
GreenOS/
├── Firmware/                         # ESP32 Firmware (PlatformIO)
│   ├── platformio.ini               # Build configuration & dependencies
│   ├── include/
│   │   ├── config.h                 # Hardware pins, thresholds, WiFi credentials
│   │   ├── config_template.h        # Template for config (safe to commit)
│   │   ├── sensor_manager.h         # Sensor reading and validation
│   │   ├── actuator_manager.h       # Relay control with safety interlocks
│   │   ├── network_manager.h        # WiFi and Firebase communication
│   │   └── data_logger.h            # SPIFFS/SD card data buffering
│   ├── src/
│   │   ├── main.cpp                 # FreeRTOS tasks, FSM, system initialization
│   │   ├── sensor_manager.cpp       # SCD-30, MQ135, Modbus implementations
│   │   ├── actuator_manager.cpp     # Safety logic, emergency responses
│   │   ├── network_manager.cpp      # WiFi Manager, Firebase sync
│   │   └── data_logger.cpp          # Offline buffering and CSV logging
│   └── libraries/                   # External library overrides (if needed)
│
├── CloudFunctions/                  # Firebase Cloud Functions
│   ├── package.json                 # Node.js dependencies
│   └── functions/
│       ├── index.js                 # Function entry point
│       ├── triggers.js              # Firestore triggers for alerts
│       ├── api.js                   # REST API endpoints
│       └── scheduled.js             # Periodic tasks (BigQuery export)
│
├── WebUI/                           # Web Application (React + Vite)
│   ├── package.json                 # Frontend dependencies
│   ├── vite.config.js              # Build configuration
│   └── src/
│       ├── App.jsx                  # Application root
│       ├── config.js                # Firebase configuration
│       ├── components/              # Reusable UI components
│       │   ├── Navbar.jsx
│       │   ├── Sidebar.jsx
│       │   ├── SensorGauge.jsx
│       │   ├── SensorChart.jsx
│       │   └── AlertList.jsx
│       └── pages/                   # Application pages
│           ├── Dashboard.jsx
│           ├── Analytics.jsx
│           ├── Alerts.jsx
│           ├── Settings.jsx
│           └── Login.jsx
│
├── Docs/                            # Documentation
│   ├── QUICKSTART.md               # 30-minute getting started guide
│   ├── HARDWARE_SETUP.md           # Wiring diagrams and hardware details
│   ├── LIBRARIES.md                # Library requirements
│   ├── HOSTINGER_MIGRATION_ADDENDUM.md # Hostinger migration proposal
│   └── IMPLEMENTATION_SUMMARY.md   # Technical design decisions
│
├── assets/                          # Project assets and media
├── firebase.json                    # Firebase project configuration
├── firestore.rules                  # Firestore security rules
├── firestore.indexes.json          # Firestore indexes
└── storage.rules                    # Cloud Storage security rules
```

**Implementation Status:**
-  **Firmware**: Production-ready ESP32 code with FreeRTOS
-  **Cloud Functions**: Partial implementation, Firebase integration active
-  **Web UI**: Structure in place, components being developed
-  **Documentation**: Comprehensive guides complete

---

##  Getting Started

### Prerequisites

Before you begin, ensure you have the following:

| Requirement | Version | Notes |
|:---|:---|:---|
| **PlatformIO** | Latest | VSCode extension or CLI |
| **Python** | 3.8+ | Required by PlatformIO |
| **Git** | Latest | For cloning the repository |
| **USB Driver** | CP210x or CH340 | Depends on your ESP32 dev board |

### Quick Start (30 Minutes)

#### Step 1: Clone the Repository

```bash
git clone https://github.com/secretengineer/GreenOS.git
cd GreenOS
```

#### Step 2: Install PlatformIO

**Option A: VSCode Extension (Recommended)**
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)
3. Restart VSCode

**Option B: CLI Installation**
```bash
pip install platformio
```

#### Step 3: Configure Credentials

1. Copy the template configuration:
   ```bash
   cd Firmware/include
   cp config_template.h config.h
   ```

2. Edit `config.h` with your credentials:
   ```cpp
   // WiFi Configuration
   #define WIFI_SSID "YOUR_WIFI_NETWORK"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   
   // Firebase Configuration
   #define FIREBASE_HOST "your-project.firebaseapp.com"
   #define FIREBASE_API_KEY "your-api-key"
   #define FIREBASE_PROJECT_ID "your-project-id"
   ```

> ⚠️ **Security Note:** Never commit `config.h` with real credentials. It's already in `.gitignore`.

#### Step 4: Build and Upload

**Using PlatformIO in VSCode:**
1. Open the `Firmware` folder in VSCode
2. Connect your ESP32 via USB
3. Click the **PlatformIO: Upload** button (→ icon in status bar)

**Using PlatformIO CLI:**
```bash
cd Firmware
pio run -t upload
```

#### Step 5: Monitor Serial Output

**VSCode:**
Click **PlatformIO: Serial Monitor** in the status bar

**CLI:**
```bash
pio device monitor
```

You should see:
```
╔════════════════════════════════════════════╗
║   GreenOS - Intelligent Greenhouse         ║
║   ESP32-WROOM-32E Firmware v2.0            ║
╚════════════════════════════════════════════╝

[INFO] ESP32 Chip Model: ESP32-D0WDQ6-V3 Rev 3
[INFO] CPU Frequency: 240 MHz
[INFO] Flash Size: 4 MB
[INFO] Free Heap: 245768 bytes
[OK] SPIFFS mounted: 12288 bytes used / 1441792 bytes total
[INIT] Initializing subsystems...
[INIT] Creating FreeRTOS tasks...
```

---

##  Hardware Requirements

### Minimum Setup (Testing)

| Component | Model | Quantity | Purpose |
|:---|:---|:---:|:---|
| ESP32 Development Board | ESP32-WROOM-32E DevKitC | 1 | Main controller |
| CO2/Climate Sensor | Adafruit SCD-30 | 1 | CO2, temperature, humidity |
| USB-C Cable | Data-capable | 1 | Programming and power |
| Breadboard + Jumpers | Standard | 1 set | Prototyping connections |

### Full Production Setup

| Component | Model | Quantity | Interface | Notes |
|:---|:---|:---:|:---|:---|
| **Microcontroller** | ESP32-WROOM-32E DevKitC | 1 | — | 4MB Flash, 520KB SRAM |
| **CO2 Sensor** | Adafruit SCD-30 | 1 | I2C (0x61) | NDIR, ±30ppm accuracy |
| **Air Quality** | MQ135 Module | 1 | Analog (ADC1) | Requires 48hr preheat |
| **Soil Sensor** | S-Soil MT-02 (7-in-1) | 1 | Modbus RS485 | EC, pH, moisture, temp, NPK |
| **RS485 Transceiver** | MAX485 Module | 1 | UART2 | For Modbus communication |
| **Relay Board** | 6-Channel 5V Optoisolated | 1 | GPIO | 15A rating for AC loads |
| **Motion Sensor** | HC-SR501 PIR | 1 | Digital GPIO | Security monitoring |
| **SD Card Module** | SPI SD Card | 1 | SPI | Offline data buffering |
| **Power Supply** | 5V 3A USB-C | 1 | USB-C | Main power |
| **Soil Sensor Power** | 12V DC Adapter | 1 | Barrel jack | For RS485 soil sensor |
| **UPS** | APC or similar | 1 | GPIO monitoring | Power failure detection |

### Sensors & Actuators Summary

**Environmental Monitoring:**

| Sensor | Model | Interface | Measurements | Notes |
|--------|-------|-----------|--------------|-------|
| **CO2/Climate** | Adafruit SCD-30 | I2C (0x61) | CO2 (ppm), Temperature (°C), Humidity (%) | NDIR sensor with altitude compensation, auto-calibration enabled |
| **Air Quality** | MQ135 | Analog ADC | Air quality (ppm) | Requires 48-hour preheat, voltage divider (5V→3.3V) mandatory |
| **Soil Monitor** | S-Soil MT-02 | Modbus RTU RS485 | EC (mS/cm), pH, Moisture (%), Temp (°C), N-P-K (mg/kg) | Industrial-grade probe, IP68 waterproof |
| **Motion** | HC-SR501 PIR | Digital GPIO | Motion detection | Security monitoring, off-hours alerts |
| **Power** | UPS Monitor | Digital GPIO | Power status | Triggers power-saving mode on mains failure |

**Actuator Control (5V Optoisolated Relays, 15A Rating):**

| Actuator | Power | Control Logic | Safety Features |
|----------|-------|---------------|-----------------|
| **Primary Heater** | 1500W @ 120VAC | Relay control | Interlock with exhaust fan, duty cycle limiting |
| **Secondary Heater** | 1500W @ 120VAC | Backup heating | Auto-activation on primary failure or extreme cold |
| **Exhaust Fan** | Variable | Speed-controlled relay | Cannot run simultaneously with heaters |
| **Circulation Fan** | Variable | Always-on capable | Distributes heat/cool air evenly |
| **Irrigation Pump** | 120VAC | Timed relay | 10-minute maximum runtime protection |
| **Grow Lights** | LED Array | Scheduled relay | Automated day/night cycles |

### Wiring Diagram (ESP32 Pin Assignments)

```
ESP32-WROOM-32E Pin Assignments
═══════════════════════════════════════════════════════════════════════════

I2C Bus (SCD-30 CO2 Sensor)
├── GPIO 21 (SDA) ──────────────── SCD-30 SDA
└── GPIO 22 (SCL) ──────────────── SCD-30 SCL

UART2 / Modbus RS485 (Soil Sensor)
├── GPIO 16 (RX2) ──────────────── MAX485 RO (Receiver Output)
├── GPIO 17 (TX2) ──────────────── MAX485 DI (Driver Input)
└── GPIO 4  (DE/RE) ────────────── MAX485 DE + RE (Direction Control)

Analog Sensors (ADC1 Only - ADC2 conflicts with WiFi)
├── GPIO 34 (ADC1_CH6) ─────────── MQ135 Analog Out (via voltage divider)
├── GPIO 35 (ADC1_CH7) ─────────── Backup VWC Sensor
├── GPIO 32 (ADC1_CH4) ─────────── Sound Level Sensor
└── GPIO 33 (ADC1_CH5) ─────────── Light Level Sensor

Digital I/O
├── GPIO 27 ────────────────────── PIR Motion Sensor (INPUT)
├── GPIO 26 ────────────────────── UPS Status (INPUT)
├── GPIO 25 ────────────────────── Buzzer Alarm (OUTPUT, DAC)
└── GPIO 2  ────────────────────── Status LED (OUTPUT, built-in)

SPI Bus (SD Card)
├── GPIO 5  (CS)  ──────────────── SD Card CS
├── GPIO 23 (MOSI) ─────────────── SD Card MOSI
├── GPIO 19 (MISO) ─────────────── SD Card MISO
└── GPIO 18 (SCK)  ─────────────── SD Card SCK

Actuator Relay Outputs (Active LOW)
├── GPIO 13 ────────────────────── Primary Heater Relay
├── GPIO 14 ────────────────────── Secondary Heater Relay
├── GPIO 12 ────────────────────── Exhaust Fan Relay
├── GPIO 15 ────────────────────── Circulation Fan Relay
├── GPIO 27* ───────────────────── Irrigation Pump Relay (alt pin)
└── GPIO 26* ───────────────────── Grow Lights Relay (alt pin)

* Alternative pins to avoid strapping pin conflicts

IMPORTANT NOTES:
═══════════════════════════════════════════════════════════════════════════
• ESP32 GPIO pins are 3.3V logic - DO NOT connect 5V signals directly!
• ADC2 pins (GPIO 0, 2, 4, 12-15, 25-27) cannot be used when WiFi is active
• Use ADC1 pins only (GPIO 32-39) for analog sensors with WiFi enabled
• Strapping pins (GPIO 0, 2, 5, 12, 15) have boot-time functions - use carefully
• MQ135 requires voltage divider (10kΩ/20kΩ) to scale 5V output to 3.3V
```

### MQ135 Voltage Divider Circuit

```
MQ135 AOUT (5V) ────┬──── R1 (10kΩ) ──── GND
                    │
                    └──── R2 (20kΩ) ──── ESP32 GPIO34 (3.3V max)

Output Voltage = 5V × (20kΩ / 30kΩ) = 3.33V (safe for ESP32)
```

---

##  Configuration Reference

### Sensor Thresholds (config.h)

All thresholds can be customized and are automatically synced from Firebase when connected:

```cpp
// Temperature Thresholds (°C)
TEMP_MIN             10.0    // Critical low - triggers emergency heating
TEMP_MAX             35.0    // Critical high - triggers full cooling
TEMP_OPTIMAL_MIN     18.0    // Optimal range minimum
TEMP_OPTIMAL_MAX     24.0    // Optimal range maximum
TEMP_FROST_ALERT      4.0    // Frost warning threshold

// Humidity Thresholds (%)
HUMIDITY_MIN         40.0    // Minimum acceptable
HUMIDITY_MAX         80.0    // Maximum acceptable
HUMIDITY_OPTIMAL_MIN 50.0    // Optimal minimum
HUMIDITY_OPTIMAL_MAX 70.0    // Optimal maximum

// CO2 Thresholds (ppm)
CO2_MIN             400.0    // Outdoor ambient
CO2_MAX            1500.0    // Maximum for plant growth
CO2_OPTIMAL_MIN     800.0    // Optimal minimum
CO2_OPTIMAL_MAX    1200.0    // Optimal maximum
CO2_DANGER         5000.0    // Dangerous for humans

// Soil Thresholds
VWC_OPTIMAL_MIN      30.0    // Volumetric Water Content (%)
VWC_OPTIMAL_MAX      50.0
PH_OPTIMAL_MIN        6.0    // pH range
PH_OPTIMAL_MAX        7.0
EC_OPTIMAL_MIN        1.0    // Electrical Conductivity (mS/cm)
EC_OPTIMAL_MAX        2.0
```

### Timing Intervals

```cpp
SENSOR_READ_INTERVAL        5000    // 5 seconds - sensor polling
FIREBASE_SYNC_INTERVAL     60000    // 1 minute - cloud sync
ANOMALY_CHECK_INTERVAL     10000    // 10 seconds - safety checks
MODBUS_READ_INTERVAL       15000    // 15 seconds - soil sensor
MEMORY_CHECK_INTERVAL      60000    // 1 minute - heap monitoring
NTP_SYNC_INTERVAL        3600000    // 1 hour - time sync
```

---

##  Safety Features

### Finite State Machine

GreenOS uses a robust state machine to ensure predictable behavior:

```
                              ┌─────────────────┐
                              │   STATE_BOOT    │
                              │  (Initialize)   │
                              └────────┬────────┘
                                       │
                              ┌────────▼────────┐
                              │ STATE_WIFI_CONNECT│
                              │  (Network Setup)│
                              └────────┬────────┘
                                       │
                              ┌────────▼────────┐
                              │ STATE_SENSOR_INIT│
                              │  (Sensor Setup) │
                              └────────┬────────┘
                                       │
          ┌────────────────────────────┼────────────────────────────┐
          │                            │                            │
          ▼                            ▼                            ▼
┌─────────────────┐         ┌─────────────────┐         ┌─────────────────┐
│ STATE_SAFE_MODE │◄────────│ STATE_NORMAL_OP │────────►│ STATE_EMERGENCY │
│  (Minimal Ops)  │         │ (Full Operation)│         │ (Auto Response) │
└─────────────────┘         └────────┬────────┘         └─────────────────┘
                                     │
                            ┌────────▼────────┐
                            │ STATE_DEEP_SLEEP│
                            │  (Power Save)   │
                            └─────────────────┘
```

### Safety Interlocks

The firmware enforces these physical safety rules:

| Rule | Purpose |
|:---|:---|
| Heaters + Exhaust Fan cannot run simultaneously | Prevents heat loss and wasted energy |
| 60-second minimum between relay toggles | Prevents relay contact wear |
| 10-minute maximum pump runtime | Prevents flooding |
| Duty cycle tracking on heaters | Fire safety |
| Heaters disabled during high temp emergency | Prevents overheating |
| Grow lights disabled during high temp emergency | Reduces heat load |

### Emergency Protocols

| Priority | Condition | Automatic Response |
|:---|:---|:---|
| 🔴 **ULTRA** | Temp < 10°C (frost danger) | Activate both heaters, disable cooling, send alert |
| 🔴 **ULTRA** | Temp > 35°C (heat stress) | Full ventilation, disable heaters, disable grow lights |
| 🟠 **HIGH** | Security breach (motion) | Activate lights, sound alarm, send photo alert |
| 🟠 **HIGH** | CO2 > 5000 ppm | Full ventilation, send danger alert |
| 🟡 **MEDIUM** | Humidity out of range | Adjust ventilation gradually |
| 🟡 **MEDIUM** | Soil moisture low | Queue irrigation cycle |

### Watchdog Timer

```cpp
#define WDT_TIMEOUT_SECONDS 30      // Hardware watchdog timeout
#define TASK_WDT_TIMEOUT_S  10      // FreeRTOS task watchdog
```

If any task becomes unresponsive, the ESP32 will automatically restart.

---

##  Data Flow

### Multi-Tier Storage Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          DATA FLOW                                       │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Sensors ──► ESP32 RAM ──► SPIFFS Buffer ──► Firebase Firestore         │
│              (5 sec)       (if offline)      (1 min sync)               │
│                                                    │                     │
│                                                    ▼                     │
│                                            BigQuery Export              │
│                                            (Historical)                  │
│                                                                          │
├─────────────────────────────────────────────────────────────────────────┤
│  STORAGE LAYER           │ RETENTION    │ PURPOSE                       │
│ ─────────────────────────┼──────────────┼─────────────────────────────  │
│  ESP32 RAM               │ Real-time    │ Current sensor values         │
│  SPIFFS/SD Card          │ Until sync   │ Offline buffering (500 max)   │
│  Cloud Firestore         │ 24-48 hours  │ Real-time dashboard           │
│  BigQuery                │ Indefinite   │ Historical analysis, ML       │
└─────────────────────────────────────────────────────────────────────────┘
```

### Offline Resilience

GreenOS maintains **full autonomous operation** without internet:

- ✅ All sensors continue reading normally
- ✅ Actuator control and safety interlocks remain active
- ✅ Emergency protocols execute locally
- ✅ Data buffered to SPIFFS (up to 500 readings)
- ✅ Automatic sync when connectivity restores

---

## 🔧 PlatformIO Build Environments

The project includes multiple build configurations in `platformio.ini`:

```bash
# Standard development build
pio run -e esp32dev -t upload

# Debug build with verbose logging
pio run -e esp32dev_debug -t upload

# OTA update (requires device on network)
pio run -e esp32dev_ota -t upload
```

### Build Flags

| Environment | Debug Level | Features |
|:---|:---|:---|
| `esp32dev` | Normal (3) | Production-ready |
| `esp32dev_debug` | Verbose (5) | Extra logging, debug symbols |
| `esp32dev_ota` | Normal (3) | Over-the-air upload enabled |

### Library Dependencies

All libraries are automatically managed by PlatformIO:

```ini
lib_deps = 
    mobizt/Firebase Arduino Client Library for ESP8266 and ESP32@^4.4.14
    sparkfun/SparkFun SCD30 Arduino Library@^1.0.20
    adafruit/Adafruit Unified Sensor@^1.1.14
    4-20ma/ModbusMaster@^2.0.1
    bblanchon/ArduinoJson@^7.0.4
    arduino-libraries/NTPClient@^3.2.1
    tzapu/WiFiManager@^2.0.17
```

---

##  Documentation

Detailed documentation is available in the `Docs/` folder:

| Document | Description |
|:---|:---|
| [QUICKSTART.md](Docs/QUICKSTART.md) | 30-minute getting started guide |
| [HARDWARE_SETUP.md](Docs/HARDWARE_SETUP.md) | Complete wiring diagrams and setup |
| [LIBRARIES.md](Docs/LIBRARIES.md) | Library requirements and installation |
| [HOSTINGER_MIGRATION_ADDENDUM.md](Docs/HOSTINGER_MIGRATION_ADDENDUM.md) | Proposal for moving GreenOS from Firebase Hosting to Hostinger |
| [IMPLEMENTATION_SUMMARY.md](Docs/IMPLEMENTATION_SUMMARY.md) | Technical design decisions |

---

##  Project Roadmap

###  Phase 1: Hardware Interface (Complete)
- [x] ESP32 platform migration from Arduino UNO Q
- [x] FreeRTOS dual-core task management
- [x] Sensor integration (SCD-30, MQ135, Modbus RS485)
- [x] Hardware watchdog timer
- [x] Finite state machine architecture
- [x] SPIFFS offline data buffering
- [x] Safety interlocks and emergency protocols
- [x] Comprehensive documentation

###  Phase 2: Cloud Integration (In Progress)
- [x] Firebase Firestore real-time sync
- [x] Firebase Authentication
- [ ] Cloud Functions for alerts and processing
- [ ] BigQuery data pipeline
- [ ] Historical data analysis tools

###  Phase 3: User Interfaces (Planned)
- [ ] Web dashboard (React + Vite)
- [ ] Real-time gauges and charts
- [ ] Mobile app (iOS/Android)
- [ ] Push notification system

###  Phase 4: Advanced Features (Future)
- [ ] Predictive analytics and forecasting
- [ ] Camera integration and plant health monitoring
- [ ] Multi-greenhouse management
- [ ] Weather API integration
- [ ] Machine learning anomaly detection

---

##  Contributing

GreenOS is open-source and welcomes contributions! Areas where help is needed:

| Area | Skills Needed | Priority |
|:---|:---|:---|
| **Firmware** | C++, ESP32, FreeRTOS | High |
| **Cloud Functions** | Node.js, Firebase, GCP | High |
| **Web UI** | React, Vite, Chart.js | Medium |
| **Documentation** | Technical writing | Medium |
| **Testing** | Hardware testing, edge cases | Medium |

### How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

##  License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

---

##  Acknowledgments

Built with:
- [Espressif ESP32](https://www.espressif.com/) — Microcontroller platform
- [PlatformIO](https://platformio.org/) — Build system and IDE
- [Firebase](https://firebase.google.com/) — Real-time backend
- [Google Cloud Platform](https://cloud.google.com/) — Analytics and ML
- [Adafruit](https://www.adafruit.com/) — SCD-30 sensor and libraries
- [SparkFun](https://www.sparkfun.com/) — SCD-30 Arduino library

Special thanks to the open-source community for libraries, tools, and inspiration.

---

##  Contact

**Pat Ryan**  
📧 [pat@patryan.com](mailto:pat@patryan.com)  
🌐 [https://greenos.app](https://greenos.app)  
🔗 [GitHub](https://github.com/secretengineer/GreenOS)

---

<div align="center">

**GreenOS** — Safe-fail by design • Offline-first operation • Production-ready reliability

*Last Updated: December 31, 2025 | Firmware v2.0.0-dev | Platform: ESP32-WROOM-32E*

</div>
