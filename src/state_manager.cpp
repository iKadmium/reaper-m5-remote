#include "state_manager.h"
#include "log.h"

StateManager::StateManager(reaper::ReaperClient *reaper, UIManager *ui)
    : reaper_client(reaper), ui_manager(ui)
{
}

void StateManager::update(unsigned long current_time)
{
    if (!reaper_client)
        return;

    // Check if we need to get the ScriptActionId
    if (!reaper_client->hasScriptId())
    {
        reaper_client->fetchScriptActionId();
        return;
    }

    // Update Reaper state if it's time and not awaiting async update
    if (!awaiting_state_update && (current_time - last_reaper_update >= getReaperStateInterval()))
    {
        reaper_client->getCurrentReaperState([this](const reaper::ReaperState &state)
                                             {
            current_reaper_state = state;
            current_transport_state = state.transport;
            
            if (state.success) {
                have_reaper_state = true;
                LOG_TRACE("Main", "Reaper state updated: {} tabs, active tab: {}", 
                    state.tabs.size(), state.active_index);
                
                if (state.active_index < state.tabs.size()) {
                    LOG_INFO("Main", "Current tab: {} ({:.1f}s)", 
                        state.tabs[state.active_index].name.c_str(),
                        state.tabs[state.active_index].length);
                }
                ui_manager->updateReaperStateUI(state);
                ui_manager->updateTransportUI(state.transport, state);
            } else {
                LOG_WARNING("Main", "Failed to update Reaper state");
            } });
        last_reaper_update = current_time;
        last_transport_update = current_time;
    }

    // Update transport state if it's time and not awaiting async transport update
    if (!awaiting_transport_update && (current_time - last_transport_update >= getTransportInterval()))
    {
        updateTransportStateFromReaper();
        last_transport_update = current_time;
    }
}

void StateManager::updateTransportStateFromReaper()
{
    if (!reaper_client)
        return;

    reaper_client->getTransportState([this](const reaper::TransportState &state)
                                     {
        current_transport_state = state;
        
        if (state.success) {
            int play_state = state.play_state;
            
            // Only update UI state if we're not in "Are you sure?" mode
            // and the play state actually changed
            UIState current_ui_state = ui_manager->getCurrentUIState();
            if (current_ui_state != UIState::ARE_YOU_SURE && play_state != last_reaper_play_state) {
                if (play_state == 0) {
                    ui_manager->setUIState(UIState::STOPPED);
                    LOG_TRACE("UI", "State: STOPPED");
                } else if (play_state == 1) {
                    ui_manager->setUIState(UIState::PLAYING);
                    LOG_TRACE("UI", "State: PLAYING");
                }
                last_reaper_play_state = play_state;
            }
        } });

    ui_manager->updateTransportUI(current_transport_state, current_reaper_state);
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
