# GreenOS README Addendum: Hostinger Migration Proposal for `greenOS.app`

## Purpose

This addendum proposes a practical refactor plan for moving GreenOS off Firebase Hosting and onto infrastructure you control through Hostinger under the production domain `greenOS.app`.

The proposal is based on the current repository layout and implementation status of:

- `Firmware/` (ESP32 device firmware)
- `WebUI/` (React + Vite dashboard)
- `CloudFunctions/functions/` (Firebase Cloud Functions backend)
- root Firebase deployment/configuration files and Firebase-oriented setup docs

This document treats the migration as a **full hosting and platform decoupling effort**, not just a static-site redeploy, because the current codebase is tightly coupled to Firebase well beyond Hosting.

---

## Executive Summary

GreenOS should move to a **Hostinger VPS-backed architecture** with:

- `greenOS.app` serving the web dashboard
- `api.greenos.app` serving the application API
- optional `mqtt.greenos.app` for device messaging if you adopt MQTT
- PostgreSQL + TimescaleDB (recommended) for application and telemetry data
- a Node.js backend replacing Firebase Cloud Functions
- a staged firmware migration away from the Firebase Arduino client

### Recommended strategy

Use a **two-stage migration**:

1. **Stage 1: Move hosting first**
   - Host the built WebUI on Hostinger
   - Keep Firebase Auth / Firestore temporarily
   - Prove DNS, SSL, deployment, and frontend delivery under `greenOS.app`

2. **Stage 2: Replace Firebase services**
   - Introduce a backend service on Hostinger
   - Migrate auth, API, telemetry storage, alerts, scheduled jobs, and device communication
   - Remove Firebase-specific code and deployment artifacts after cutover

This minimizes downtime and lets the greenhouse keep operating while the platform is refactored.

---

## Current Repository Findings

## 1. Firebase Hosting is only one part of the dependency chain

The repository currently depends on Firebase in at least four layers:

### Frontend (`WebUI/`)

- `WebUI/src/App.jsx` initializes Firebase directly and uses Firebase Auth
- `WebUI/src/pages/Login.jsx` uses Firebase email/password auth
- `WebUI/src/pages/Dashboard.jsx`, `WebUI/src/components/SensorChart.jsx`, and `WebUI/src/components/AlertList.jsx` read Firestore directly with real-time listeners
- The current frontend build is also coupled to a missing `src/config.js`, which means configuration handling needs cleanup before any hosting change

### Backend (`CloudFunctions/functions/`)

- `index.js` exports Firestore triggers, callable functions, and scheduled jobs
- `api.js` depends on Firebase Auth context, Firestore, and BigQuery
- `triggers.js` depends on Firestore writes to generate alerts and notifications
- `scheduled.js` depends on Firestore and BigQuery export workflows

### Firmware (`Firmware/`)

- `Firmware/include/network_manager.h` and `Firmware/src/network_manager.cpp` directly depend on `Firebase_ESP_Client`
- device sync, alert publishing, config fetches, and command polling all use Firestore document paths
- `Firmware/include/config_template.h` contains Firebase host, API key, and project identifiers

### Infrastructure and docs

- `firebase.json` and `.firebaserc` define Hosting and Functions deployment
- `Docs/GETTING_STARTED.md` instructs users to deploy both Hosting and Functions through Firebase
- `README.md` still presents Firebase as the platform backend

## 2. The codebase is not yet portable

Today, GreenOS is not simply “hosted on Firebase”; it is **architected around Firebase APIs**:

- authentication is Firebase-native
- real-time dashboard reads are Firebase-native
- backend automation is Firebase-native
- device/cloud sync is Firebase-native

Moving only the web files to Hostinger would change where the SPA is served from, but it would **not** remove Firebase as the control plane.

## 3. Existing repo issues increase migration urgency

During review, the following issues stood out:

- `WebUI` build fails because `src/config.js` is missing
- `WebUI` lint is not configured for the installed ESLint version
- `CloudFunctions/functions` has no ESLint config
- `CloudFunctions/functions` has no tests

Those gaps do not block writing this proposal, but they reinforce the need to formalize configuration, contracts, and deployment boundaries before migration.

---

## Target Architecture on Hostinger

## Recommended deployment model

Use a **Hostinger VPS**, not basic shared hosting, for the final GreenOS platform.

### Why VPS is the right fit

GreenOS needs more than static file hosting:

- long-running API processes
- background jobs
- secure secret management
- database connectivity
- optional MQTT broker
- reverse proxy and TLS termination

