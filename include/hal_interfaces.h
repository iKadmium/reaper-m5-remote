#pragma once

#include <stdint.h>
#include <functional>
#include <string>

namespace hal
{

    // Network interface abstraction
    class INetworkManager
    {
    public:
        virtual ~INetworkManager() = default;

        // Connection management
        virtual bool connect(const char *ssid, const char *password) = 0;
        virtual bool disconnect() = 0;
        virtual bool isConnected() const = 0;
        virtual const char *getIP() const = 0;

        // HTTP client functionality (blocking only)
        virtual bool httpGetBlocking(const char *url, std::string &response, int &status_code) = 0;
    };

    // Power management interface abstraction
    class IPowerManager
    {
    public:
        virtual ~IPowerManager() = default;

        // Battery status
        virtual uint8_t getBatteryPercentage() const = 0;
        virtual bool isCharging() const = 0;

        // Power control
        virtual void deepSleep(uint32_t seconds) = 0;
        virtual void lightSleep(uint32_t milliseconds) = 0;
        virtual void restart() = 0;

        // Power saving
        virtual void setCpuFrequency(uint32_t mhz) = 0;
        virtual void enableWiFiPowerSave(bool enable) = 0;
        virtual void powerOff() = 0;
    };

    // Display interface abstraction
    class IDisplayManager
    {
    public:
        virtual ~IDisplayManager() = default;

        // Display control
        virtual void setBrightness(uint8_t brightness) = 0;
        virtual uint8_t getBrightness() const = 0;
        virtual void turnOn() = 0;
        virtual void turnOff() = 0;

        // Display properties
        virtual uint16_t getWidth() const = 0;
        virtual uint16_t getHeight() const = 0;
        virtual void *getFrameBuffer() = 0;

        // LVGL integration
        virtual void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p) = 0;
    };

    // Input interface abstraction
    class IInputManager
    {
    public:
        virtual ~IInputManager() = default;

        // Button states
        virtual bool isButtonPressed(uint8_t button_id) const = 0;
        virtual bool wasButtonPressed(uint8_t button_id) = 0;
        virtual bool wasButtonReleased(uint8_t button_id) = 0;

        // Touch interface (if available)
        virtual bool getTouchPoint(int16_t *x, int16_t *y) = 0;
        virtual bool isTouched() const = 0;

        // Update state (should be called in main loop)
        virtual void update() = 0;
    };

    // System abstraction - combines all interfaces
    class ISystemHAL
    {
    public:
        virtual ~ISystemHAL() = default;

        virtual INetworkManager &getNetworkManager() = 0;
        virtual IPowerManager &getPowerManager() = 0;
        virtual IDisplayManager &getDisplayManager() = 0;
        virtual IInputManager &getInputManager() = 0;

        // System lifecycle
        virtual void init() = 0;
        virtual void update() = 0;
        virtual uint32_t getMillis() const = 0;
        virtual void delay(uint32_t ms) = 0;
    };

} // namespace hal
