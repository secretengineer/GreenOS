# GreenOS Architecture Assessment (2026-03)

This review evaluates:
1. MCU/platform fit for greenhouse control
2. Backend/database fit for cost vs reliability
3. Refactor and feature roadmap

## Executive recommendation

- **Microcontroller/platform:** stay on ESP32 family. Current ESP32-WROOM-32E is fit-for-purpose; migrate to **ESP32-S3** only if you need more ADC headroom or longer platform runway.
- **Backend/database:** keep Firebase for identity, realtime status, and command/control, but stop treating Firestore as long-term time-series storage. Use Firestore for “current state + alerts + commands” and BigQuery (or Timescale/Influx) for historical sensor telemetry.
- **Codebase direction:** prioritize reliability and data-model fixes over new UI features. Several timestamp/type assumptions and schema gaps can cause silent failures under load.

---

## 1) Microprocessor and platform fit

## What is strong today

- Firmware is already structured around FreeRTOS tasks with separated sensor/network/actuation loops, which matches deterministic control requirements well.
- Safety-minded defaults exist: watchdog, explicit emergency state transitions, and local buffering primitives.
- ESP32 capabilities align with your device interfaces (Wi-Fi + UART/I2C/ADC/SPI usage, OTA intent, NVS usage).

## Risks observed in implementation

- The project currently uses **default partitioning** while enabling OTA + SPIFFS buffering. This can become a practical ceiling for robust OTA and retained logs on 4 MB modules.
- There are **two firmware code trees** (`Firmware/src/*.cpp` and `Firmware/src/main/*`) and a source filter excludes `main/`, which indicates legacy drift and maintenance ambiguity.
- The config template references constrained/strap-sensitive GPIO choices and ADC caveats; this is manageable but makes hardware reproducibility more fragile if expanded.

## Recommendation

- Keep ESP32-WROOM-32E for current deployment if boards are already in hand.
- For new hardware purchases, target ESP32-S3 modules with larger flash/PSRAM variants for better headroom.
- Immediately define a custom partition table for OTA + filesystem and document expected data retention size.

---

## 2) Backend + database fit (cost vs reliability)

## What works

- Firebase Auth + callable functions + Firestore model is a good velocity stack for a solo/small-team IoT control plane.
- Current architecture naturally supports realtime dashboards and command channels.

## Critical data-path concerns

- Scheduled and API code frequently treats Firestore timestamps as raw JS dates/numbers, which can break freshness checks, online/offline status, and export paths when timestamp fields are Firestore `Timestamp` objects.
- BigQuery export currently iterates all object keys and can export non-sensor fields unless filtered.
- Alerting logic is partial (temp/humidity/motion-centric), while your firmware and domain require broader checks (CO2, EC, pH, VWC, anomaly rates).
- Firestore as the primary store for high-frequency historical telemetry will eventually become cost-inefficient versus columnar/time-series sinks.

## Recommendation

- Keep Firebase as the control plane (auth, config, command, latest status, alerts).
- Move historical telemetry to BigQuery-first (or Timescale/Influx if you prefer SQL/self-host).
- Add a strict sensor schema and export contract so only typed sensor metrics flow to analytics.
- Introduce alert dedup/cooldown windows to avoid notification storms and reduce write amplification.

---

## 3) Refactor / modifications / additional features

## Highest-priority refactors (do first)

1. **Timestamp normalization layer**
   - Normalize all timestamps in Cloud Functions before arithmetic/comparison.
   - Apply in status checks, health checks, daily summary boundaries, and exports.

2. **Schema hardening + validation**
   - Define canonical payload for `sensors` documents (field names, units, numeric requirements).
   - Validate callable function inputs (`sensorType`, date range, greenhouse membership).

3. **Partition/storage strategy on firmware**
   - Replace default partition assumptions with explicit OTA/logging partitions.
   - Add retention policy and bounded offline queue semantics.

4. **Repository cleanup**
   - Remove/archive legacy firmware subtree to prevent accidental regressions and confusion.

## High-value feature additions

- **Device digital twin doc** (`greenhouses/{id}/status/current`): derived online flag, firmware version, last heartbeat, sensor health summary.
- **Command ACK protocol**: command accepted/executed/failed with latency metrics, not just fire-and-forget writes.
- **Shadow config versioning**: include `configVersion` and rollback slot on device.
- **SLO dashboarding**: uptime %, sensor freshness SLA, alert MTTA/MTTR.
- **Rule simulation mode**: test threshold changes against last 7 days before applying.

## Suggested roadmap

- **Phase 1 (1-2 weeks):** timestamp/type fixes, schema contract, alert dedup, legacy code cleanup.
- **Phase 2 (2-4 weeks):** BigQuery-first historical pipeline, dashboard status doc, command ACKs.
- **Phase 3 (ongoing):** predictive controls, weather-informed scheduling, multi-greenhouse tenancy hardening.

---

## Decision summary

- **You are on the right MCU family/platform.** Keep ESP32; optionally step up to ESP32-S3 for expansion margin.
- **You are on an acceptable backend platform.** Keep Firebase for control plane; adjust storage architecture for telemetry economics.
- **You should refactor now before broad feature expansion.** Most risk is in data contracts/timestamps and operational reliability, not missing UI components.