Shared hosting is acceptable only for a temporary frontend-only move. It is not a good long-term fit for the full GreenOS stack.

## Proposed service layout

### Public endpoints

- `https://greenOS.app` or `https://greenos.app`
  - serves the React dashboard
- `https://api.greenos.app`
  - serves REST and/or WebSocket APIs
- `https://mqtt.greenos.app` (optional)
  - device messaging endpoint if MQTT is adopted

### Core runtime components

- **Nginx**
  - TLS termination
  - SPA routing
  - reverse proxy to backend services
- **WebUI static build**
  - generated from `WebUI/dist`
- **GreenOS API service**
  - Node.js service replacing Firebase callable functions and triggers
- **PostgreSQL**
  - users, greenhouses, config, alerts, commands, audit logs
- **TimescaleDB extension**
  - recommended for sensor telemetry and historical queries
- **Redis** (optional but recommended)
  - queues, caching, rate limiting, transient command state
- **Scheduler/worker process**
  - periodic jobs now implemented as Firebase scheduled functions
- **Mosquitto MQTT broker** (optional, recommended for later phase)
  - more efficient device communication than repeated HTTPS polling

---

## Recommended Refactor Direction by Subsystem

## 1. WebUI refactor

### Current state

The frontend is directly coupled to Firebase SDKs for:

- app bootstrap
- authentication
- Firestore subscriptions

### Refactor goal

Move the frontend to a backend-agnostic client architecture.

### Required changes

1. Replace Firebase SDK usage with a thin application client layer
   - add `src/lib/apiClient`
   - add `src/lib/authClient`
   - centralize base URL and auth handling

2. Replace Firestore listeners with backend APIs
   - dashboard current state endpoint
   - alerts endpoint
   - historical telemetry endpoint
   - settings/config endpoints

3. Replace Firebase Auth with session or token auth
   - recommended: HTTP-only cookie sessions for users
   - fallback: JWT bearer tokens if the frontend must remain fully decoupled

4. Move config to environment variables
   - replace missing `src/config.js` with `.env`-driven runtime configuration
   - required variables should include API base URL and greenhouse defaults

5. Preserve SPA hosting compatibility
   - keep client-side routes working behind Nginx rewrites

### Outcome

After this refactor, the WebUI can be served from Hostinger without any Hosting-provider lock-in.

---

## 2. Backend refactor

### Current state

The repository has backend logic, but it is fragmented into Firebase-specific execution modes:

- callable functions
- Firestore triggers
- scheduled jobs

### Refactor goal

Consolidate backend logic into a standard application service that can run on Hostinger.

### Proposed backend modules

- `backend/src/routes`
  - auth
  - telemetry
  - alerts
  - commands
  - settings
  - analytics
- `backend/src/services`
  - alert evaluation
  - notification delivery
  - greenhouse authorization
  - telemetry aggregation
- `backend/src/jobs`
  - daily summary generation
  - device health checks
  - weather fetch
  - retention/downsampling jobs
- `backend/src/db`
  - repositories and schema access

### Firebase mapping to new backend

| Current Firebase behavior | Hostinger replacement |
|---|---|
| Callable functions | REST endpoints or RPC-style API routes |
| Firestore triggers | application service logic triggered on write/ingest |
| Cloud Scheduler jobs | cron + worker process |
| Firebase Admin auth checks | backend session/JWT middleware |
| BigQuery analytics queries | SQL/Timescale queries or warehouse sync job |

### Recommendation

Refactor the existing Cloud Functions logic into a conventional Node.js service rather than trying to emulate Firebase semantics on Hostinger.

---

## 3. Data model refactor

### Current state

The codebase uses Firestore collections such as:

- `greenhouses/{id}/sensors`
- `greenhouses/{id}/alerts`
- `greenhouses/{id}/commands`
- `greenhouses/{id}/logs`
- `greenhouses/{id}/weather`
- `greenhouses/{id}/daily_summaries`

### Refactor goal

Adopt a storage model that separates:

- operational state
- historical telemetry
- user/account data
- jobs and notifications

### Recommended relational/time-series split

#### PostgreSQL relational tables

- `users`
- `greenhouses`
- `greenhouse_memberships`
- `devices`
- `alerts`
- `commands`
- `command_results`
- `config_versions`
- `audit_logs`
- `weather_observations`
- `daily_summaries`

#### Timescale hypertables

- `sensor_readings`
- optional `device_health_events`

