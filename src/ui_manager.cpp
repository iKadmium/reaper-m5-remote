#include "ui_manager.h"
#include "log.h"
#include <cstdio>

UIManager::UIManager(hal::ISystemHAL *hal) : system_hal(hal), wifi_connected(false), reaper_connected(false)
{
}

void UIManager::createUI()
{
    setupMainScreen();

    lv_obj_t *scr = lv_scr_act();
    if (!scr)
    {
        LOG_ERROR("UIManager", "No screen available after setupMainScreen!");
        return;
    }

    createGlobalContainer(scr);

    createStatusContainer(global_container);
    createConnectionStatusLabel(global_container);
    createMainUIContainer(global_container);

    createTabInfoSection(main_ui_container);
    createTransportSection(main_ui_container);
    createButtonSection(main_ui_container);
    // Start with connection status showing
    showConnectionStatus("Connecting to WiFi...");
}

void UIManager::updateBatteryUI()
{
    if (!system_hal || !battery_icon_label)
        return;

    auto &power = system_hal->getPowerManager();
    uint8_t battery_percent = power.getBatteryPercentage();
    bool is_charging = power.isCharging();

    // Update battery icon based on level and charging status
    const char *icon_text;
    lv_color_t icon_color;

    if (is_charging)
    {
        icon_text = LV_SYMBOL_CHARGE;
        icon_color = lv_color_hex(0x00FF00); // Green when charging
    }
    else if (battery_percent > 80)
    {
        icon_text = LV_SYMBOL_BATTERY_FULL;
        icon_color = lv_color_hex(0x00FF00); // Green
    }
    else if (battery_percent > 60)
    {
        icon_text = LV_SYMBOL_BATTERY_3;
        icon_color = lv_color_hex(0x00FF00); // Green
    }
    else if (battery_percent > 40)
    {
        icon_text = LV_SYMBOL_BATTERY_2;
        icon_color = lv_color_hex(0xFFFF00); // Yellow
    }
    else if (battery_percent > 20)
    {
        icon_text = LV_SYMBOL_BATTERY_1;
        icon_color = lv_color_hex(0xFF8000); // Orange
    }
    else
    {
        icon_text = LV_SYMBOL_BATTERY_EMPTY;
        icon_color = lv_color_hex(0xFF0000); // Red
    }

    auto battery_percent_text = std::to_string(battery_percent) + "%";
    lv_label_set_text(battery_percentage_label, battery_percent_text.c_str());
    lv_obj_set_style_text_color(battery_percentage_label, icon_color, 0);

    lv_label_set_text(battery_icon_label, icon_text);
    lv_obj_set_style_text_color(battery_icon_label, icon_color, 0);
}

void UIManager::updateWiFiUI()
{
    if (!system_hal || !wifi_status_label)
        return;

    auto &network = system_hal->getNetworkManager();
    updateWiFiUI(network.isConnected());
}

void UIManager::updateWiFiUI(const bool connected)
{
    if (!system_hal || !wifi_status_label)
        return;

    LOG_INFO("WIFI", "Connected %d", connected);
    auto color = connected ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED);
    lv_obj_set_style_text_color(wifi_status_label, color, 0);
    lv_obj_invalidate(wifi_status_label);

    // Track WiFi connection state and update UI
    wifi_connected = connected;
    updateConnectionState(wifi_connected, reaper_connected);
}

