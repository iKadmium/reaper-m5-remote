#ifdef NATIVE_BUILD

#include "native_hal.h"

namespace hal
{
    // Static member definitions
    NativeDisplayManager *NativeDisplayManager::instance = nullptr;
    lv_color_t NativeSystemHAL::buf_1[NativeSystemHAL::buf_size];
    lv_color_t NativeSystemHAL::buf_2[NativeSystemHAL::buf_size];

    void NativeSystemHAL::init()
    {
        printf("Starting NativeSystemHAL::init()\n");
        start_time = std::chrono::steady_clock::now();

        printf("Initializing logging...\n");
        // Initialize logging with colored log4j-style format
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%^%l%$] [%Y-%m-%d %H:%M:%S.%e] %@ %! - %v");

        auto logger = std::make_shared<spdlog::logger>("default", console_sink);
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
        printf("Logging initialized\n");

        printf("Initializing SDL display manager...\n");
        // Initialize SDL first
        if (!display_mgr.initSDL())
        {
            LOG_ERROR("Native", "Failed to initialize SDL");
            exit(1);
        }
        printf("SDL display manager initialized\n");

        printf("Initializing LVGL...\n");
        // Initialize LVGL
        lv_init();
        printf("LVGL initialized\n");

        printf("Creating LVGL display...\n");
        // Create display with single buffer to save memory
        display = lv_display_create(320, 240);
        lv_display_set_flush_cb(display, NativeDisplayManager::lvgl_flush_cb);
        lv_display_set_buffers(display, buf_1, nullptr, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
        printf("LVGL display created\n");

        printf("Creating input device...\n");
        // Create input device
        indev = lv_indev_create();
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, input_read_cb);
        lv_indev_set_user_data(indev, this);
        printf("Input device created\n");

        LOG_INFO("Native", "System initialized");
        LOG_INFO("Native", "Use keys A/1, B/2, C/3 for buttons or click with mouse");
        printf("NativeSystemHAL::init() completed\n");
    }
}

#endif // NATIVE_BUILD
