#include "network_manager.h"

NetworkManager::NetworkManager(hal::INetworkManager *network) : network_mgr(network)
{
}

bool NetworkManager::connectToWiFi()
{
    if (!network_mgr)
        return false;

    // Get WiFi credentials
    const char *ssid = getWiFiSSID();
    const char *password = getWiFiPassword();

    LOG_INFO("WiFi", "Connecting to network: {}", ssid);

    // Attempt to connect
    bool connected = network_mgr->connect(ssid, password);

    if (connected)
    {
        LOG_INFO("WiFi", "Connected successfully! IP: {}", network_mgr->getIP());
        return true;
    }
    else
    {
        LOG_ERROR("WiFi", "Failed to connect to WiFi");
        return false;
    }
}
