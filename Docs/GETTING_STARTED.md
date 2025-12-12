# GreenOS - Getting Started Guide

## 🎯 Quick Start Overview

This guide will walk you through setting up GreenOS Phase 1 using Firebase for rapid prototyping. Follow these steps in order to get your greenhouse controller up and running.

## 📋 Prerequisites

### Hardware
- Arduino UNO Q (or compatible ESP32-based board)
- USB-C cable (5V @ 3A power supply)
- Sensors (see [HARDWARE.md](HARDWARE.md) for complete BOM)
- Actuator relays
- WiFi network access

### Software & Accounts
- [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO
- [Node.js](https://nodejs.org/) v18 or later
- [Firebase account](https://firebase.google.com/) (free tier works)
- [Google Cloud account](https://cloud.google.com/) (optional for BigQuery)
- Git

---

## 🔥 Step 1: Firebase Project Setup

### 1.1 Create Firebase Project

1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Click "Add Project"
3. Name it "GreenOS" (or your preference)
4. Disable Google Analytics (optional)
5. Click "Create Project"

### 1.2 Enable Firebase Services

**Firestore Database:**
1. Navigate to "Firestore Database"
2. Click "Create database"
3. Start in **production mode**
4. Choose a location close to you
5. Click "Enable"

**Authentication:**
1. Navigate to "Authentication"
2. Click "Get started"
3. Enable "Email/Password" sign-in method
4. Save

**Cloud Storage:**
1. Navigate to "Storage"
2. Click "Get started"
3. Use default security rules
4. Choose same location as Firestore
5. Click "Done"

**Firebase Hosting:**
1. Navigate to "Hosting"
2. Click "Get started"
3. Follow the setup wizard (we'll deploy later)

### 1.3 Get Firebase Configuration

1. Go to Project Settings (gear icon)
2. Scroll to "Your apps"
3. Click the web icon (`</>`)
4. Register your app as "GreenOS Web"
5. Copy the `firebaseConfig` object - you'll need this!

---

## 🔧 Step 2: Arduino Firmware Setup

### 2.1 Install Arduino IDE & Libraries

**Required Libraries:**
```
- Firebase ESP Client (by Mobizt)
- DHT sensor library
- Adafruit Unified Sensor
- ArduinoJson
```

Install via Arduino IDE: `Tools > Manage Libraries`

### 2.2 Configure Firmware

1. Navigate to `Firmware/src/`
2. Copy `config.example.h` to `config.h`:
   ```powershell
   cp Firmware\src\config.example.h Firmware\src\config.h
   ```
3. Edit `config.h` with your credentials:
   ```cpp
   #define WIFI_SSID "YourWiFiName"
   #define WIFI_PASSWORD "YourWiFiPassword"
   #define FIREBASE_HOST "greenos-xxxxx.firebaseio.com"
   #define API_KEY "your-firebase-api-key"
   ```

### 2.3 Upload Firmware

1. Connect Arduino UNO Q via USB
2. Open `Firmware/src/main.cpp` in Arduino IDE
3. Select board: `Tools > Board > ESP32 Arduino > ESP32 Dev Module`
4. Select port: `Tools > Port > COM# (Arduino UNO Q)`
5. Click Upload (→)
6. Open Serial Monitor to verify connection

---

## ☁️ Step 3: Deploy Cloud Functions

### 3.1 Install Firebase CLI

```powershell
npm install -g firebase-tools
```

### 3.2 Login to Firebase

```powershell
firebase login
```

### 3.3 Initialize Firebase Project

```powershell
cd CloudFunctions
firebase init
```

Select:
- ✓ Functions
- ✓ Firestore
- ✓ Hosting
- Use existing project: GreenOS
- JavaScript (or TypeScript if preferred)
- Install dependencies: Yes

### 3.4 Install Dependencies

```powershell
cd functions
npm install
```

### 3.5 Deploy Cloud Functions

```powershell
firebase deploy --only functions
```

This will deploy all backend logic to Firebase.

---

## 🌐 Step 4: Deploy Web UI

### 4.1 Configure Web App

1. Navigate to `WebUI/src/`
2. Edit `config.js`:
   ```javascript
   export const firebaseConfig = {
     apiKey: "your-api-key",
     authDomain: "greenos-xxxxx.firebaseapp.com",
     projectId: "greenos-xxxxx",
     storageBucket: "greenos-xxxxx.appspot.com",
     messagingSenderId: "1234567890",
     appId: "1:1234567890:web:abcdef"
   };
   
   export const greenhouseId = "denver_greenhouse_01";
   ```

### 4.2 Install Dependencies

```powershell
cd WebUI
npm install
```

### 4.3 Test Locally

```powershell
npm run dev
```

Visit `http://localhost:3000` to see your app!

### 4.4 Deploy to Firebase Hosting

```powershell
npm run build
firebase deploy --only hosting
```

Your app will be live at `https://greenos-xxxxx.web.app`

---

## 🔒 Step 5: Set Up Firestore Security Rules

### 5.1 Configure Security Rules

In Firebase Console, go to Firestore > Rules and update:

```javascript
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    // Allow authenticated users to read/write their greenhouses
    match /greenhouses/{greenhouseId} {
      allow read, write: if request.auth != null && 
        request.auth.uid in resource.data.users;
      
      match /sensors/{sensorId} {
        allow read: if request.auth != null;
        allow write: if request.auth != null;
      }
      
      match /alerts/{alertId} {
        allow read: if request.auth != null;
        allow write: if request.auth != null;
      }
      
      match /logs/{logId} {
        allow read: if request.auth != null;
        allow write: if request.auth != null;
      }
    }
  }
}
```

Click "Publish"

---

## 👤 Step 6: Create User Account

### 6.1 Create First User

1. Visit your deployed web app
2. Click "Sign Up"
3. Enter email and password
4. Click "Sign Up"

### 6.2 Create Greenhouse Document

In Firestore Console, manually create:

**Collection:** `greenhouses`  
**Document ID:** `denver_greenhouse_01`  
**Fields:**
```json
{
  "name": "Denver Greenhouse",
  "location": {
    "city": "Denver",
    "state": "CO",
    "lat": 39.7392,
    "lon": -104.9903
  },
  "users": ["user-uid-from-auth"],
  "thresholds": {
    "tempMin": 10,
    "tempMax": 35,
    "humidityMin": 40,
    "humidityMax": 80,
    "vwcMin": 20,
    "vwcMax": 60
  },
  "createdAt": [timestamp],
  "userTokens": []
}
```

---

## ✅ Step 7: Test End-to-End

### 7.1 Verify Arduino Connection

1. Check Serial Monitor - should see "WiFi connected!"
2. Should see periodic sensor readings
3. Data should appear in Firestore Console under `greenhouses/denver_greenhouse_01/sensors`

### 7.2 Verify Web Dashboard

1. Login to web app
2. Dashboard should show real-time sensor readings
3. Gauges should update automatically
4. Charts should populate with historical data

### 7.3 Test Alerts

Manually create an alert in Firestore to test notifications:

**Path:** `greenhouses/denver_greenhouse_01/alerts`  
**Fields:**
```json
{
  "type": "TEST_ALERT",
  "severity": "medium",
  "message": "This is a test alert",
  "timestamp": [timestamp],
  "acknowledged": false
}
```

Should appear in Dashboard alerts list!

---

## 🐛 Troubleshooting

### Arduino won't connect to WiFi
- Double-check SSID and password in `config.h`
- Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)
- Check Serial Monitor for error messages

### Firebase permission denied
- Verify Firestore security rules are published
- Ensure user UID is in greenhouse `users` array
- Check that you're logged in

### Web app blank page
- Check browser console for errors
- Verify Firebase config in `config.js`
- Ensure Firebase services are enabled
- Try clearing cache and hard refresh

### No sensor data in dashboard
- Verify Arduino is uploading data (check Serial Monitor)
- Check Firestore Console - data should be there
- Verify `greenhouseId` matches in both Arduino and Web config

---

## 📊 Optional: Enable BigQuery

For historical data analytics:

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Enable BigQuery API
3. Create dataset: `greenos`
4. Create table: `sensor_data` with schema:
   ```
   greenhouse_id: STRING
   timestamp: TIMESTAMP
   sensor_type: STRING
   value: FLOAT
   ```
5. Cloud Functions will automatically export data hourly

---

## 🎉 Success!

Your GreenOS system is now running! You should have:

✅ Arduino collecting sensor data  
✅ Real-time data in Firestore  
✅ Cloud Functions processing and alerting  
✅ Web dashboard displaying live data  
✅ Secure authentication and access control  

## 📚 Next Steps

- [Hardware Setup Guide](HARDWARE.md) - Complete sensor wiring
- [API Documentation](API.md) - Integrate with other systems
- [Contributing Guide](CONTRIBUTING.md) - Help improve GreenOS

## 💡 Need Help?

- Check [Issues](https://github.com/patryan/greenos/issues)
- Email: pat@patryan.com
