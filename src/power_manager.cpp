#include "power_manager.h"
#include "log.h"

PowerManager::PowerManager(hal::ISystemHAL *hal, UIManager *ui)
    : system_hal(hal), ui_manager(ui)
{
    // Initialize last button press to current time
    last_button_press_time = system_hal->getMillis();
    LOG_INFO("PowerManager", "Power manager initialized");
}

void PowerManager::onButtonPress()
{
    unsigned long current_time = system_hal->getMillis();
    unsigned long old_time = last_button_press_time;
    last_button_press_time = current_time;

    // If we were in any sleep mode, cancel it
    if (is_in_play_sleep || is_in_light_sleep)
    {
        LOG_INFO("PowerManager", "Button press during sleep - waking up");
        is_in_play_sleep = false;
        is_in_light_sleep = false;
        play_sleep_scheduled = false;
    }

    LOG_INFO("PowerManager", "Button press recorded at time %lu (was %lu, diff=%lu)",
             current_time, old_time, (current_time >= old_time) ? (current_time - old_time) : 0);
}

void PowerManager::onUIStateChange(UIState new_state, UIState old_state)
{
    unsigned long current_time = system_hal->getMillis();

    if (new_state == UIState::PLAYING && old_state != UIState::PLAYING)
    {
        // Started playing from stopped/paused state
        play_start_time = current_time;
        play_sleep_scheduled = true;
        is_in_play_sleep = false;
        LOG_INFO("PowerManager", "Play started from %d - scheduling sleep in %lu ms", (int)old_state, PLAY_SLEEP_DELAY);
    }
    else if (old_state == UIState::PLAYING && new_state != UIState::PLAYING)
    {
        // Stopped playing
        play_sleep_scheduled = false;
        is_in_play_sleep = false;
        LOG_INFO("PowerManager", "Play stopped - canceling play sleep");
    }
    else if (new_state == UIState::PLAYING && old_state == UIState::PLAYING)
    {
        // Still playing (track change, etc.) - don't reschedule sleep
        LOG_TRACE("PowerManager", "Still playing (track change) - keeping existing sleep schedule");
    }
}

void PowerManager::onTransportUpdate(const reaper::TransportState &transport_state, const reaper::ReaperState &reaper_state)
{
    if (transport_state.success && reaper_state.success &&
        reaper_state.active_index < reaper_state.tabs.size())
    {
        current_song_length = reaper_state.tabs[reaper_state.active_index].length;
        last_known_position = transport_state.position_seconds;

        LOG_TRACE("PowerManager", "Transport update: position=%.1fs, length=%.1fs",
                  last_known_position, current_song_length);
    }
}

