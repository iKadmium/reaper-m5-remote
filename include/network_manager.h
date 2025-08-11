#pragma once

#include "hal_interfaces.h"
#include "config.h"
#include "log.h"

class NetworkManager
{
private:
    hal::INetworkManager *network_mgr;

public:
    NetworkManager(hal::INetworkManager *network);
    ~NetworkManager() = default;

    bool connectToWiFi();
};
