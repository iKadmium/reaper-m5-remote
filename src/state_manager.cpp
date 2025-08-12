#include "state_manager.h"
#include "http_job_manager.h"
#include "log.h"

StateManager::StateManager(http::HttpJobManager *http_manager, UIManager *ui)
    : http_job_manager(http_manager), ui_manager(ui)
{
}

void StateManager::update(unsigned long current_time)
{
    if (!http_job_manager)
        return;

    // Update Reaper state if it's time and not awaiting async update
    if (http_job_manager->isWiFiConnected() && !awaiting_state_update && (current_time - last_reaper_update >= getReaperStateInterval()))
    {
        awaiting_state_update = true;
        http_job_manager->submitGetStatusJob();
        last_reaper_update = current_time;
        last_transport_update = current_time;
    }

    // Periodic transport updates when playing or in "are you sure" mode
    UIState current_ui_state = ui_manager->getCurrentUIState();
    if (http_job_manager->isWiFiConnected() && (current_ui_state == UIState::PLAYING || current_ui_state == UIState::ARE_YOU_SURE) && (current_time - last_transport_update >= 1000)) // 1 second interval
    {
        http_job_manager->submitGetTransportJob();
        last_transport_update = current_time;
        LOG_DEBUG("StateManager", "Submitted periodic transport update");
    }
}

void StateManager::periodicDebugLog(unsigned long current_time)
{
    static unsigned long last_ui_debug = 0;
    if (current_time - last_ui_debug >= 5000) // Every 5 seconds
    {
        UIState current_ui_state = ui_manager->getCurrentUIState();
        LOG_TRACE("UI", "UI State: {}, Tabs: {}, Active: {}, Transport: {}",
                  current_ui_state == UIState::STOPPED ? "STOPPED" : current_ui_state == UIState::PLAYING ? "PLAYING"
                                                                                                          : "ARE_YOU_SURE",
                  current_reaper_state.tabs.size(),
                  current_reaper_state.active_index,
                  current_transport_state.play_state);
        last_ui_debug = current_time;
    }
}

unsigned long StateManager::getReaperStateInterval() const
{
    if (!have_reaper_state)
    {
        return 1000; // Get it ASAP if we don't have it
    }
    return 10000; // Every 10 seconds if we have it
}

unsigned long StateManager::getTransportInterval() const
{
    UIState current_ui_state = ui_manager->getCurrentUIState();
    if (current_ui_state == UIState::STOPPED)
    {
        return 10000; // Every 10 seconds when stopped
    }
    return 1000; // Every 1 second when playing or "are you sure"
}