void UIManager::updateReaperStateUI(const reaper::ReaperState &state)
{
    if (!tab_info_label || !tab_name_label)
        return;

    bool reaper_is_connected = state.success;
    auto status_color = reaper_is_connected ? lv_palette_main(LV_PALETTE_GREEN) : lv_palette_main(LV_PALETTE_RED);
    lv_obj_set_style_text_color(reaper_status_label, status_color, 0);
    lv_obj_invalidate(reaper_status_label);

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

    // Track Reaper connection state and update UI
    reaper_connected = reaper_is_connected;
    updateConnectionState(wifi_connected, reaper_connected);
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
            icon_text = LV_SYMBOL_STOP;
            icon_color = lv_color_hex(0xFF0000); // Red
            break;
        case 1: // Playing
            icon_text = LV_SYMBOL_PLAY;
            icon_color = lv_color_hex(0x00FF00); // Green
            break;
        case 2: // Paused
            icon_text = LV_SYMBOL_PAUSE;
            icon_color = lv_color_hex(0xFFFF00); // Yellow
            break;
        case 5: // Recording
            icon_text = "REC";
            icon_color = lv_color_hex(0xFF0000); // Red
            break;
        default:
            icon_text = LV_SYMBOL_WARNING;
            icon_color = lv_color_hex(0xFFFFFF); // White
            break;
        }
    }
    else
    {
        icon_text = "";
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
    case UIState::DISCONNECTED:
        lv_label_set_text(btn1_label, LV_SYMBOL_CLOSE);
        lv_label_set_text(btn2_label, LV_SYMBOL_CLOSE);
        lv_label_set_text(btn3_label, LV_SYMBOL_CLOSE);
        lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Hide "Are you sure?"
        break;
    case UIState::STOPPED:
        lv_label_set_text(btn1_label, LV_SYMBOL_PREV);
        lv_label_set_text(btn2_label, LV_SYMBOL_PLAY);
        lv_label_set_text(btn3_label, LV_SYMBOL_NEXT);
        lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Hide "Are you sure?"
        break;
    case UIState::PLAYING:
        lv_label_set_text(btn1_label, "");
        lv_label_set_text(btn2_label, LV_SYMBOL_STOP);
        lv_label_set_text(btn3_label, "");
        lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Hide "Are you sure?"
        break;
    case UIState::ARE_YOU_SURE:
        lv_label_set_text(btn1_label, LV_SYMBOL_OK);
        lv_label_set_text(btn2_label, LV_SYMBOL_CLOSE);
        lv_label_set_text(btn3_label, LV_SYMBOL_CLOSE);
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
        updateWiFiUI();
        updateBatteryUI();
        last_battery_update = current_time;
    }
}

void UIManager::updateConnectionState(bool wifi_connected, bool reaper_connected)
{
    if (!wifi_connected)
    {
        showConnectionStatus("Connecting to WiFi...");
    }
    else if (!reaper_connected)
    {
        showConnectionStatus("Connecting to Reaper...");
    }
    else
    {
        showMainUI();
    }
}

