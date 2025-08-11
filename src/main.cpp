#include <lv_conf.h>
#include <lvgl.h>
#include <string>
#include "config.h"
#include "log.h"
#include "ui_manager.h"
#include "button_handler.h"
#include "state_manager.h"
#include "network_manager.h"
#include "http_job_manager.h"

#ifdef ARDUINO
#include "m5stack_hal.h"
using SystemHAL = hal::M5StackSystemHAL;
#else
#include "native_hal.h"
using SystemHAL = hal::NativeSystemHAL;
#endif

// Global system components
SystemHAL *g_system = nullptr;
UIManager *g_ui = nullptr;
ButtonHandler *g_button_handler = nullptr;
StateManager *g_state_manager = nullptr;
NetworkManager *g_network = nullptr;
http::HttpJobManager *g_http_manager = nullptr;

#ifdef ARDUINO
void setup()
#else
int main()
#endif
{
    // Create and initialize system HAL
    g_system = new SystemHAL();
    g_system->init();

    LOG_INFO("Main", "Application starting...");

    // Initialize network manager and connect to WiFi
    g_network = new NetworkManager(&g_system->getNetworkManager());
    g_network->connectToWiFi();

    // Initialize UI manager
    g_ui = new UIManager(g_system);
    g_ui->createUI();

    // Initialize HTTP job manager
    char url_buffer[256];
    snprintf(url_buffer, sizeof(url_buffer), "http://%s:%d/_", getReaperServer(), getReaperPort());
    std::string reaper_base_url(url_buffer);

    g_http_manager = new http::HttpJobManager(g_system, reaper_base_url);
    if (!g_http_manager->initialize())
    {
        LOG_ERROR("Main", "Failed to initialize HTTP job manager");
    }

    // Initialize state manager
    g_state_manager = new StateManager(g_http_manager, g_ui);

    // Initialize button handler
    g_button_handler = new ButtonHandler(&g_system->getInputManager(), g_http_manager, g_ui);
    g_button_handler->setStateReferences(&g_state_manager->getReaperState(), &g_state_manager->getTransportState());

    LOG_INFO("Main", "Application initialized");

#ifdef ARDUINO
}

