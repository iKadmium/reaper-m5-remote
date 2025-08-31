#pragma once

#include <lvgl.h>
#include <string>
#include "hal_interfaces.h"
#include "reaper_types.h"

enum class UIState
{
    DISCONNECTED,
    STOPPED,
    PLAYING,
    ARE_YOU_SURE
};

class UIManager
{
private:
    // UI containers
    lv_obj_t *global_container = nullptr;
    lv_obj_t *status_container = nullptr;        // Contains status row (always visible)
    lv_obj_t *main_ui_container = nullptr;       // Contains all main UI elements (tab info, transport, buttons)
    lv_obj_t *connection_status_label = nullptr; // Shows connection status messages

    // UI elements
    lv_obj_t *wifi_status_label = nullptr;
    lv_obj_t *reaper_status_label = nullptr;
    lv_obj_t *battery_icon_label = nullptr;
    lv_obj_t *battery_percentage_label = nullptr;
    lv_obj_t *tab_info_label = nullptr;
    lv_obj_t *play_icon_label = nullptr;
    lv_obj_t *tab_name_label = nullptr;
    lv_obj_t *time_label = nullptr;
    lv_obj_t *are_you_sure_label = nullptr;
    lv_obj_t *btn1_label = nullptr;
    lv_obj_t *btn2_label = nullptr;
    lv_obj_t *btn3_label = nullptr;

    // State tracking
    UIState current_ui_state = UIState::DISCONNECTED;
    unsigned long last_battery_update = 0;
    bool wifi_connected = false;
    bool reaper_connected = false;

    // System references
    hal::ISystemHAL *system_hal;

    // Private UI creation helpers
    void setupMainScreen();
    void createGlobalContainer(lv_obj_t *parent);
    void createStatusContainer(lv_obj_t *parent);
    void createConnectionStatusLabel(lv_obj_t *parent);
    void createMainUIContainer(lv_obj_t *parent);
    void createTabInfoSection(lv_obj_t *parent);
    void createTransportSection(lv_obj_t *parent);
    void createButtonSection(lv_obj_t *parent);

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

    // Connection state management
    void updateConnectionState(bool wifi_connected, bool reaper_connected);
    void showConnectionStatus(const std::string &message);
    void showMainUI();

    // Periodic updates
    void updatePeriodicUI(unsigned long current_time);

    // State management
    UIState getCurrentUIState() const { return current_ui_state; }
    void setUIState(UIState state) { current_ui_state = state; }
};