### Why this is better

- easier reporting than Firestore
- better time-series retention and aggregation
- easier backup and restore
- better portability between providers
- no frontend coupling to proprietary document APIs

---

## 4. Firmware refactor

### Current state

The firmware is the most sensitive migration surface because it currently:

- authenticates through Firebase
- writes sensor data to Firestore
- reads commands from Firestore
- fetches remote config from Firestore
- sends alerts through Firestore

### Refactor goal

Introduce a provider-neutral cloud transport layer.

### Required changes

1. Replace Firebase-specific network manager responsibilities with an abstraction
   - `CloudClient` or similar interface
   - implementations for temporary Firebase mode and new Hostinger backend mode during migration

2. Replace Firestore document writes with one of:
   - HTTPS JSON API calls
   - MQTT publish events

3. Replace Firestore command polling with:
   - command fetch endpoint plus ACK response, or
   - MQTT subscription for device commands

4. Replace Firebase config fetch with:
   - signed config endpoint
   - versioned config payloads

5. Replace Firebase authentication with device credentials
   - per-device secret
   - API key + HMAC signature, or
   - short-lived JWT minted by backend after device registration

### Recommendation

For GreenOS, the cleanest long-term model is:

- **MQTT for commands and state changes**
- **HTTPS for device registration, firmware metadata, and larger config payloads**

If you want the least firmware churn first, start with HTTPS-only and add MQTT later.

---

## 5. Authentication and authorization refactor

### User auth

Replace Firebase email/password auth with one of:

- custom backend auth with secure password hashing and session cookies
- an external identity provider integrated with the Hostinger backend

For simplicity and ownership, a custom backend auth flow with:

- Argon2 or bcrypt password hashing
- email/password login
- secure cookie sessions
- optional TOTP for admin users

is sufficient for the current project scale.

### Device auth

Define a device identity model:

- each ESP32 gets a unique device record
- each device has a secret or certificate
- every telemetry submission is signed or token-authenticated

### Authorization

Move greenhouse membership checks into backend middleware and repository logic instead of relying on Firestore rules.

---

## 6. Notifications and automation

### Current state

Alerts are generated through Firestore-triggered logic, and notification fan-out is tied to Firebase messaging assumptions.

### Refactor goal

Move alert generation and delivery into explicit backend workflows.

### Required changes

- evaluate thresholds during ingest or immediately after ingest
- add alert cooldown/deduplication
- store notification delivery state
- use pluggable notification channels:
  - email
  - SMS
  - push later if desired

This will make alert behavior testable and independent of database side effects.

---

## 7. Documentation and repository refactor

### Files that should eventually be retired or rewritten

- `/firebase.json`
- `/.firebaserc`
- Firebase deployment instructions in `Docs/GETTING_STARTED.md`
- Firebase-focused setup sections in `README.md`
- Firebase keys in `Firmware/include/config_template.h`

### Files that should be added during migration

- `backend/` service directory
- `.env.example` files for backend and WebUI
- deployment docs for Hostinger VPS
- database schema docs
- device API contract docs

---

## Phased Migration Plan

## Phase 0: Stabilize the current repo before migration

### Objectives

- fix broken WebUI configuration handling
- define environment variable strategy
- document current Firebase data contracts
- inventory every Firebase dependency path

### Deliverables

- frontend config refactor plan
- device API contract draft
- current collection/schema map

## Phase 1: Move the web frontend to Hostinger first

### Objectives

- serve the WebUI from Hostinger under `greenOS.app`
- keep Firebase backend services temporarily
- verify domain, SSL, caching, and SPA routing

### Refactor scope

- make WebUI build from env vars
- produce a stable `npm run build`
- configure Hostinger/Nginx rewrite to `index.html`
- point DNS for `greenOS.app`

### Benefits

- immediate removal of Firebase Hosting dependency
- low operational risk
- minimal firmware impact

## Phase 2: Stand up the new backend on Hostinger

### Objectives

- deploy Node.js API service to Hostinger VPS
- stand up PostgreSQL + TimescaleDB
- implement user auth, telemetry ingest, commands, alerts, and config endpoints

### Refactor scope

- port Cloud Functions business logic into backend services
- add database schema and migrations
- add worker/scheduler jobs
- define notification integrations

## Phase 3: Migrate the WebUI off Firebase services

### Objectives

- remove Firebase Auth and Firestore SDK usage
- switch the dashboard to the new API

### Refactor scope

