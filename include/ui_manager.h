#pragma once

#include <lvgl.h>
#include <string>
#include "hal_interfaces.h"
#include "reaper_types.h"

enum class UIState
{
    STOPPED,
    PLAYING,
    ARE_YOU_SURE
};

class UIManager
{
private:
    // UI elements
    lv_obj_t *wifi_status_label = nullptr;
    lv_obj_t *battery_icon_label = nullptr;
    lv_obj_t *tab_info_label = nullptr;
    lv_obj_t *play_icon_label = nullptr;
    lv_obj_t *tab_name_label = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *are_you_sure_label = nullptr;
    lv_obj_t *btn1_label = nullptr;
    lv_obj_t *btn2_label = nullptr;
    lv_obj_t *btn3_label = nullptr;

    // State tracking
    UIState current_ui_state = UIState::STOPPED;
    unsigned long last_battery_update = 0;

    // System references
    hal::ISystemHAL *system_hal;

public:
    UIManager(hal::ISystemHAL *hal);
    ~UIManager() = default;

    // UI Creation
    void createUI();

    // UI Updates
    void updateBatteryUI();
    void updateWiFiUI();
    void updateWiFiUI(const bool connected);
    void updateReaperStateUI(const reaper::ReaperState &state);
    void updateTransportUI(const reaper::TransportState &state, const reaper::ReaperState &reaper_state);
    void updateButtonLabelsUI();

    // Periodic updates
    void updatePeriodicUI(unsigned long current_time);

    // State management
    UIState getCurrentUIState() const { return current_ui_state; }
    void setUIState(UIState state) { current_ui_state = state; }
};
