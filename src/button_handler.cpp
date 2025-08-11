#include "button_handler.h"
#include "http_job_manager.h"
#include "log.h"

ButtonHandler::ButtonHandler(hal::IInputManager *input, http::HttpJobManager *http_manager, UIManager *ui)
    : input_mgr(input), http_job_manager(http_manager), ui_manager(ui),
      current_reaper_state(nullptr), current_transport_state(nullptr)
{
}

void ButtonHandler::setStateReferences(reaper::ReaperState *reaper_state, reaper::TransportState *transport_state)
{
    current_reaper_state = reaper_state;
    current_transport_state = transport_state;
}

void ButtonHandler::handleButtonPress()
{
    if (!input_mgr || !http_job_manager || !ui_manager)
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
    http_job_manager->submitChangeTabJob(http::TabDirection::PREVIOUS);
}

void ButtonHandler::handlePlay()
{
    LOG_INFO("UI", "Play");
    awaiting_transport_update = true;
    http_job_manager->submitChangePlaystateJob(http::PlayAction::PLAY);
}

void ButtonHandler::handleNextTab()
{
    LOG_INFO("UI", "Next tab");
    awaiting_state_update = true;
    http_job_manager->submitChangeTabJob(http::TabDirection::NEXT);
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
    http_job_manager->submitChangePlaystateJob(http::PlayAction::STOP);
}

void ButtonHandler::handleCancel()
{
    LOG_INFO("UI", "Stop cancelled");
    ui_manager->setUIState(UIState::PLAYING);
}