void loop()
{
#else
    while (true)
    {
#endif
    // Update system
    g_system->update();

    // Get current time
    unsigned long current_time = g_system->getMillis();

    // Handle button presses
    g_button_handler->handleButtonPress();

    // Update state management
    g_state_manager->update(current_time);

    // Process HTTP job results
    if (g_http_manager)
    {
        auto results = g_http_manager->processResults();
        for (const auto &result : results)
        {
            // Handle different types of results
            if (result->result_type == http::ResultType::CHANGE_TAB)
            {
                auto change_tab_result = static_cast<const http::ChangeTabResult *>(result.get());
                LOG_DEBUG("Main", "Processing change tab result - tabs: {}, active_index: {}, play_state: {}",
                          change_tab_result->reaper_state.tabs.size(),
                          change_tab_result->reaper_state.active_index,
                          change_tab_result->transport_state.play_state);
                g_state_manager->updateReaperState(change_tab_result->reaper_state);
                g_state_manager->updateTransportState(change_tab_result->transport_state);
                g_button_handler->setAwaitingStateUpdate(false);

                // Update UI state based on transport state
                if (g_ui->getCurrentUIState() != UIState::ARE_YOU_SURE)
                {
                    if (change_tab_result->transport_state.play_state == 0)
                    {
                        g_ui->setUIState(UIState::STOPPED);
                    }
                    else if (change_tab_result->transport_state.play_state == 1)
                    {
                        g_ui->setUIState(UIState::PLAYING);
                    }
                }
            }
            else if (result->result_type == http::ResultType::CHANGE_PLAYSTATE)
            {
                auto change_playstate_result = static_cast<const http::ChangePlaystateResult *>(result.get());
                LOG_DEBUG("Main", "Processing change playstate result - play_state: {}",
                          change_playstate_result->transport_state.play_state);
                g_state_manager->updateTransportState(change_playstate_result->transport_state);
                g_button_handler->setAwaitingTransportUpdate(false);

                // Update UI state based on transport state
                if (change_playstate_result->transport_state.play_state == 0)
                {
                    g_ui->setUIState(UIState::STOPPED);
                }
                else if (change_playstate_result->transport_state.play_state == 1)
                {
                    g_ui->setUIState(UIState::PLAYING);
                }
            }
            else if (result->result_type == http::ResultType::GET_STATUS)
            {
                auto get_status_result = static_cast<const http::GetStatusResult *>(result.get());
                LOG_DEBUG("Main", "Processing get status result - tabs: {}, active_index: {}, play_state: {}",
                          get_status_result->reaper_state.tabs.size(),
                          get_status_result->reaper_state.active_index,
                          get_status_result->transport_state.play_state);
                g_state_manager->updateReaperState(get_status_result->reaper_state);
                g_state_manager->updateTransportState(get_status_result->transport_state);
                g_state_manager->setAwaitingStateUpdate(false);
                g_state_manager->setHaveReaperState(true);
            }
            else if (result->result_type == http::ResultType::GET_SCRIPT_ACTION_ID)
            {
                auto script_result = static_cast<const http::GetScriptActionIdResult *>(result.get());
                LOG_TRACE("Main", "Processing script action ID result");
                if (script_result->success && !script_result->script_action_id.empty())
                {
                    g_http_manager->setScriptActionId(script_result->script_action_id);
                    LOG_INFO("Main", "ReaperSetlist script action ID set: {}", script_result->script_action_id);
                }
                else
                {
                    LOG_ERROR("Main", "Failed to get ReaperSetlist script action ID");
                }
            }
            else if (result->result_type == http::ResultType::GET_TRANSPORT)
            {
                auto transport_result = static_cast<const http::GetTransportResult *>(result.get());
                LOG_DEBUG("Main", "Processing transport result - play_state: {}",
                          transport_result->transport_state.play_state);
                g_state_manager->updateTransportState(transport_result->transport_state);

                // Update UI state based on transport state if not in ARE_YOU_SURE mode
                if (g_ui->getCurrentUIState() != UIState::ARE_YOU_SURE)
                {
                    if (transport_result->transport_state.play_state == 0)
                    {
                        g_ui->setUIState(UIState::STOPPED);
                    }
                    else if (transport_result->transport_state.play_state == 1)
                    {
                        g_ui->setUIState(UIState::PLAYING);
                    }
                }
            }
        }
    }

    // Periodic transport updates when playing or in "are you sure" mode
    static unsigned long last_transport_update = 0;
    const unsigned long TRANSPORT_UPDATE_INTERVAL = 1000; // 1 second
    
    UIState current_ui_state = g_ui->getCurrentUIState();
    if ((current_ui_state == UIState::PLAYING || current_ui_state == UIState::ARE_YOU_SURE) &&
        (current_time - last_transport_update >= TRANSPORT_UPDATE_INTERVAL))
    {
        g_http_manager->submitGetTransportJob();
        last_transport_update = current_time;
        LOG_DEBUG("Main", "Submitted periodic transport update");
    }

    // Update UI elements based on current state
    g_ui->updateReaperStateUI(g_state_manager->getReaperState());
    g_ui->updateTransportUI(g_state_manager->getTransportState(), g_state_manager->getReaperState());
    g_ui->updateButtonLabelsUI();

    // Periodic UI updates (battery, WiFi, etc.)
    g_ui->updatePeriodicUI(current_time);

    // Force LVGL to refresh the display
    lv_obj_invalidate(lv_scr_act());

    // Force immediate display refresh
    lv_disp_t *disp = lv_disp_get_default();
    if (disp)
    {
        lv_refr_now(disp);
    }

    // Debug logging
    g_state_manager->periodicDebugLog(current_time);

    // Small delay to prevent overwhelming the system
    g_system->delay(1000 / 60);

#ifndef ARDUINO
}

// Cleanup
delete g_button_handler;
delete g_state_manager;
delete g_ui;
if (g_http_manager)
{
    delete g_http_manager;
}
delete g_network;
delete g_system;
return 0;
#endif
}
