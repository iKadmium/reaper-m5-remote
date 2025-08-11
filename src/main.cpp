#include <lv_conf.h>
#include <lvgl.h>
#include <string>
#include "config.h"
#include "reaper_client.h"
#include "log.h"
#include "ui_manager.h"
#include "button_handler.h"
#include "state_manager.h"
#include "network_manager.h"

#ifdef ARDUINO
#include "m5stack_hal.h"
using SystemHAL = hal::M5StackSystemHAL;
#else
#include "native_hal.h"
using SystemHAL = hal::NativeSystemHAL;
#endif

// Global system components
SystemHAL *g_system = nullptr;
reaper::ReaperClient *g_reaper = nullptr;
UIManager *g_ui = nullptr;
ButtonHandler *g_button_handler = nullptr;
StateManager *g_state_manager = nullptr;
NetworkManager *g_network = nullptr;

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

    // Initialize Reaper client
    reaper::ReaperConfig reaper_config(getReaperServer(), getReaperPort());
    g_reaper = new reaper::ReaperClient(&g_system->getNetworkManager(), reaper_config);

    // Initialize UI manager
    g_ui = new UIManager(g_system);
    g_ui->createUI();

    // Initialize state manager
    g_state_manager = new StateManager(g_reaper, g_ui);

    // Initialize button handler
    g_button_handler = new ButtonHandler(&g_system->getInputManager(), g_reaper, g_ui);
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

    // Update UI elements based on current state
    g_ui->updateReaperStateUI(g_state_manager->getReaperState());
    g_ui->updateTransportUI(g_state_manager->getTransportState(), g_state_manager->getReaperState());
    g_ui->updateButtonLabelsUI();

    // Periodic UI updates (battery, WiFi, etc.)
    g_ui->updatePeriodicUI(current_time);

    // Force LVGL to refresh the display
    lv_obj_invalidate(lv_scr_act());

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
delete g_reaper;
delete g_network;
delete g_system;
return 0;
#endif
}
