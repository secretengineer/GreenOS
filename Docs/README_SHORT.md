# GreenOS

**Intelligent Greenhouse Controller** — Open-source IoT system for automated greenhouse management

[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)]()
[![Platform](https://img.shields.io/badge/platform-ESP32--WROOM--32E-blue)]()
[![Firmware](https://img.shields.io/badge/firmware-v2.0.0--dev-orange)]()
[![Backend](https://img.shields.io/badge/backend-Firebase%20%7C%20GCP-yellow)]()

##  Quick Start

Get your greenhouse running in 30 minutes:

```powershell
# Clone the repository
git clone https://github.com/secretengineer/GreenOS.git
cd GreenOS

# Open in VS Code with PlatformIO
code Firmware

# Build and upload to ESP32
pio run --target upload
pio device monitor
```

##  Key Features

- **ESP32-WROOM-32E** — Dual-core 240MHz with WiFi + Bluetooth
- **FreeRTOS** — Real-time task management across both cores
- **SPIFFS Buffering** — Offline data storage for up to 500 readings
- **Hardware Watchdog** — 30-second timeout with automatic recovery
- **Firebase Integration** — Real-time data sync and cloud analytics
- **Safety Interlocks** — Prevents dangerous actuator combinations

##  Documentation

| Document | Description |
|:--|:--|
| **[Quick Start](Docs/QUICKSTART.md)** | Get running in 30 minutes |
| **[Getting Started](Docs/GETTING_STARTED.md)** | Complete setup guide |
| **[Hardware Setup](Docs/HARDWARE_SETUP.md)** | Wiring diagrams and BOM |
| **[Libraries](Docs/LIBRARIES.md)** | PlatformIO dependencies |
| **[Implementation](Docs/IMPLEMENTATION_SUMMARY.md)** | Technical architecture |

##  Hardware

| Component | Purpose |
|:--|:--|
| ESP32-WROOM-32E | Main controller (Dual-core @ 240MHz) |
| SCD-30 | CO2, temperature, humidity (I2C) |
| MQ135 | Air quality (ADC1 with voltage divider) |
| S-Soil MT-02 | EC, pH, moisture, NPK (Modbus RS485) |
| Relay Board | 6-channel actuator control (15A) |

##  Contact

**Pat Ryan**  
📧 [pat@patryan.com](mailto:pat@patryan.com)

##  License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

*Last Updated: December 31, 2025 | Platform: ESP32-WROOM-32E | Firmware: v2.0.0-dev*