void PowerManager::update(unsigned long current_time)
{
    if (!system_hal || !ui_manager)
        return;

    // Get our own timestamp to ensure consistency with onButtonPress()
    current_time = system_hal->getMillis();

    UIState current_ui_state = ui_manager->getCurrentUIState();

    // Check for tiered idle timeout (only if not on external power)
    if (!isOnExternalPower() || true)
    {
        // Protect against unsigned integer underflow
        if (current_time < last_button_press_time)
        {
            // Timer wrapped around, reset the button press time
            last_button_press_time = current_time;
            LOG_WARNING("PowerManager", "Timer wraparound detected, resetting button press time (current=%lu, last=%lu)",
                        current_time, last_button_press_time);
            return;
        }

        unsigned long time_since_button = current_time - last_button_press_time;

        // Additional safety check - if time difference is impossibly large, reset
        if (time_since_button > (24UL * 60UL * 60UL * 1000UL)) // More than 24 hours
        {
            LOG_WARNING("PowerManager", "Impossibly large time difference (%lu ms), resetting", time_since_button);
            last_button_press_time = current_time;
            return;
        }

        LOG_TRACE("PowerManager", "Idle check: current_time=%lu, last_button=%lu, diff=%lu, light_threshold=%lu, deep_threshold=%lu",
                  current_time, last_button_press_time, time_since_button, LIGHT_SLEEP_TIMEOUT, DEEP_SLEEP_TIMEOUT);

        if (time_since_button >= DEEP_SLEEP_TIMEOUT)
        {
            LOG_INFO("PowerManager", "Deep sleep timeout reached (%lu ms since last button press) - entering deep sleep",
                     time_since_button);

            // Enter indefinite deep sleep until button press wakes us up
            enterDeepSleep(0); // 0 means indefinite sleep
            return;
        }
        else if (time_since_button >= LIGHT_SLEEP_TIMEOUT && !is_in_light_sleep)
        {
            LOG_INFO("PowerManager", "Light sleep timeout reached (%lu ms since last button press) - entering light sleep",
                     time_since_button);

            // Enter light sleep until button press or deep sleep timeout
            unsigned long remaining_time = DEEP_SLEEP_TIMEOUT - time_since_button;
            is_in_light_sleep = true;
            enterLightSleep(remaining_time);
            return;
        }
    }

    // Handle play mode sleep logic (use light sleep for play mode)
    if (current_ui_state == UIState::PLAYING && play_sleep_scheduled && !is_in_play_sleep && !is_in_light_sleep)
    {
        unsigned long time_since_play_start = current_time - play_start_time;
        unsigned long time_since_button = current_time - last_button_press_time;

        // Don't sleep if there was recent button activity (within last 10 seconds)
        if (time_since_button < 10000) // 10 seconds grace period after button press
        {
            LOG_INFO("PowerManager", "Play sleep delayed - button pressed %lu ms ago", time_since_button);
            return;
        }

        if (time_since_play_start >= PLAY_SLEEP_DELAY)
        {
            // Time to enter play sleep (light sleep to preserve state)
            if (current_song_length > 0.0)
            {
                unsigned long sleep_duration = calculateSleepDuration(current_song_length, last_known_position);

                if (sleep_duration > 1000) // Only sleep if more than 1 second
                {
                    LOG_INFO("PowerManager", "Entering play light sleep for %lu ms (song ends in %.1fs)",
                             sleep_duration, current_song_length - last_known_position);

                    is_in_play_sleep = true;
                    play_sleep_scheduled = false;
                    enterLightSleep(sleep_duration); // Use light sleep for play mode
                }
                else
                {
                    LOG_INFO("PowerManager", "Song ending soon (%.1fs left) - not entering sleep",
                             current_song_length - last_known_position);
                    play_sleep_scheduled = false;
                }
            }
            else
            {
                LOG_WARNING("PowerManager", "Unknown song length - cannot calculate sleep duration");
                play_sleep_scheduled = false;
            }
        }
    }
}

bool PowerManager::isOnExternalPower() const
{
    if (!system_hal)
        return false;

    unsigned long current_time = system_hal->getMillis();

    // Use cached value if we checked recently to reduce I2C calls
    if (current_time - last_power_check_time < POWER_CHECK_INTERVAL)
    {
        return cached_external_power_status;
    }

    // Update cache
    auto &power = system_hal->getPowerManager();

    // Cast away const for cache updates (this is a common pattern for caching)
    PowerManager *non_const_this = const_cast<PowerManager *>(this);
    non_const_this->cached_external_power_status = power.isCharging();
    non_const_this->last_power_check_time = current_time;

    return cached_external_power_status;
}

unsigned long PowerManager::calculateSleepDuration(double song_length, double current_position) const
{
    double remaining_time = song_length - current_position;
    double sleep_time = remaining_time - (WAKEUP_BEFORE_END / 1000.0);

    if (sleep_time <= 0.0)
    {
        return 0; // Don't sleep if less than wakeup time remaining
    }

    return (unsigned long)(sleep_time * 1000.0); // Convert to milliseconds
}

void PowerManager::enterLightSleep(unsigned long duration_ms)
{
    if (!system_hal)
        return;

    auto &power = system_hal->getPowerManager();

    if (duration_ms == 0)
    {
        LOG_INFO("PowerManager", "Entering indefinite light sleep");
        power.lightSleep(DEEP_SLEEP_TIMEOUT); // Sleep until deep sleep timeout
    }
    else
    {
        LOG_INFO("PowerManager", "Entering light sleep for %lu ms", duration_ms);
        power.lightSleep(duration_ms);
    }
}

void PowerManager::enterDeepSleep(unsigned long duration_ms)
{
    if (!system_hal)
        return;

    auto &power = system_hal->getPowerManager();

    if (duration_ms == 0)
    {
        LOG_INFO("PowerManager", "Entering indefinite deep sleep");
        power.deepSleep(0);
    }
    else
    {
        LOG_INFO("PowerManager", "Entering deep sleep for %lu ms", duration_ms);
        power.deepSleep(duration_ms);
    }
}

bool PowerManager::shouldEnterSleep() const
{
    if (isOnExternalPower())
    {
        return false; // Never sleep on external power
    }

    unsigned long current_time = system_hal->getMillis();
    unsigned long time_since_button = current_time - last_button_press_time;

    return time_since_button >= LIGHT_SLEEP_TIMEOUT; // Use light sleep timeout as minimum
}

unsigned long PowerManager::getTimeSinceLastButtonPress(unsigned long current_time) const
{
    return current_time - last_button_press_time;
}
