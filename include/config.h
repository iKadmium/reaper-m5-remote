#pragma once

// WiFi Configuration
// You can modify these values or create a config.cpp file to override them
#ifndef WIFI_SSID
#define WIFI_SSID "YourWiFiNetwork"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "YourWiFiPassword"
#endif

// Network Configuration
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 20000 // 20 seconds
#endif

#ifndef WIFI_RETRY_ATTEMPTS
#define WIFI_RETRY_ATTEMPTS 3
#endif

// Reaper Server Configuration
#ifndef REAPER_SERVER
#define REAPER_SERVER "192.168.1.100"
#endif

#ifndef REAPER_PORT
#define REAPER_PORT 8080
#endif

// You can also create a config.cpp file to define these at runtime:
/*
// config.cpp example:
#include "config.h"

const char* getWiFiSSID() {
    return "MyActualNetwork";
}

const char* getWiFiPassword() {
    return "MyActualPassword";
}

const char* getReaperServer() {
    return "192.168.1.50";
}

int getReaperPort() {
    return 8080;
}
*/

// Function declarations for runtime config (optional)
#ifdef __cplusplus
extern "C"
{
#endif

    // Optional: Override with runtime functions
    const char *getWiFiSSID();
    const char *getWiFiPassword();
    const char *getReaperServer();
    int getReaperPort();

#ifdef __cplusplus
}
#endif
