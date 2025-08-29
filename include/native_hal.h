#pragma once

#ifdef NATIVE_BUILD

#include "hal_interfaces.h"
#include <SDL2/SDL.h>
#include <lvgl.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <curl/curl.h>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "log.h"

namespace hal
{

    struct HttpResponse
    {
        std::string data;
        long response_code;
    };

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, HttpResponse *response)
    {
        size_t total_size = size * nmemb;
        response->data.append((char *)contents, total_size);
        return total_size;
    }

    class NativeNetworkManager : public INetworkManager
    {
    private:
        bool connected = false;
        std::string ip_address = "127.0.0.1";

    public:
        NativeNetworkManager()
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);
        }

        ~NativeNetworkManager()
        {
            curl_global_cleanup();
        }

        bool connect(const char *ssid, const char *password) override
        {
            // Simulate connection
            printf("Simulating WiFi connection to %s...\n", ssid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            connected = true;
            printf("Connected! IP: %s\n", ip_address.c_str());
            return true;
            // connected = false;
            // return false;
        }

        bool disconnect() override
        {
            connected = false;
            printf("WiFi disconnected\n");
            return true;
        }

        bool isConnected() const override
        {
            return connected;
        }

        const char *getIP() const override
        {
            return ip_address.c_str();
        }

        bool httpGetBlocking(const char *url, std::string &response, int &status_code) override
        {
            CURL *curl = curl_easy_init();
            if (!curl)
            {
                status_code = 0;
                response.clear();
                return false;
            }

            HttpResponse curl_response;
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

            // Force fresh connections to prevent connection pooling issues
            curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
            curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1L);

            // Disable keep-alive to ensure connections are properly closed
            struct curl_slist *headers = nullptr;
            headers = curl_slist_append(headers, "Connection: close");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Add user agent to identify requests
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Reaper-M5-Remote/1.0");

            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK)
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &curl_response.response_code);
                response = curl_response.data;
                status_code = curl_response.response_code;
            }
            else
            {
                response.clear();
                status_code = 0;
            }

            // Clean up headers and curl handle
            if (headers)
            {
                curl_slist_free_all(headers);
            }
            curl_easy_cleanup(curl);
            return res == CURLE_OK && status_code > 0;
        }
    };

    class NativePowerManager : public IPowerManager
    {
    public:
        float getBatteryVoltage() const override
        {
            return 3.7f; // Simulate battery voltage
        }

        uint8_t getBatteryPercentage() const override
        {
            return 85; // Simulate 85% battery
        }

        bool isCharging() const override
        {
            return false; // Simulate not charging
        }

        void deepSleep(uint32_t seconds) override
        {
            printf("Deep sleep for %u seconds (simulated)\n", seconds);
            std::this_thread::sleep_for(std::chrono::seconds(seconds));
        }

        void lightSleep(uint32_t milliseconds) override
        {
            printf("Light sleep for %u ms (simulated)\n", milliseconds);
            std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
        }

        void powerOff() override
        {
            printf("System power off (simulated)\n");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        void restart() override
        {
            printf("System restart (simulated)\n");
            exit(0);
        }

        void setCpuFrequency(uint32_t mhz) override
        {
            printf("Setting CPU frequency to %u MHz (simulated)\n", mhz);
        }

        void enableWiFiPowerSave(bool enable) override
        {
            printf("WiFi power save %s (simulated)\n", enable ? "enabled" : "disabled");
        }
    };

    class NativeDisplayManager : public IDisplayManager
    {
    private:
        static NativeDisplayManager *instance;
        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;
        SDL_Texture *texture = nullptr;
        uint8_t brightness = 100;
        bool display_on = true;

        static const int SCREEN_WIDTH = 320;
        static const int SCREEN_HEIGHT = 240;
        static const int SCALE_FACTOR = 2;

    public:
        NativeDisplayManager()
        {
            instance = this;
        }

        ~NativeDisplayManager()
        {
            if (texture)
                SDL_DestroyTexture(texture);
            if (renderer)
                SDL_DestroyRenderer(renderer);
            if (window)
                SDL_DestroyWindow(window);
        }

        bool initSDL()
        {
            if (SDL_Init(SDL_INIT_VIDEO) < 0)
            {
                printf("SDL init failed: %s\n", SDL_GetError());
                return false;
            }

            window = SDL_CreateWindow("M5Stack Simulator",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SCREEN_WIDTH * SCALE_FACTOR,
                                      SCREEN_HEIGHT * SCALE_FACTOR,
                                      SDL_WINDOW_SHOWN);

            if (!window)
            {
                printf("Window creation failed: %s\n", SDL_GetError());
                return false;
            }

            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (!renderer)
            {
                printf("Renderer creation failed: %s\n", SDL_GetError());
                return false;
            }

            texture = SDL_CreateTexture(renderer,
                                        SDL_PIXELFORMAT_RGB565,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        SCREEN_WIDTH,
                                        SCREEN_HEIGHT);

            if (!texture)
            {
                printf("Texture creation failed: %s\n", SDL_GetError());
                return false;
            }

            return true;
        }

        void setBrightness(uint8_t brightness) override
        {
            this->brightness = brightness;
            if (texture)
            {
                SDL_SetTextureAlphaMod(texture, (brightness * 255) / 100);
            }
        }

        uint8_t getBrightness() const override
        {
            return brightness;
        }

        void turnOn() override
        {
            display_on = true;
        }

        void turnOff() override
        {
            display_on = false;
            if (renderer)
            {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                SDL_RenderPresent(renderer);
            }
        }

        uint16_t getWidth() const override
        {
            return SCREEN_WIDTH;
        }

        uint16_t getHeight() const override
        {
            return SCREEN_HEIGHT;
        }

        void *getFrameBuffer() override
        {
            return nullptr;
        }

        void flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const uint16_t *color_p) override
        {
            if (!display_on || !texture || !renderer)
            {
                return;
            }

            void *pixels;
            int pitch;

            if (SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0)
            {
                uint16_t *pixel_buffer = (uint16_t *)pixels;

                for (int y = y1; y <= y2; y++)
                {
                    for (int x = x1; x <= x2; x++)
                    {
                        int src_index = (y - y1) * (x2 - x1 + 1) + (x - x1);
                        int dst_index = y * SCREEN_WIDTH + x;
                        pixel_buffer[dst_index] = color_p[src_index];
                    }
                }

                SDL_UnlockTexture(texture);
            }

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }

        void processSDLEvents()
        {
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    exit(0);
                }
            }
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

    class NativeInputManager : public IInputManager
    {
    private:
        bool button_states[3] = {false, false, false};
        bool prev_button_states[3] = {false, false, false};

    public:
        bool isButtonPressed(uint8_t button_id) const override
        {
            if (button_id < 3)
            {
                return button_states[button_id];
            }
            return false;
        }

        bool wasButtonPressed(uint8_t button_id) override
        {
            if (button_id < 3)
            {
                bool pressed = button_states[button_id] && !prev_button_states[button_id];
                return pressed;
            }
            return false;
        }

        bool wasButtonReleased(uint8_t button_id) override
        {
            if (button_id < 3)
            {
                bool released = !button_states[button_id] && prev_button_states[button_id];
                return released;
            }
            return false;
        }

        bool getTouchPoint(int16_t *x, int16_t *y) override
        {
            int mouse_x, mouse_y;
            uint32_t mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);

            if (mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT))
            {
                *x = mouse_x / 2; // Scale down by SCALE_FACTOR
                *y = mouse_y / 2;
                return true;
            }
            return false;
        }

        bool isTouched() const override
        {
            uint32_t mouse_state = SDL_GetMouseState(nullptr, nullptr);
            return mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT);
        }

        void update() override
        {
            // Copy current to previous
            memcpy(prev_button_states, button_states, sizeof(button_states));

            // Update from keyboard
            const uint8_t *keyboard_state = SDL_GetKeyboardState(nullptr);
            button_states[0] = keyboard_state[SDL_SCANCODE_A] || keyboard_state[SDL_SCANCODE_1];
            button_states[1] = keyboard_state[SDL_SCANCODE_B] || keyboard_state[SDL_SCANCODE_2];
            button_states[2] = keyboard_state[SDL_SCANCODE_C] || keyboard_state[SDL_SCANCODE_3];
        }
    };

    class NativeSystemHAL : public ISystemHAL
    {
    private:
        NativeNetworkManager network_mgr;
        NativePowerManager power_mgr;
        NativeDisplayManager display_mgr;
        NativeInputManager input_mgr;

        std::chrono::steady_clock::time_point start_time;

        // LVGL display and input objects
        static const size_t buf_size = 320 * 60;
        static lv_color_t buf_1[buf_size];
        static lv_color_t buf_2[buf_size];
        lv_display_t *display;
        lv_indev_t *indev;

    public:
        INetworkManager &getNetworkManager() override { return network_mgr; }
        IPowerManager &getPowerManager() override { return power_mgr; }
        IDisplayManager &getDisplayManager() override { return display_mgr; }
        IInputManager &getInputManager() override { return input_mgr; }

        void init() override
        {
            start_time = std::chrono::steady_clock::now();

            // Initialize logging with colored log4j-style format
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] %@ %! - %v");

            auto logger = std::make_shared<spdlog::logger>("default", console_sink);
            logger->set_level(spdlog::level::debug);
            spdlog::set_default_logger(logger);
            spdlog::set_level(spdlog::level::debug);

            // Initialize SDL and display
            if (!display_mgr.initSDL())
            {
                LOG_ERROR("Native", "Failed to initialize SDL");
                exit(1);
            }

            // Initialize LVGL
            lv_init();

            // Create display (LVGL 9.x API)
            display = lv_display_create(320, 240);
            lv_display_set_flush_cb(display, NativeDisplayManager::lvgl_flush_cb);
            lv_display_set_buffers(display, buf_1, buf_2, buf_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

            // Create input device (LVGL 9.x API)
            indev = lv_indev_create();
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(indev, input_read_cb);
            lv_indev_set_user_data(indev, this);

            LOG_INFO("Native", "System initialized");
            LOG_INFO("Native", "Use keys A/1, B/2, C/3 for buttons or click with mouse");
        }

        void update() override
        {
            display_mgr.processSDLEvents();
            input_mgr.update();
            lv_timer_handler();
        }

        uint32_t getMillis() const override
        {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
            return duration.count();
        }

        void delay(uint32_t ms) override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }

    private:
        static void input_read_cb(lv_indev_t *indev_drv, lv_indev_data_t *data)
        {
            NativeSystemHAL *hal = (NativeSystemHAL *)lv_indev_get_user_data(indev_drv);
            int16_t x, y;
            bool touched = hal->input_mgr.getTouchPoint(&x, &y);

            if (touched)
            {
                data->state = LV_INDEV_STATE_PRESSED;
                data->point.x = x;
                data->point.y = y;
            }
            else
            {
                data->state = LV_INDEV_STATE_RELEASED;
            }
        }
    };

} // namespace hal

#endif // NATIVE_BUILD
