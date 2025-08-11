#include "button_handler.h"
#include "log.h"

ButtonHandler::ButtonHandler(hal::IInputManager *input, reaper::ReaperClient *reaper, UIManager *ui)
    : input_mgr(input), reaper_client(reaper), ui_manager(ui), current_reaper_state(nullptr), current_transport_state(nullptr)
{
}

void ButtonHandler::setStateReferences(reaper::ReaperState *reaper_state, reaper::TransportState *transport_state)
{
    current_reaper_state = reaper_state;
    current_transport_state = transport_state;
}

void ButtonHandler::handleButtonPress()
{
    if (!input_mgr || !reaper_client || !ui_manager)
        return;

    // Check for button presses
    bool btn1_pressed = input_mgr->wasButtonPressed(0); // Button A
    bool btn2_pressed = input_mgr->wasButtonPressed(1); // Button B
    bool btn3_pressed = input_mgr->wasButtonPressed(2); // Button C

    if (!btn1_pressed && !btn2_pressed && !btn3_pressed)
        return;

    UIState current_state = ui_manager->getCurrentUIState();

    switch (current_state)
    {
    case UIState::STOPPED:
        handleStoppedState();
        break;
    case UIState::PLAYING:
        handlePlayingState();
        break;
    case UIState::ARE_YOU_SURE:
        handleAreYouSureState();
        break;
    }
}

void ButtonHandler::handleStoppedState()
{
    bool btn1_pressed = input_mgr->wasButtonPressed(0);
    bool btn2_pressed = input_mgr->wasButtonPressed(1);
    bool btn3_pressed = input_mgr->wasButtonPressed(2);

    if (btn1_pressed)
    {
        handlePreviousTab();
    }
    else if (btn2_pressed)
    {
        handlePlay();
    }
    else if (btn3_pressed)
    {
        handleNextTab();
    }
}

void ButtonHandler::handlePlayingState()
{
    bool btn2_pressed = input_mgr->wasButtonPressed(1);

    if (btn2_pressed)
    {
        handleStopConfirmation();
    }
    // btn1 and btn3 do nothing in playing state
}

void ButtonHandler::handleAreYouSureState()
{
    bool btn1_pressed = input_mgr->wasButtonPressed(0);
    bool btn2_pressed = input_mgr->wasButtonPressed(1);
    bool btn3_pressed = input_mgr->wasButtonPressed(2);

    if (btn1_pressed)
    {
        handleStop();
    }
    else if (btn2_pressed || btn3_pressed)
    {
        handleCancel();
    }
}

void ButtonHandler::handlePreviousTab()
{
    LOG_INFO("UI", "Previous tab");
    awaiting_state_update = true;
    reaper_client->previousTab();

    if (reaper_client->hasScriptId())
    {
        reaper_client->getCurrentReaperState([this](const reaper::ReaperState &state)
                                             {
            if (current_reaper_state) {
                *current_reaper_state = state;
            }
            if (current_transport_state) {
                *current_transport_state = state.transport;
            }
            awaiting_state_update = false;
            if (state.success) {
                LOG_TRACE("UI", "Tab state updated after previous");
                ui_manager->updateReaperStateUI(state);
                ui_manager->updateTransportUI(state.transport, state);

                // Update UI state based on transport state
                if (ui_manager->getCurrentUIState() != UIState::ARE_YOU_SURE) {
                    if (state.transport.play_state == 0) {
                        ui_manager->setUIState(UIState::STOPPED);
                    } else if (state.transport.play_state == 1) {
                        ui_manager->setUIState(UIState::PLAYING);
                    }
                    LOG_INFO("UI", "Tab changed - transport state: {}, UI state: {}", 
                        state.transport.play_state,
                        ui_manager->getCurrentUIState() == UIState::STOPPED ? "STOPPED" : "PLAYING");
                }
            } });
    }
}

void ButtonHandler::handlePlay()
{
    LOG_INFO("UI", "Play");
    awaiting_transport_update = true;
    reaper_client->play();

    reaper_client->getTransportState([this](const reaper::TransportState &state)
                                     {
        if (current_transport_state) {
            *current_transport_state = state;
        }
        awaiting_transport_update = false;
        if (state.success) {
            LOG_TRACE("UI", "Transport state updated after play");
            if (state.play_state == 1) {
                ui_manager->setUIState(UIState::PLAYING);
                LOG_INFO("UI", "UI state changed to PLAYING");
            } else {
                LOG_WARNING("UI", "Expected playing state but got {}", state.play_state);
            }
        } else {
            LOG_ERROR("UI", "Failed to get transport state after play command");
        } });
}

void ButtonHandler::handleNextTab()
{
    LOG_INFO("UI", "Next tab");
    awaiting_state_update = true;
    awaiting_transport_update = true;
    reaper_client->nextTab();

    if (reaper_client->hasScriptId())
    {
        reaper_client->getCurrentReaperState([this](const reaper::ReaperState &state)
                                             {
            if (current_reaper_state) {
                *current_reaper_state = state;
            }
            if (current_transport_state) {
                *current_transport_state = state.transport;
            }
            awaiting_state_update = false;
            if (state.success) {
                LOG_TRACE("UI", "Tab state updated after next");
                ui_manager->updateReaperStateUI(state);
                ui_manager->updateTransportUI(state.transport, state);

                if (current_transport_state) {
                    *current_transport_state = state.transport;
                }
                awaiting_transport_update = false;

                // Update UI state based on transport state
                if (ui_manager->getCurrentUIState() != UIState::ARE_YOU_SURE) {
                    if (state.transport.play_state == 0) {
                        ui_manager->setUIState(UIState::STOPPED);
                    } else if (state.transport.play_state == 1) {
                        ui_manager->setUIState(UIState::PLAYING);
                    }
                    LOG_INFO("UI", "Tab changed - transport state: {}, UI state: {}", 
                        state.transport.play_state,
                        ui_manager->getCurrentUIState() == UIState::STOPPED ? "STOPPED" : "PLAYING");
                }
            } });
    }
}

void ButtonHandler::handleStopConfirmation()
{
    LOG_INFO("UI", "Are you sure? (Stop confirmation)");
    ui_manager->setUIState(UIState::ARE_YOU_SURE);
}

void ButtonHandler::handleStop()
{
    LOG_INFO("UI", "Stop confirmed");
    awaiting_transport_update = true;
    reaper_client->stop();

    reaper_client->getTransportState([this](const reaper::TransportState &state)
                                     {
        if (current_transport_state) {
            *current_transport_state = state;
        }
        awaiting_transport_update = false;
        if (state.success) {
            LOG_TRACE("UI", "Transport state updated after stop");
            if (state.play_state == 0) {
                ui_manager->setUIState(UIState::STOPPED);
                LOG_INFO("UI", "UI state changed to STOPPED");
            } else {
                LOG_WARNING("UI", "Expected stopped state but got {}", state.play_state);
            }
        } else {
            LOG_ERROR("UI", "Failed to get transport state after stop command");
        } });
}

void ButtonHandler::handleCancel()
{
    LOG_INFO("UI", "Stop cancelled");
    ui_manager->setUIState(UIState::PLAYING);
}
