User: Read the README.md file and review the entire codebase for this repo. I want to refactor this project using best practices and a better, more robust approach. I want to answer the questions: 1) What is the best platform for hardware development for the functionality I need for this project. 2) What would be the best backend solution for this application? Is Firebase the best option? 


## Full Codebase Review & Refactor Recommendations

### Current State Summary

GreenOS is a 3-tier greenhouse automation system: ESP32 firmware (FreeRTOS, C++), Firebase Cloud Functions (Node.js), and a React/Vite WebUI. The firmware is the most mature component. The Cloud Functions are partially implemented, and the WebUI is largely stub pages. There's also a dead legacy Arduino UNO Q variant in main that should be removed from the main branch.

---

## Question 1: Best Hardware Platform

### Your Requirements (extracted from the codebase)

| Requirement | Details |
|---|---|
| **WiFi connectivity** | 2.4GHz, HTTPS/TLS to cloud backend |
| **I2C** | SCD-30 CO2 sensor |
| **Modbus RS485 (UART)** | 7-in-1 soil sensor via MAX485 transceiver |
| **Analog inputs** | MQ135 air quality, light level, sound level (3+ ADC channels) |
| **Digital GPIO** | 6 relay outputs, PIR input, UPS status, buzzer, status LED (~10 pins) |
| **SPI** | SD card module |
| **Local storage** | Offline data buffering (~500 JSON readings) |
| **Dual-core / RTOS** | Separate network vs sensor/actuator tasks |
| **OTA updates** | Remote firmware deployment |
| **Hardware watchdog** | Auto-recovery from hangs |
| **Low power (optional)** | Deep sleep with wake-on-interrupt |
| **Operating environment** | Greenhouse (humidity, temperature swings) |

### Platform Comparison

| Platform | WiFi | Cores | SRAM | Flash | ADC w/ WiFi | RTOS | OTA | Price | Maturity |
|---|---|---|---|---|---|---|---|---|---|
| **ESP32-WROOM-32E** (current) | Yes | 2 × 240MHz | 520KB | 4MB | ADC1 only (4 ch) | FreeRTOS | Yes | ~$4 | Excellent |
| **ESP32-S3-WROOM-1** | Yes + BLE 5 | 2 × 240MHz | 512KB | up to 16MB | ADC1 only (10 ch) | FreeRTOS | Yes | ~$5 | Good |
| **ESP32-C6** | Yes + BLE 5 + Thread/Zigbee | 1 × 160MHz | 512KB | 4MB | 7 ch | FreeRTOS | Yes | ~$4 | Newer |
| **Raspberry Pi Pico W** | Yes | 2 × 133MHz | 264KB | 2MB | 3 ADC ch | FreeRTOS optional | Manual | ~$6 | Good |
| **Raspberry Pi Zero 2 W** | Yes | 4 × 1GHz | 512MB | SD card | No native ADC | Linux | Yes | ~$15 | Excellent |
| **STM32 + WiFi module** | External | 1–2 cores | 64–640KB | 128KB–2MB | Many ch | FreeRTOS | Yes | ~$8–15 | Industrial |

### Recommendation: **ESP32-S3-WROOM-1** (upgrade) or **stay with ESP32-WROOM-32E**

**The ESP32-WROOM-32E is genuinely the right choice for this project.** Here's why:

1. **It meets every requirement natively** — built-in WiFi, dual-core FreeRTOS, hardware WDT, OTA, SPIFFS/LittleFS, sufficient GPIO, I2C, UART, SPI, ADC.

2. **The ESP32-S3 is the only worthwhile upgrade path**, giving you:
   - More usable ADC channels (10 on ADC1 vs. 4) — eliminates the ADC2/WiFi conflict pain
   - USB-OTG native (no UART bridge chip needed)
   - Vector instructions for future ML/anomaly detection on-device
   - Same price bracket, same PlatformIO/Arduino toolchain
   - Same pin-compatible form factor on many dev boards

3. **What would NOT be a good choice:**
   - **Raspberry Pi (Zero/Pico)** — overkill OS overhead for a real-time control loop; Linux introduces boot time, SD card corruption risk, and non-deterministic task scheduling that conflicts with your safety-interlock requirements
   - **STM32** — much better hardware, but the ecosystem for WiFi connectivity, Firebase libraries, and Arduino-compatible sensor drivers is significantly weaker. You'd spend months on plumbing instead of features
   - **ESP32-C6** — single core would force you to restructure your dual-core architecture; the Thread/Zigbee capabilities aren't useful for your direct WiFi-to-cloud design
   - **Arduino UNO R4** — you already tried this (the `main/` subdirectory), and hit BSP incompatibilities with WiFi, SPIFFS, and Firebase

**Bottom line:** Stick with ESP32 family. If you're buying new boards, get ESP32-S3 for the better ADC. If you already have ESP32-WROOM-32E boards, the code is solid on that platform and there's no compelling reason to migrate.

### Firmware Refactoring Priorities (regardless of board choice)