- build auth client
- build telemetry, alerts, settings, and analytics API calls
- replace real-time Firestore listeners with polling, SSE, or WebSocket streams

## Phase 4: Migrate firmware off Firebase

### Objectives

- move device sync, commands, alerts, and config retrieval to the new backend

### Refactor scope

- replace `Firebase_ESP_Client`
- add secure device authentication
- add ingest and command acknowledgment protocol
- preserve offline buffering and safe-fail behavior

## Phase 5: Cut over and remove Firebase artifacts

### Objectives

- switch production traffic completely
- archive old Firebase configs
- update docs to the new operational model

### Exit criteria

- no production dependency on Firebase Hosting
- no production dependency on Firebase Auth
- no production dependency on Firestore or Cloud Functions

---

## Recommended API and Messaging Shape

## Minimum HTTP API set

### User-facing

- `POST /auth/login`
- `POST /auth/logout`
- `GET /greenhouses/:id/status`
- `GET /greenhouses/:id/alerts`
- `GET /greenhouses/:id/telemetry`
- `PATCH /greenhouses/:id/config`
- `POST /greenhouses/:id/alerts/:alertId/ack`

### Device-facing

- `POST /devices/register`
- `POST /devices/:id/telemetry`
- `POST /devices/:id/alerts`
- `GET /devices/:id/commands`
- `POST /devices/:id/commands/:commandId/ack`
- `GET /devices/:id/config`

## Optional MQTT topics

- `greenos/devices/{deviceId}/telemetry`
- `greenos/devices/{deviceId}/alerts`
- `greenos/devices/{deviceId}/commands`
- `greenos/devices/{deviceId}/status`

---

## Deployment Proposal for Hostinger

## Production topology

- Hostinger VPS
- Ubuntu LTS
- Docker Compose
- Nginx reverse proxy
- automatic TLS via Let's Encrypt
- persistent Docker volumes for database and logs
- nightly database backups to off-server storage

## Example container layout

- `nginx`
- `greenos-web`
- `greenos-api`
- `greenos-worker`
- `postgres`
- `redis`
- optional `mosquitto`

## DNS plan

- `greenos.app` → VPS public IP
- `www.greenos.app` → redirect to apex
- `api.greenos.app` → same VPS, routed by Nginx
- `mqtt.greenos.app` → same VPS or dedicated host if MQTT is enabled

---

## Risks and Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Frontend moved before backend is ready | partial migration confusion | treat hosting migration and backend migration as separate milestones |
| Firmware migration introduces greenhouse downtime | high | preserve dual-mode support until Hostinger backend is proven |
| Realtime UX regresses when leaving Firestore | medium | add SSE/WebSocket stream or short polling for live dashboard cards |
| Self-hosted auth/security errors | high | keep auth scope small first, use secure cookies, strong password hashing, rate limiting, audit logs |
| VPS operational burden | medium | use Docker Compose, backups, uptime checks, and minimal service count initially |
| Data migration quality issues | medium | backfill historical data with validation scripts and parallel run comparisons |

---

## Recommended Order of Work

1. Fix frontend configuration and build portability
2. Move `greenOS.app` static hosting to Hostinger
3. Build the new backend alongside Firebase
4. Migrate WebUI reads and auth
5. Migrate firmware writes, commands, and config
6. Remove Firebase deployment files and rewrite setup docs

This order gives you visible progress early while protecting the live device workflow.

---

## Acceptance Criteria for the Migration

GreenOS can be considered successfully migrated when:

- `greenOS.app` is served from Hostinger with valid HTTPS
- the dashboard builds and deploys without Firebase Hosting
- user authentication works without Firebase Auth
- telemetry, alerts, commands, and settings work through the new backend
- firmware no longer depends on `Firebase_ESP_Client`
- scheduled jobs run on Hostinger infrastructure
- setup docs no longer require Firebase CLI or Firebase Hosting

---

## Final Recommendation

Do **not** treat this as a simple Hosting swap.

The safest and most maintainable path is:

- **immediately** decouple the frontend from Hosting-specific assumptions
- **short-term** move the WebUI to Hostinger under `greenOS.app`
- **medium-term** replace Firebase services with a Hostinger VPS backend and a portable data model
- **long-term** move the ESP32 transport layer to HTTPS and/or MQTT so the greenhouse platform is no longer tied to Firebase APIs

If you want the least risky path, start by moving only the web hosting. If you want full platform ownership under Hostinger, plan for a broader refactor across WebUI, backend, firmware, docs, and deployment workflows.
