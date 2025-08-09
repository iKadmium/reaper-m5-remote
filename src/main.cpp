#include <lvgl.h>

#ifdef ARDUINO
#include "m5stack_hal.h"
using SystemHAL = hal::M5StackSystemHAL;
#else
#include "native_hal.h"
using SystemHAL = hal::NativeSystemHAL;
#endif

// Global system HAL instance
SystemHAL *g_system = nullptr;

// LVGL UI elements
lv_obj_t *label_status;
lv_obj_t *label_battery;
lv_obj_t *label_network;
lv_obj_t *btn_a;
lv_obj_t *btn_b;
lv_obj_t *btn_c;

// Button event handler
static void btn_event_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);

    if (btn == btn_a)
    {
        g_system->getLogger().log(hal::ILogger::INFO, "UI", "Button A pressed");
        lv_label_set_text(label_status, "Button A pressed!");

        // Example network call
        g_system->getNetworkManager().httpGet("https://httpbin.org/get",
                                              [](const char *response, int status_code)
                                              {
                                                  if (response)
                                                  {
                                                      g_system->getLogger().logf(hal::ILogger::INFO, "HTTP", "GET response (%d): %.100s...", status_code, response);
                                                  }
                                              });
    }
    else if (btn == btn_b)
    {
        g_system->getLogger().log(hal::ILogger::INFO, "UI", "Button B pressed");
        lv_label_set_text(label_status, "Button B pressed!");

        // Toggle display brightness
        auto &display = g_system->getDisplayManager();
        uint8_t brightness = display.getBrightness();
        brightness = (brightness > 50) ? 25 : 100;
        display.setBrightness(brightness);

        g_system->getLogger().logf(hal::ILogger::INFO, "Display", "Brightness set to %d%%", brightness);
    }
    else if (btn == btn_c)
    {
        g_system->getLogger().log(hal::ILogger::INFO, "UI", "Button C pressed");
        lv_label_set_text(label_status, "Button C pressed!");

        // Example: Enable power saving
        g_system->getPowerManager().enableWiFiPowerSave(true);
        g_system->getPowerManager().setCpuFrequency(80); // Lower frequency
    }
}

void create_ui()
{
    // Create main screen
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    // Title label
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "M5Stack LVGL Demo");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // Status label
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Ready");
    lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 40);

    // Battery status
    label_battery = lv_label_create(scr);
    lv_obj_set_style_text_color(label_battery, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(label_battery, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Network status
    label_network = lv_label_create(scr);
    lv_obj_set_style_text_color(label_network, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(label_network, LV_ALIGN_TOP_LEFT, 10, 10);

    // Create buttons
    btn_a = lv_btn_create(scr);
    lv_obj_set_size(btn_a, 80, 40);
    lv_obj_align(btn_a, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(btn_a, btn_event_handler, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *label_a = lv_label_create(btn_a);
    lv_label_set_text(label_a, "HTTP");
    lv_obj_center(label_a);

    btn_b = lv_btn_create(scr);
    lv_obj_set_size(btn_b, 80, 40);
    lv_obj_align(btn_b, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn_b, btn_event_handler, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *label_b = lv_label_create(btn_b);
    lv_label_set_text(label_b, "BRIGHT");
    lv_obj_center(label_b);

    btn_c = lv_btn_create(scr);
    lv_obj_set_size(btn_c, 80, 40);
    lv_obj_align(btn_c, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(btn_c, btn_event_handler, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *label_c = lv_label_create(btn_c);
    lv_label_set_text(label_c, "POWER");
    lv_obj_center(label_c);

    // Create a simple meter in the center
    lv_obj_t *meter = lv_meter_create(scr);
    lv_obj_set_size(meter, 120, 120);
    lv_obj_align(meter, LV_ALIGN_CENTER, 0, -10);

    // Add scale and indicator
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 11, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_major_ticks(meter, scale, 2, 4, 15, lv_color_black(), 10);
    lv_meter_set_scale_range(meter, scale, 0, 100, 270, 135);

    lv_meter_indicator_t *indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_RED), -10);
    lv_meter_set_indicator_value(meter, indic, 85); // Show 85% battery
}

void update_status_labels()
{
    static uint32_t last_update = 0;
    uint32_t now = g_system->getMillis();

    if (now - last_update > 1000)
    { // Update every second
        // Update battery status
        auto &power = g_system->getPowerManager();
        float voltage = power.getBatteryVoltage();
        uint8_t percentage = power.getBatteryPercentage();

        char battery_text[32];
        snprintf(battery_text, sizeof(battery_text), "BAT: %d%% %.1fV", percentage, voltage);
        lv_label_set_text(label_battery, battery_text);

        // Update network status
        auto &network = g_system->getNetworkManager();
        if (network.isConnected())
        {
            char network_text[32];
            snprintf(network_text, sizeof(network_text), "WiFi: %s", network.getIP());
            lv_label_set_text(label_network, network_text);
        }
        else
        {
            lv_label_set_text(label_network, "WiFi: OFF");
        }

        last_update = now;
    }
}

#ifdef ARDUINO
void setup()
#else
int main()
#endif
{
    // Create and initialize system HAL
    g_system = new SystemHAL();
    g_system->init();

    g_system->getLogger().log(hal::ILogger::INFO, "Main", "Application starting...");

    // Connect to WiFi (example)
    g_system->getNetworkManager().connect("YourWiFiSSID", "YourPassword");

    // Create the UI
    create_ui();

    g_system->getLogger().log(hal::ILogger::INFO, "Main", "UI created, entering main loop");

#ifdef ARDUINO
}

void loop()
{
#else
    while (true)
    {
#endif
    // Update system
    g_system->update();

    // Update status displays
    update_status_labels();

    // Small delay to prevent overwhelming the system
    g_system->delay(10);

#ifndef ARDUINO
}

delete g_system;
return 0;
#endif
}