| Priority | Issue | Fix |
|---|---|---|
| **High** | SPIFFS is deprecated | Migrate to **LittleFS** — drop-in replacement, better wear leveling |
| **High** | `millis()` overflow in actuator runtime tracking | Use elapsed-time helpers with overflow-safe subtraction |
| **High** | Blocking `delay()` calls in network_manager and actuator_manager | Replace with non-blocking `vTaskDelay` / state-machine patterns |
| **Medium** | Dead `Firmware/src/main/` directory | Remove from the repo or move to a separate branch |
| **Medium** | MQ135 preheat not enforced | Add a preheat state gate that returns `NaN` / invalid marker until 48h elapsed |
| **Medium** | SCD-30 `dataAvailable()` counted as error | Only increment error on actual read failure, not data-not-ready |
| **Medium** | ArduinoJson v6 vs v7 conflict | [LIBRARIES.md](Docs/LIBRARIES.md) says v6 required, but [platformio.ini](Firmware/platformio.ini) pins `^7.0.4`. Resolve this |
| **Low** | Global pointer `g_sensorManager` is dead code | Remove it; Modbus callbacks use `MODBUS_DE_RE_PIN` directly |
| **Low** | `enforceMinimumCycleTime()` is empty | Remove dead method |

---

## Question 2: Best Backend Solution

### What You Currently Have (Firebase)

| Firebase Service | Usage | Status |
|---|---|---|
| **Firestore** | Real-time sensor data, alerts, config, commands | Working |
| **Cloud Functions (v1)** | Triggers, API endpoints, scheduled jobs | Partial |
| **Authentication** | Email/password + custom device tokens | Working |
| **Hosting** | WebUI static site | Configured |
| **Storage** | Camera images (rules written, not implemented) | Planned |
| **BigQuery** | Historical data export | Planned |

### Firebase — Honest Assessment

**Strengths for this project:**
- Real-time `onSnapshot` listeners are perfect for live dashboard gauges
- Authentication is simple and well-integrated
- Cloud Functions handle event-driven processing (sensor thresholds → alerts)
- Hosting is zero-config for the React SPA
- Free tier (Spark plan) is generous for a single greenhouse
- Admin SDK bypasses security rules for device writes — clean separation

**Weaknesses / pain points you're already hitting:**

| Problem | Details |
|---|---|
| **Cost at scale** | Firestore charges per read/write. At 1 sensor write/minute + real-time listeners + Cloud Function triggers, a single greenhouse generates ~43K writes/month + ~100K+ reads. Multi-greenhouse scales linearly. |
| **Time-series data is a bad fit for Firestore** | Sensor data is classic time-series, but Firestore has no native aggregation, windowing, downsampling, or retention policies. You're already working around this with BigQuery export. |
| **Cloud Functions v1 is legacy** | v1 has cold start issues (5–10s), limited concurrency, and Google is pushing v2 (Gen 2) with Cloud Run behind the scenes. |
| **Vendor lock-in** | Firebase Auth, Firestore, and Cloud Functions use proprietary APIs. Your ESP32's Mobizt library is tightly coupled to Firebase. |
| **No MQTT support** | Your ESP32 uses HTTPS REST to push data. MQTT is the standard for IoT because it's lighter, maintains a persistent connection, and supports QoS levels for unreliable networks. |
| **Alert deduplication gap** | Firestore triggers fire on every document write. You have no built-in way to debounce or deduplicate repeated threshold violations. |

### Alternative Backend Architectures

#### Option A: **Supabase** (Best Firebase Alternative)

| Aspect | Details |
|---|---|
| **Database** | PostgreSQL with real-time subscriptions (via WebSocket) |
| **Time-series** | Use TimescaleDB extension — native aggregation, downsampling, retention policies |
| **Auth** | Drop-in replacement for Firebase Auth (email/password, OAuth, JWT device tokens) |
| **Edge Functions** | Deno-based, similar to Cloud Functions |
| **Storage** | S3-compatible object storage |
| **Cost** | Free tier: 500MB DB, 1GB storage, 50K edge function invocations/month |
| **Self-hosting** | Can run on your own server — eliminates vendor lock-in |
| **ESP32 impact** | Replace Mobizt Firebase library with HTTPS REST calls to Supabase (straightforward) |

**Verdict:** Excellent choice if you want a SQL-based backend with real-time capabilities. TimescaleDB would solve your time-series problem natively. The migration effort is moderate.

#### Option B: **AWS IoT Core + DynamoDB/Timestream** (Best Enterprise IoT)

| Aspect | Details |
|---|---|
| **Protocol** | Native MQTT broker — ESP32 libraries exist (PubSubClient, AsyncMqttClient) |
| **Database** | DynamoDB (key-value) or **Timestream** (purpose-built time-series) |
| **Rules engine** | SQL-like rules on incoming MQTT messages → trigger Lambda/alerts |
| **Auth** | X.509 certificates for devices, Cognito for users |
| **OTA** | AWS IoT Jobs for fleet firmware management |
| **Cost** | Pay-per-message; free tier: 500K messages/month, 250K DynamoDB writes |

