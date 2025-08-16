#pragma once

#include "hal_interfaces.h"
#include "ui_manager.h"
#include "reaper_types.h"

class PowerManager
{
private:
    hal::ISystemHAL *system_hal;
    UIManager *ui_manager;

    // Sleep timing constants (in milliseconds)
    static const unsigned long LIGHT_SLEEP_TIMEOUT = 30000; // 30 seconds -> light sleep
    static const unsigned long DEEP_SLEEP_TIMEOUT = 90000;  // 90 seconds -> deep sleep
    static const unsigned long PLAY_SLEEP_DELAY = 5000;     // 5 seconds after play starts
    static const unsigned long WAKEUP_BEFORE_END = 15000;   // 15 seconds before song ends

    // State tracking
    unsigned long last_button_press_time = 0;
    unsigned long play_start_time = 0;
    bool is_in_play_sleep = false;
    bool play_sleep_scheduled = false;
    bool is_in_light_sleep = false;

    // External power status caching to reduce I2C calls
    bool cached_external_power_status = false;
    unsigned long last_power_check_time = 0;
    static const unsigned long POWER_CHECK_INTERVAL = 5000; // Check power status every 5 seconds

    // Song timing
    double current_song_length = 0.0;
    double last_known_position = 0.0;

    // Helper methods
    bool isOnExternalPower() const;
    unsigned long calculateSleepDuration(double song_length, double current_position) const;
    void enterLightSleep(unsigned long duration_ms);
    void enterDeepSleep(unsigned long duration_ms);

public:
    PowerManager(hal::ISystemHAL *hal, UIManager *ui);
    ~PowerManager() = default;

    // Called when any button is pressed
    void onButtonPress();

    // Called when UI state changes
    void onUIStateChange(UIState new_state, UIState old_state);

    // Called when transport state updates (for song position tracking)
    void onTransportUpdate(const reaper::TransportState &transport_state, const reaper::ReaperState &reaper_state);

    // Main update loop - checks for sleep conditions
    void update(unsigned long current_time);

    // Query methods
    bool shouldEnterSleep() const;
    unsigned long getTimeSinceLastButtonPress(unsigned long current_time) const;
    bool isPlaySleepActive() const { return is_in_play_sleep; }
};
