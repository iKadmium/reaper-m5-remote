#pragma once

#include "hal_interfaces.h"
#include "reaper_client.h"
#include "ui_manager.h"

class ButtonHandler
{
private:
    hal::IInputManager *input_mgr;
    reaper::ReaperClient *reaper_client;
    UIManager *ui_manager;

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
    ButtonHandler(hal::IInputManager *input, reaper::ReaperClient *reaper, UIManager *ui);
    ~ButtonHandler() = default;

    // Set state references for callbacks
    void setStateReferences(reaper::ReaperState *reaper_state, reaper::TransportState *transport_state);

    // Main button handling
    void handleButtonPress();

    // State query
    bool isAwaitingUpdate() const { return awaiting_state_update || awaiting_transport_update; }
};
