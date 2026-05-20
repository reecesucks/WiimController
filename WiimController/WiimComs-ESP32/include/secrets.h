#pragma once

// Copy this file to include/secrets.h and fill in real values.
// secrets.h is gitignored.

#define WIFI_SSID      "house of dumpling"
#define WIFI_PASSWORD  "dumptruck"

// Base URL of the Wiim device on your LAN.
// The Wiim HTTP API is reachable on http://<ip>/httpapi.asp?... ; HTTPS also
// works but presents a self-signed cert (handled via setInsecure()).
#define WIIM_BASE_URL  "https://192.168.0.116"