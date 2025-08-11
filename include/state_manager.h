#pragma once

#include "reaper_client.h"
#include "ui_manager.h"

class StateManager
{
private:
    reaper::ReaperClient *reaper_client;
    UIManager *ui_manager;

    // Current state
    reaper::ReaperState current_reaper_state;
    reaper::TransportState current_transport_state;

    // Timing
    unsigned long last_reaper_update = 0;
    unsigned long last_transport_update = 0;

    // Status flags
    bool have_reaper_state = false;
    bool awaiting_state_update = false;
    bool awaiting_transport_update = false;

    // UI state tracking
    int last_reaper_play_state = 0;

    // Helper methods
    unsigned long getReaperStateInterval() const;
    unsigned long getTransportInterval() const;
    void updateTransportStateFromReaper();

public:
    StateManager(reaper::ReaperClient *reaper, UIManager *ui);
    ~StateManager() = default;

    // Update methods
    void update(unsigned long current_time);
    void periodicDebugLog(unsigned long current_time);

    // State accessors
    const reaper::ReaperState &getReaperState() const { return current_reaper_state; }
    const reaper::TransportState &getTransportState() const { return current_transport_state; }
    reaper::ReaperState &getReaperState() { return current_reaper_state; }
    reaper::TransportState &getTransportState() { return current_transport_state; }

    // Status checks
    bool isAwaitingUpdate() const { return awaiting_state_update || awaiting_transport_update; }
    void setAwaitingStateUpdate(bool awaiting) { awaiting_state_update = awaiting; }
    void setAwaitingTransportUpdate(bool awaiting) { awaiting_transport_update = awaiting; }
};
