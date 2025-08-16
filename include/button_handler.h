#pragma once

#include "hal_interfaces.h"
#include "ui_manager.h"

// Forward declarations
namespace reaper
{
    struct ReaperState;
    struct TransportState;
}

// Forward declaration to avoid circular dependency
namespace http
{
    class HttpJobManager;
}

class ButtonHandler
{
private:
    hal::IInputManager *input_mgr;
    UIManager *ui_manager;
    http::HttpJobManager *http_job_manager;

    // State tracking for async operations
    bool awaiting_state_update = false;
    bool awaiting_transport_update = false;

    // State references
    reaper::ReaperState *current_reaper_state;
    reaper::TransportState *current_transport_state;

    void handleStoppedState();
    void handlePlayingState();
    void handleAreYouSureState();

    void handlePreviousTab();
    void handlePlay();
    void handleNextTab();
    void handleStopConfirmation();
    void handleStop();
    void handleCancel();

public:
    ButtonHandler(hal::IInputManager *input, http::HttpJobManager *http_manager, UIManager *ui);
    ~ButtonHandler() = default;

    // Set state references for callbacks
    void setStateReferences(reaper::ReaperState *reaper_state, reaper::TransportState *transport_state);

    // Main button handling
    bool handleButtonPress();

    // State control for HTTP job processing
    void setAwaitingStateUpdate(bool awaiting) { awaiting_state_update = awaiting; }
    void setAwaitingTransportUpdate(bool awaiting) { awaiting_transport_update = awaiting; }

    // State query
    bool isAwaitingUpdate() const { return awaiting_state_update || awaiting_transport_update; }
};
