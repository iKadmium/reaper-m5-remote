#pragma once

#ifdef NATIVE_BUILD
#include <spdlog/spdlog.h>
#include <fmt/format.h>

#define LOG_TRACE(tag, fmt_str, ...) SPDLOG_TRACE(fmt_str, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt_str, ...) SPDLOG_DEBUG(fmt_str, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt_str, ...) SPDLOG_INFO(fmt_str, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt_str, ...) SPDLOG_WARN(fmt_str, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt_str, ...) SPDLOG_ERROR(fmt_str, ##__VA_ARGS__)
#define LOG_CRITICAL(tag, fmt_str, ...) SPDLOG_CRITICAL(fmt_str, ##__VA_ARGS__)
#else
#include <Arduino.h>

// Helper function to convert fmt-style to printf-style for Arduino
template <typename... Args>
void log_printf(const char *fmt_str, Args &&...args)
{
    // For Arduino, we'll use a simple approach that works with basic formatting
    Serial.printf(fmt_str, args...);
    Serial.println();
}

#define LOG_TRACE(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#define LOG_WARNING(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#define LOG_CRITICAL(tag, fmt_str, ...) log_printf(fmt_str, ##__VA_ARGS__)
#endif