#pragma once

#ifdef ARDUINO

#include "hal_interfaces.h"
#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <lvgl.h>
#include <Wire.h>

namespace hal
{

    class M5StackNetworkManager : public INetworkManager
    {
    private:
        HTTPClient http;
        bool connected = false;
        String ip_address;

    public:
        bool connect(const char *ssid, const char *password) override
        {
            WiFi.begin(ssid, password);

            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 20)
            {
                delay(500);
                attempts++;
            }

            connected = (WiFi.status() == WL_CONNECTED);
            if (connected)
            {
                ip_address = WiFi.localIP().toString();
            }
            return connected;
        }

        bool disconnect() override
        {
            WiFi.disconnect();
            connected = false;
            ip_address = "";
            return true;
        }

        bool isConnected() const override
        {
            LOG_INFO("WIFI", "Connected %d, Wifi status %d", connected, WiFi.status());
            return connected && WiFi.status() == WL_CONNECTED;
        }

        const char *getIP() const override
        {
            return ip_address.c_str();
        }

        bool httpGetBlocking(const char *url, std::string &response, int &status_code) override
        {
            http.begin(url);
            status_code = http.GET();

            if (status_code > 0)
            {
                String payload = http.getString();
                response = payload.c_str();
            }
            else
            {
                response.clear();
            }

            http.end();
            return status_code > 0;
        }
    };

    class M5StackPowerManager : public IPowerManager
    {
    public:
        uint8_t getBatteryPercentage() const override
        {
            uint8_t battery_level = M5.Power.getBatteryLevel();
            LOG_INFO("PowerManager", "Battery level: %d%%", battery_level);
            return battery_level;
        }

        bool isCharging() const override
        {
            return M5.Power.isCharging();
        }

        void deepSleep(uint32_t seconds) override
        {
            M5.Power.deepSleep(seconds * 1000000ULL);
        }

        void lightSleep(uint32_t milliseconds) override
        {
            M5.Power.lightSleep(milliseconds * 1000ULL);
        }

        void powerOff() override
        {
            M5.Power.powerOFF();
        }

        void restart() override
        {
            ESP.restart();
        }

        void setCpuFrequency(uint32_t mhz) override
        {
            setCpuFrequencyMhz(mhz);
        }

        void enableWiFiPowerSave(bool enable) override
        {
            if (enable)
            {
                WiFi.setSleep(true);
            }
            else
            {
                WiFi.setSleep(false);
            }
        }
    };

    class M5StackDisplayManager : public IDisplayManager
    {
    private:
        static M5StackDisplayManager *instance;
        uint8_t current_brightness = 100;

    public:
        M5StackDisplayManager()
        {
            instance = this;
        }

        void setBrightness(uint8_t brightness) override
        {
            current_brightness = brightness;
            M5.Lcd.setBrightness(brightness);
        }

        uint8_t getBrightness() const override
        {
            return current_brightness;
        }

        void turnOn() override
        {
            M5.Lcd.wakeup();
            setBrightness(current_brightness);
        }

        void turnOff() override
        {
            M5.Lcd.sleep();
        }

        uint16_t getWidth() const override
        {
            return 320;
        }

        uint16_t getHeight() const override
        {
            return 240;
        }

        void *getFrameBuffer() override
        {
            return nullptr; // M5Stack doesn't expose framebuffer directly
        }

        void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p) override
        {
            M5.Lcd.pushImage(x1, y1, x2 - x1 + 1, y2 - y1 + 1, (uint16_t *)color_p);
        }

        static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
        {
            if (instance)
            {
                instance->flush(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
            }
            lv_display_flush_ready(disp);
        }
    };

    class M5StackInputManager : public IInputManager
    {
    private:
        bool prev_btn_state[3] = {false, false, false};
        bool curr_btn_state[3] = {false, false, false};

    public:
        bool isButtonPressed(uint8_t button_id) const override
        {
            switch (button_id)
            {
            case 0:
                return M5.BtnA.isPressed();
            case 1:
                return M5.BtnB.isPressed();
            case 2:
                return M5.BtnC.isPressed();
            default:
                return false;
            }
        }

        bool wasButtonPressed(uint8_t button_id) override
        {
            switch (button_id)
            {
            case 0:
                return M5.BtnA.wasPressed();
            case 1:
                return M5.BtnB.wasPressed();
            case 2:
                return M5.BtnC.wasPressed();
            default:
                return false;
            }
        }

        bool wasButtonReleased(uint8_t button_id) override
        {
            switch (button_id)
            {
            case 0:
                return M5.BtnA.wasReleased();
            case 1:
                return M5.BtnB.wasReleased();
            case 2:
                return M5.BtnC.wasReleased();
            default:
                return false;
            }
        }

        bool getTouchPoint(int16_t *x, int16_t *y) override
        {
            // M5Stack Core doesn't have touch by default
            return false;
        }

        bool isTouched() const override
        {
            return false;
        }

        void update() override
        {
            M5.update();
        }
    };

    class M5StackSystemHAL : public ISystemHAL
    {
    private:
        M5StackNetworkManager network_mgr;
        M5StackPowerManager power_mgr;
        M5StackDisplayManager display_mgr;
        M5StackInputManager input_mgr;

        // LVGL display and input objects
        static const size_t buf_size = 320 * 15; // Reduced from 60 to 15 lines
        static lv_color_t buf_1[buf_size];
        lv_display_t *display;
        lv_indev_t *indev;

    public:
        M5StackSystemHAL()
        {
        }

        INetworkManager &getNetworkManager() override { return network_mgr; }
        IPowerManager &getPowerManager() override { return power_mgr; }
        IDisplayManager &getDisplayManager() override { return display_mgr; }
        IInputManager &getInputManager() override { return input_mgr; }

        void init() override
        {
            // Initialize M5Stack
            M5.begin();
            Serial.begin(115200);

            // Explicitly initialize I2C (Wire) to prevent I2C communication errors
            // M5.begin() should do this, but we'll be explicit to ensure it works
            Wire.begin();

            // Initialize LVGL
            lv_init();

            // Create display (LVGL 9.x API)
            display = lv_display_create(320, 240);
            lv_display_set_flush_cb(display, M5StackDisplayManager::lvgl_flush_cb);
            lv_display_set_buffers(display, buf_1, nullptr, buf_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

            // Create input device (LVGL 9.x API)
            indev = lv_indev_create();
            lv_indev_set_type(indev, LV_INDEV_TYPE_BUTTON);
            lv_indev_set_read_cb(indev, input_read_cb);

            Serial.println("M5Stack System initialized");
        }

        void update() override
        {
            input_mgr.update();
            lv_timer_handler();
        }

        uint32_t getMillis() const override
        {
            return millis();
        }

        void delay(uint32_t ms) override
        {
            ::delay(ms);
        }

    private:
        static void input_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data)
        {
            // Simple button to coordinate mapping for demo
            static bool btn_pressed = false;

            if (M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
            {
                data->state = LV_INDEV_STATE_PRESSED;
                if (M5.BtnA.isPressed())
                {
                    data->point.x = 50;
                    data->point.y = 200;
                }
                else if (M5.BtnB.isPressed())
                {
                    data->point.x = 160;
                    data->point.y = 200;
                }
                else if (M5.BtnC.isPressed())
                {
                    data->point.x = 270;
                    data->point.y = 200;
                }
            }
            else
            {
                data->state = LV_INDEV_STATE_RELEASED;
            }
        }
    };

    // Static member definitions
    M5StackDisplayManager *M5StackDisplayManager::instance = nullptr;
    lv_color_t M5StackSystemHAL::buf_1[M5StackSystemHAL::buf_size];

} // namespace hal

#endif // ARDUINO
