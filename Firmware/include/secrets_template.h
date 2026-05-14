/**
 * GreenOS - Secrets Template
 *
 * SETUP INSTRUCTIONS:
 *   1. Copy this file to secrets.h  (same directory)
 *   2. Fill in your actual credentials below
 *   3. secrets.h is gitignored — never commit it
 *
 * secrets.h defines all credential macros that config.h depends on.
 */

#ifndef SECRETS_H
#define SECRETS_H

// ----------------------------------------------------------------------------
// WiFi — primary network
// ----------------------------------------------------------------------------
#define WIFI_SSID           "YOUR_WIFI_SSID"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"

// ----------------------------------------------------------------------------
// WiFi Manager fallback AP password
// (The AP SSID is set in config.h as WIFI_AP_SSID = "GreenOS-Setup")
// ----------------------------------------------------------------------------
#define WIFI_AP_PASSWORD    "YOUR_AP_PASSWORD"

// ----------------------------------------------------------------------------
// Firebase Web API key
// Found in Firebase Console → Project Settings → General → Web API Key
// ----------------------------------------------------------------------------
#define FIREBASE_API_KEY    "YOUR_FIREBASE_API_KEY"

// ----------------------------------------------------------------------------
// OTA update password
// Must match the --auth value used in platformio.ini and on the device
// Set the GREENOS_OTA_PASSWORD environment variable on your build machine
// ----------------------------------------------------------------------------
#define GREENOS_OTA_PASSWORD "YOUR_OTA_PASSWORD"

#endif // SECRETS_H