**Verdict:** Best-in-class for IoT, but dramatically more complex to set up. Overkill for a single greenhouse. Makes sense if you plan to sell this as a product with hundreds of devices.

#### Option C: **Self-hosted (MQTT + InfluxDB + Grafana)** (Best for Full Control)

| Aspect | Details |
|---|---|
| **Broker** | Mosquitto MQTT on a VPS/Raspberry Pi |
| **Database** | InfluxDB (purpose-built time-series) or TimescaleDB |
| **Visualization** | Grafana dashboards (gorgeous, free, time-series native) |
| **Alerts** | Grafana alerting or Kapacitor |
| **Auth** | Custom JWT or Traefik reverse proxy |
| **Cost** | $5–10/month VPS (DigitalOcean, Hetzner) |
| **ESP32 impact** | Replace Firebase library with PubSubClient (MQTT) — well-supported |

**Verdict:** Most cost-effective long-term, full control, no vendor lock-in. But you own all the infrastructure, security, backups, and uptime.

#### Option D: **Keep Firebase, but fix the architecture** (Lowest Migration Effort)

| Change | Why |
|---|---|
| Upgrade Cloud Functions to **v2 (Gen 2)** | Better cold starts, concurrency, pricing |
| Add **MQTT bridge** via Cloud IoT Core (deprecated) or a lightweight MQTT→Firestore relay | More efficient device communication |
| Move time-series to **BigQuery** as primary store (not export) | Purpose-built for analytics, cheaper per GB than Firestore |
| Use Firestore only for **current state + alerts + config** (not historical data) | Reduces read/write costs dramatically |
| Add alert deduplication with **cooldown timestamps** in Firestore | Prevents alert flooding |
| Add **Pub/Sub** between Firestore triggers and processing | Decouples trigger from processing, adds retry logic |

**Verdict:** Least disruption, gets you 80% of the benefit. The key insight is to stop using Firestore as a time-series database.

### My Recommendation

**For a single-greenhouse personal project:** **Option D (fix Firebase architecture)** is the pragmatic choice. You've already invested in Firebase, the free tier covers your usage, and the WebUI is already wired to Firestore. The critical fix is separating real-time state (Firestore) from historical time-series (BigQuery or a time-series DB).

**If you're considering making this a product / multi-greenhouse:** **Option A (Supabase with TimescaleDB)** gives you the best balance of developer experience, cost, real-time capabilities, and time-series support, with the option to self-host later.

**If you want maximum IoT best practices:** **Option C (MQTT + InfluxDB + Grafana)** is the industry standard stack for IoT sensor monitoring. Grafana alone would replace your entire WebUI for dashboarding, and MQTT is fundamentally better than HTTPS for constrained devices on flaky WiFi.

### Cloud Functions Refactoring Priorities (regardless of backend choice)

| Priority | Issue | Fix |
|---|---|---|
| **High** | Functions v1 API | Migrate to **v2 (Gen 2)** |
| **High** | No alert deduplication | Add cooldown logic in `processSensorData` trigger |
| **High** | Missing threshold checks | Add VWC, EC, pH, CO2 checks alongside temp/humidity |
| **High** | `exportToBigQuery` exports non-sensor fields | Filter `Object.keys(data)` to only numeric sensor fields |
| **Medium** | `isOffHours()` uses server timezone | Use greenhouse doc's timezone field |
| **Medium** | `node-schedule` dead dependency | Remove from `package.json` |
| **Medium** | No input validation in API endpoints | Add validation for `sensorType`, date ranges |
| **Low** | Inconsistent alert type naming | Standardize on `ULTRA`/`HIGH`/`MEDIUM`/`LOW` everywhere |

### WebUI Refactoring Priorities

| Priority | Issue | Fix |
|---|---|---|
| **High** | Missing `src/config.js` | Create a template or use environment variables |
| **High** | Missing Tailwind CSS config | Add `tailwind.config.js` and PostCSS config, or remove Tailwind classes |
| **High** | Stub pages (Analytics, Settings, Alerts) | Wire up to existing Cloud Function endpoints |
| **Medium** | No error boundary | Add React error boundary at root |
| **Medium** | Dashboard only shows 4 of ~15 sensors | Add remaining sensor displays |
| **Medium** | No alert acknowledgment UI | Add acknowledge button in `AlertList` |
| **Low** | GA tracking ID hardcoded | Move to environment variable |
| **Low** | Dead dependencies (`date-fns`, `@types/react`) | Remove from `package.json` |

---

**TL;DR:** The ESP32-WROOM-32E (or an ESP32-S3 upgrade) is the right hardware. Firebase is adequate but you should stop using Firestore as a time-series DB — keep it for real-time state only and route historical data to BigQuery or InfluxDB. The biggest wins come from fixing the existing architecture rather than replatforming entirely. 

Completed: *Provide hardware platform analysis* (6/7)