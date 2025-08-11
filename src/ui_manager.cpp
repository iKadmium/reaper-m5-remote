#include "ui_manager.h"
#include "log.h"
#include <cstdio>

UIManager::UIManager(hal::ISystemHAL *hal) : system_hal(hal)
{
}

void UIManager::createUI()
{
    // Create main screen
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    // Get screen dimensions
    lv_coord_t screen_width = lv_obj_get_width(scr);
    lv_coord_t screen_height = lv_obj_get_height(scr);

    // Row 1: WiFi status, battery icon, battery percentage (right-aligned)
    wifi_status_label = lv_label_create(scr);
    lv_label_set_text(wifi_status_label, "WiFi");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_RIGHT, -80, 5);

    battery_icon_label = lv_label_create(scr);
    lv_label_set_text(battery_icon_label, "BAT");
    lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(battery_icon_label, LV_ALIGN_TOP_RIGHT, -50, 5);

    battery_percent_label = lv_label_create(scr);
    lv_label_set_text(battery_percent_label, "100%");
    lv_obj_set_style_text_color(battery_percent_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(battery_percent_label, LV_ALIGN_TOP_RIGHT, -5, 5);

    // Row 2: Tab index info (centered)
    tab_info_label = lv_label_create(scr);
    lv_label_set_text(tab_info_label, "[1 of 4]");
    lv_obj_set_style_text_color(tab_info_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(tab_info_label, LV_ALIGN_TOP_MID, 0, 30);

    // Row 3: Play/stop icon and tab name (left-aligned)
    play_icon_label = lv_label_create(scr);
    lv_label_set_text(play_icon_label, "STOP");
    lv_obj_set_style_text_color(play_icon_label, lv_color_hex(0xFF0000), 0);
    lv_obj_align(play_icon_label, LV_ALIGN_TOP_LEFT, 5, 55);

    tab_name_label = lv_label_create(scr);
    lv_label_set_text(tab_name_label, "No Tab Selected");
    lv_obj_set_style_text_color(tab_name_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(tab_name_label, LV_ALIGN_TOP_LEFT, 60, 55);

    // Row 4: Current time / total length (left-aligned)
    time_label = lv_label_create(scr);
    lv_label_set_text(time_label, "0:00 / 0:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FFFF), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 5, 80);

    // Row 4.5: "Are you sure?" message (centered, initially hidden)
    are_you_sure_label = lv_label_create(scr);
    lv_label_set_text(are_you_sure_label, "Are you sure?");
    lv_obj_set_style_text_color(are_you_sure_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(are_you_sure_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    // Row 5: Button functions (aligned over button positions)
    btn1_label = lv_label_create(scr);
    lv_label_set_text(btn1_label, "PREV");
    lv_obj_set_style_text_color(btn1_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(btn1_label, LV_ALIGN_BOTTOM_LEFT, 20, -5);

    btn2_label = lv_label_create(scr);
    lv_label_set_text(btn2_label, "PLAY");
    lv_obj_set_style_text_color(btn2_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(btn2_label, LV_ALIGN_BOTTOM_MID, 0, -5);

    btn3_label = lv_label_create(scr);
    lv_label_set_text(btn3_label, "NEXT");
    lv_obj_set_style_text_color(btn3_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(btn3_label, LV_ALIGN_BOTTOM_RIGHT, -20, -5);
}

void UIManager::updateBatteryUI()
{
    if (!system_hal || !battery_icon_label || !battery_percent_label)
        return;

    auto &power = system_hal->getPowerManager();
    uint8_t battery_percent = power.getBatteryPercentage();
    bool is_charging = power.isCharging();

    // Update battery percentage
    char percent_text[8];
    snprintf(percent_text, sizeof(percent_text), "%d%%", battery_percent);
    lv_label_set_text(battery_percent_label, percent_text);

    // Update battery icon based on level and charging status
    const char *icon_text;
    lv_color_t icon_color;

    if (is_charging)
    {
        icon_text = "CHG";
        icon_color = lv_color_hex(0x00FF00); // Green when charging
    }
    else if (battery_percent > 75)
    {
        icon_text = "FULL";
        icon_color = lv_color_hex(0x00FF00); // Green
    }
    else if (battery_percent > 50)
    {
        icon_text = "MED";
        icon_color = lv_color_hex(0xFFFF00); // Yellow
    }
    else if (battery_percent > 25)
    {
        icon_text = "LOW";
        icon_color = lv_color_hex(0xFF8000); // Orange
    }
    else
    {
        icon_text = "CRIT";
        icon_color = lv_color_hex(0xFF0000); // Red
    }

    lv_label_set_text(battery_icon_label, icon_text);
    lv_obj_set_style_text_color(battery_icon_label, icon_color, 0);
}

void UIManager::updateWiFiUI()
{
    if (!system_hal || !wifi_status_label)
        return;

    auto &network = system_hal->getNetworkManager();
    if (network.isConnected())
    {
        lv_label_set_text(wifi_status_label, "WiFi");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0); // Green
    }
    else
    {
        lv_label_set_text(wifi_status_label, "No WiFi");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0); // Red
    }
}

void UIManager::updateReaperStateUI(const reaper::ReaperState &state)
{
    if (!tab_info_label || !tab_name_label)
        return;

    if (state.success && !state.tabs.empty())
    {
        // Update tab info
        char tab_info[32];
        snprintf(tab_info, sizeof(tab_info), "[%d of %d]",
                 state.active_index + 1, (int)state.tabs.size());
        lv_label_set_text(tab_info_label, tab_info);
        lv_obj_invalidate(tab_info_label);

        // Update tab name
        if (state.active_index < state.tabs.size())
        {
            const auto &active_tab = state.tabs[state.active_index];
            lv_label_set_text(tab_name_label, active_tab.name.c_str());
            lv_obj_invalidate(tab_name_label);

            // Debug output
            static std::string last_tab_name;
            if (last_tab_name != active_tab.name)
            {
                LOG_INFO("UI", "UI Updated: Tab [{} of {}] - {}",
                         state.active_index + 1,
                         state.tabs.size(),
                         active_tab.name.c_str());
                last_tab_name = active_tab.name;
            }
        }
        else
        {
            lv_label_set_text(tab_name_label, "Invalid Tab");
            lv_obj_invalidate(tab_name_label);
        }
    }
    else
    {
        lv_label_set_text(tab_info_label, "[? of ?]");
        lv_label_set_text(tab_name_label, "No Connection");
        lv_obj_invalidate(tab_info_label);
        lv_obj_invalidate(tab_name_label);
    }
}

void UIManager::updateTransportUI(const reaper::TransportState &transport_state, const reaper::ReaperState &reaper_state)
{
    if (!play_icon_label || !time_label)
        return;

    // Update play/stop icon
    const char *icon_text;
    lv_color_t icon_color;

    if (transport_state.success)
    {
        switch (transport_state.play_state)
        {
        case 0: // Stopped
            icon_text = "STOP";
            icon_color = lv_color_hex(0xFF0000); // Red
            break;
        case 1: // Playing
            icon_text = "PLAY";
            icon_color = lv_color_hex(0x00FF00); // Green
            break;
        case 2: // Paused
            icon_text = "PAUSE";
            icon_color = lv_color_hex(0xFFFF00); // Yellow
            break;
        case 5: // Recording
            icon_text = "REC";
            icon_color = lv_color_hex(0xFF0000); // Red
            break;
        default:
            icon_text = "UNK";
            icon_color = lv_color_hex(0xFFFFFF); // White
            break;
        }
    }
    else
    {
        icon_text = "---";
        icon_color = lv_color_hex(0x808080); // Gray
    }

    lv_label_set_text(play_icon_label, icon_text);
    lv_obj_set_style_text_color(play_icon_label, icon_color, 0);
    lv_obj_invalidate(play_icon_label);

    // Update time display
    char time_text[32];
    if (transport_state.success && reaper_state.success &&
        reaper_state.active_index < reaper_state.tabs.size())
    {
        double current_pos = transport_state.position_seconds;
        double total_length = reaper_state.tabs[reaper_state.active_index].length;

        int current_min = (int)(current_pos / 60);
        int current_sec = (int)(current_pos) % 60;
        int total_min = (int)(total_length / 60);
        int total_sec = (int)(total_length) % 60;

        snprintf(time_text, sizeof(time_text), "%d:%02d / %d:%02d",
                 current_min, current_sec, total_min, total_sec);
    }
    else
    {
        snprintf(time_text, sizeof(time_text), "0:00 / 0:00");
    }

    lv_label_set_text(time_label, time_text);
    lv_obj_invalidate(time_label);
}

void UIManager::updateButtonLabelsUI()
{
    if (!btn1_label || !btn2_label || !btn3_label || !are_you_sure_label)
        return;

    static UIState last_ui_state = UIState::STOPPED;
    if (last_ui_state != current_ui_state)
    {
        LOG_INFO("UI", "Button labels updating for state: {}",
                 current_ui_state == UIState::STOPPED ? "STOPPED" : current_ui_state == UIState::PLAYING ? "PLAYING"
                                                                                                         : "ARE_YOU_SURE");
        last_ui_state = current_ui_state;
    }

    switch (current_ui_state)
    {
    case UIState::STOPPED:
        lv_label_set_text(btn1_label, "PREV");
        lv_label_set_text(btn2_label, "PLAY");
        lv_label_set_text(btn3_label, "NEXT");
        lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Hide "Are you sure?"
        break;
    case UIState::PLAYING:
        lv_label_set_text(btn1_label, "---");
        lv_label_set_text(btn2_label, "STOP?");
        lv_label_set_text(btn3_label, "---");
        lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Hide "Are you sure?"
        break;
    case UIState::ARE_YOU_SURE:
        lv_label_set_text(btn1_label, "STOP");
        lv_label_set_text(btn2_label, "CANCEL");
        lv_label_set_text(btn3_label, "CANCEL");
        lv_obj_clear_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Show "Are you sure?"
        break;
    }

    // Force redraw of all button labels and the "Are you sure?" label
    lv_obj_invalidate(btn1_label);
    lv_obj_invalidate(btn2_label);
    lv_obj_invalidate(btn3_label);
    lv_obj_invalidate(are_you_sure_label);
}

void UIManager::updatePeriodicUI(unsigned long current_time)
{
    // Update battery UI every 30 seconds
    if (current_time - last_battery_update >= 30000)
    {
        updateBatteryUI();
        updateWiFiUI();
        last_battery_update = current_time;
    }
}