void UIManager::showConnectionStatus(const std::string &message)
{
    if (!connection_status_label || !main_ui_container)
        return;

    // Hide main UI
    lv_obj_add_flag(main_ui_container, LV_OBJ_FLAG_HIDDEN);

    // Show connection status message
    lv_label_set_text(connection_status_label, message.c_str());
    lv_obj_clear_flag(connection_status_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(connection_status_label);
}

void UIManager::showMainUI()
{
    if (!connection_status_label || !main_ui_container)
        return;

    // Hide connection status message
    lv_obj_add_flag(connection_status_label, LV_OBJ_FLAG_HIDDEN);

    // Show main UI
    lv_obj_clear_flag(main_ui_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(main_ui_container);
}

// Private UI creation helper methods
void UIManager::setupMainScreen()
{
    auto scr = lv_scr_act();
    if (scr)
    {
        lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    }
}

void UIManager::createGlobalContainer(lv_obj_t *parent)
{
    // Create global container (always visible - contains all other elements)
    global_container = lv_obj_create(parent);
    lv_obj_set_size(global_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(global_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(global_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(global_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(global_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(global_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(global_container, 0, 0);
    lv_obj_set_style_pad_gap(global_container, 10, 0);
}

void UIManager::createStatusContainer(lv_obj_t *parent)
{
    // Create status container (always visible - contains WiFi/Reaper/Battery status)
    status_container = lv_obj_create(parent);
    lv_obj_set_size(status_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(status_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_container, 0, 0);
    lv_obj_set_style_pad_gap(status_container, 10, 0);

    // Create status labels
    reaper_status_label = lv_label_create(status_container);
    lv_label_set_text(reaper_status_label, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(reaper_status_label, lv_color_hex(0xFF0000), 0);

    wifi_status_label = lv_label_create(status_container);
    lv_label_set_text(wifi_status_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);

    battery_icon_label = lv_label_create(status_container);
    lv_label_set_text(battery_icon_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(battery_icon_label, lv_color_hex(0xFFFFFF), 0);

    battery_percentage_label = lv_label_create(status_container);
    lv_label_set_text(battery_percentage_label, "??");
    lv_obj_set_style_text_color(battery_percentage_label, lv_color_hex(0xFFFFFF), 0);
}

void UIManager::createConnectionStatusLabel(lv_obj_t *parent)
{
    // Create connection status label (shown when WiFi/Reaper not connected)
    connection_status_label = lv_label_create(parent);
    lv_label_set_text(connection_status_label, "Connecting...");
    lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_align(connection_status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_add_flag(connection_status_label, LV_OBJ_FLAG_HIDDEN); // Initially hidden
}

void UIManager::createMainUIContainer(lv_obj_t *parent)
{
    // Create main UI container (hidden when not connected)
    main_ui_container = lv_obj_create(parent);
    lv_obj_set_size(main_ui_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(main_ui_container, 1); // This will make it grow to fill available space
    lv_obj_set_layout(main_ui_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_ui_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_ui_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(main_ui_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(main_ui_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(main_ui_container, 0, 0);
}

void UIManager::createTabInfoSection(lv_obj_t *parent)
{
    // Tab index info (inside main UI container)
    tab_info_label = lv_label_create(parent);
    lv_label_set_text(tab_info_label, "[x of x]");
    lv_obj_set_style_text_color(tab_info_label, lv_color_hex(0xFFFF00), 0);
}

void UIManager::createTransportSection(lv_obj_t *parent)
{
    // Play status container (Play icon + Tab name)
    lv_obj_t *play_row = lv_obj_create(parent);
    lv_obj_set_size(play_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(play_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(play_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(play_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(play_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(play_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(play_row, 0, 0);
    lv_obj_set_style_pad_gap(play_row, 10, 0);

    play_icon_label = lv_label_create(play_row);
    lv_label_set_text(play_icon_label, LV_SYMBOL_STOP);
    lv_obj_set_style_text_color(play_icon_label, lv_color_hex(0xFF0000), 0);

    tab_name_label = lv_label_create(play_row);
    lv_label_set_text(tab_name_label, "No Tab Selected");
    lv_obj_set_style_text_color(tab_name_label, lv_color_hex(0xFFFFFF), 0);

    time_label = lv_label_create(parent);
    lv_label_set_text(time_label, "0:00 / 0:00");
    lv_obj_set_style_text_color(time_label, lv_color_hex(0x00FFFF), 0);

    printf("createTransportSection - Creating are_you_sure_label\n");
    // "Are you sure?" message (centered, initially hidden)
    are_you_sure_label = lv_label_create(parent);
    lv_label_set_text(are_you_sure_label, "Are you sure?");
    lv_obj_set_style_text_color(are_you_sure_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_add_flag(are_you_sure_label, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    printf("createTransportSection - Completed\n");
}

void UIManager::createButtonSection(lv_obj_t *parent)
{
    // Button functions container
    lv_obj_t *button_row = lv_obj_create(parent);
    lv_obj_set_size(button_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(button_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(button_row, 0, 0);

    btn1_label = lv_label_create(button_row);
    lv_label_set_text(btn1_label, "");
    lv_obj_set_style_text_color(btn1_label, lv_color_hex(0xFFFFFF), 0);

    btn2_label = lv_label_create(button_row);
    lv_label_set_text(btn2_label, "");
    lv_obj_set_style_text_color(btn2_label, lv_color_hex(0xFFFFFF), 0);

    btn3_label = lv_label_create(button_row);
    lv_label_set_text(btn3_label, "");
    lv_obj_set_style_text_color(btn3_label, lv_color_hex(0xFFFFFF), 0);
}
