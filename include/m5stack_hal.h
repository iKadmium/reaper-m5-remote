#pragma once

#ifdef ARDUINO

#include "hal_interfaces.h"
#include <M5Stack.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <lvgl.h>

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
            return connected && WiFi.status() == WL_CONNECTED;
        }

        const char *getIP() const override
        {
            return ip_address.c_str();
        }

        bool httpGet(const char *url, std::function<void(const char *response, int status_code)> callback) override
        {
            http.begin(url);
            int httpCode = http.GET();

            if (httpCode > 0)
            {
                String payload = http.getString();
                callback(payload.c_str(), httpCode);
            }
            else
            {
                callback(nullptr, httpCode);
            }

            http.end();
            return httpCode > 0;
        }

        bool httpPost(const char *url, const char *data, std::function<void(const char *response, int status_code)> callback) override
        {
            http.begin(url);
            http.addHeader("Content-Type", "application/json");
            int httpCode = http.POST(data);

            if (httpCode > 0)
            {
                String payload = http.getString();
                callback(payload.c_str(), httpCode);
            }
            else
            {
                callback(nullptr, httpCode);
            }

            http.end();
            return httpCode > 0;
        }
    };

    class M5StackPowerManager : public IPowerManager
    {
    public:
        float getBatteryVoltage() const override
        {
            // Note: M5Stack Power API varies by version, using getBatteryLevel as fallback
            return M5.Power.getBatteryLevel(); // This typically returns voltage in V
        }

        uint8_t getBatteryPercentage() const override
        {
            float voltage = getBatteryVoltage();
            // Simple voltage to percentage conversion (adjust for your battery)
            float percentage = ((voltage - 3.0) / (4.2 - 3.0)) * 100.0;
            return constrain(percentage, 0, 100);
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

        static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
        {
            if (instance)
            {
                instance->flush(area->x1, area->y1, area->x2, area->y2, (uint16_t *)color_p);
            }
            lv_disp_flush_ready(disp);
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

        // LVGL display buffer
        static const size_t buf_size = 320 * 60;
        static lv_color_t buf_1[buf_size];
        static lv_color_t buf_2[buf_size];
        lv_disp_draw_buf_t draw_buf;
        lv_disp_drv_t disp_drv;
        lv_indev_drv_t indev_drv;

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

            // Initialize LVGL
            lv_init();

            // Initialize display buffer
            lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, buf_size);

            // Initialize display driver
            lv_disp_drv_init(&disp_drv);
            disp_drv.hor_res = 320;
            disp_drv.ver_res = 240;
            disp_drv.flush_cb = M5StackDisplayManager::lvgl_flush_cb;
            disp_drv.draw_buf = &draw_buf;
            lv_disp_drv_register(&disp_drv);

            // Initialize input driver (buttons)
            lv_indev_drv_init(&indev_drv);
            indev_drv.type = LV_INDEV_TYPE_BUTTON;
            indev_drv.read_cb = input_read_cb;
            lv_indev_drv_register(&indev_drv);

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
        static void input_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
        {
            // Simple button to coordinate mapping for demo
            static bool btn_pressed = false;

            if (M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
            {
                data->state = LV_INDEV_STATE_PR;
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
                data->state = LV_INDEV_STATE_REL;
            }
        }
    };

    // Static member definitions
    M5StackDisplayManager *M5StackDisplayManager::instance = nullptr;
    lv_color_t M5StackSystemHAL::buf_1[M5StackSystemHAL::buf_size];
    lv_color_t M5StackSystemHAL::buf_2[M5StackSystemHAL::buf_size];

} // namespace hal

#endif // ARDUINO
