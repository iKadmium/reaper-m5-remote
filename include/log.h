#pragma once

#ifdef NATIVE_BUILD
#include <cstdio>
#include <ctime>
#include <iostream>

// Simple logging for native build using printf
#define LOG_TRACE(tag, fmt_str, ...) printf("[TRACE][%s] " fmt_str "\n", tag, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt_str, ...) printf("[DEBUG][%s] " fmt_str "\n", tag, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt_str, ...) printf("[INFO][%s] " fmt_str "\n", tag, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt_str, ...) printf("[WARN][%s] " fmt_str "\n", tag, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt_str, ...) printf("[ERROR][%s] " fmt_str "\n", tag, ##__VA_ARGS__)
#define LOG_CRITICAL(tag, fmt_str, ...) printf("[FATAL][%s] " fmt_str "\n", tag, ##__VA_ARGS__)

// Initialize logging for native build
inline void init_logging()
{
    printf("=== Native logging initialized ===\n");
}

#else
#include <Arduino.h>
#include <ArduinoLog.h>

// For Arduino build, use printf-style format strings directly
#define LOG_TRACE(tag, fmt_str, ...) Log.traceln("[%s] " fmt_str, tag, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt_str, ...) Log.verboseln("[%s] " fmt_str, tag, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt_str, ...) Log.infoln("[%s] " fmt_str, tag, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt_str, ...) Log.warningln("[%s] " fmt_str, tag, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt_str, ...) Log.errorln("[%s] " fmt_str, tag, ##__VA_ARGS__)
#define LOG_CRITICAL(tag, fmt_str, ...) Log.fatalln("[%s] " fmt_str, tag, ##__VA_ARGS__)

// Initialize logging for Arduino build
inline void init_logging()
{
    // Wait for Serial to be ready
    delay(100);
    while (!Serial)
    {
        delay(10);
    }

    Log.begin(LOG_LEVEL_INFO, &Serial);
    Log.setPrefix([](Print *_logOutput, int logLevel)
                  {
        const char* levelStr = "";
        switch(logLevel) {
            case 0: levelStr = "SILENT"; break;
            case 1: levelStr = "FATAL"; break;
            case 2: levelStr = "ERROR"; break;
            case 3: levelStr = "WARN"; break;
            case 4: levelStr = "INFO"; break;
            case 5: levelStr = "VERBOSE"; break;
            case 6: levelStr = "TRACE"; break;
            default: levelStr = "UNKNOWN"; break;
        }
        _logOutput->printf("[%lu][%s] ", millis(), levelStr); });
    Log.setSuffix([](Print *_logOutput, int logLevel)
                  {
                      // No suffix needed
                  });

    // Print a startup message to verify logging is working
    Log.infoln("=== Logging initialized ===");
}

#endif