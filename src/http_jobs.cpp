#include "http_jobs.h"
#include "log.h"
#include "config.h"
#include "network_manager.h"
#include <cstring>
#include <sstream>
#include <ArduinoJson.h>

namespace http
{
    // Reaper command constants
    namespace commands
    {
        // Transport commands
        static const char *TRANSPORT = "TRANSPORT";
        static const char *PLAY = "1007";
        static const char *STOP = "1016";

        // Tab navigation commands
        static const char *NEXT_TAB = "40861";
        static const char *PREVIOUS_TAB = "40862";

        // ReaperSetlist commands
        static const char *GET_SCRIPT_ACTION_ID = "GET/EXTSTATE/ReaperSetlist/ScriptActionId";
        static const char *SET_OPERATION_GET_OPEN_TABS = "SET/EXTSTATE/ReaperSetlist/Operation/getOpenTabs";
        static const char *GET_TABS = "GET/EXTSTATE/ReaperSetlist/tabs";
        static const char *GET_ACTIVE_INDEX = "GET/EXTSTATE/ReaperSetlist/activeIndex";

        // Response prefixes
        static const char *EXTSTATE_PREFIX = "EXTSTATE";
        static const char *REAPER_SETLIST = "ReaperSetlist";
        static const char *SCRIPT_ACTION_ID_KEY = "ScriptActionId";
    }

    // Helper function to parse tab-separated response
    static std::vector<std::string> parseTabSeparatedResponse(const std::string &response)
    {
        std::vector<std::string> items;
        std::stringstream ss(response);
        std::string item;

        while (std::getline(ss, item, '\t'))
        {
            // Remove newline at end if present
            if (!item.empty() && item.back() == '\n')
            {
                item.pop_back();
            }
            if (!item.empty())
            {
                items.push_back(item);
            }
        }

        return items;
    }

    // WiFi Connection Job Constructor
    WiFiConnectJob::WiFiConnectJob(uint32_t id, NetworkManager *network)
        : HttpJob(id), network_manager(network)
    {
    }

    // WiFi Connection Job Implementation
    std::unique_ptr<HttpJobResult> WiFiConnectJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<WiFiConnectResult>(new WiFiConnectResult(job_id));

        LOG_INFO("WiFiConnectJob", "Attempting to connect to WiFi...");

        try
        {
            // Try to connect to WiFi using NetworkManager
            bool connected = network_manager->connectToWiFi();

            if (connected && network_mgr->isConnected())
            {
                result->success = true;
                result->connected = true;
                result->ip_address = network_mgr->getIP();
                LOG_INFO("WiFiConnectJob", "WiFi connected successfully. IP: {}", result->ip_address);
            }
            else
            {
                result->success = false;
                result->connected = false;
                LOG_ERROR("WiFiConnectJob", "Failed to connect to WiFi");
            }
        }
        catch (const std::exception &e)
        {
            result->success = false;
            result->connected = false;
            LOG_ERROR("WiFiConnectJob", "Exception during WiFi connection: {}", e.what());
        }

        return std::move(result);
    }

    // Helper function to build URL for single command
    static std::string buildCommandUrl(const std::string &base_url, const std::string &command)
    {
        return base_url + "/" + command;
    }

    // Helper function to build URL for multiple commands (separated by semicolons)
    static std::string buildCommandUrl(const std::string &base_url, const std::vector<std::string> &commands)
    {
        if (commands.empty())
        {
            return base_url;
        }

        std::string url = base_url + "/";
        for (size_t i = 0; i < commands.size(); ++i)
        {
            if (i > 0)
            {
                url += ";";
            }
            url += commands[i];
        }
        return url;
    }

    // Helper function to parse newline-separated batch response
    static std::vector<std::string> parseBatchResponse(const std::string &response)
    {
        std::vector<std::string> lines;
        std::stringstream ss(response);
        std::string line;

        while (std::getline(ss, line, '\n'))
        {
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            if (!line.empty())
            {
                lines.push_back(line);
            }
        }

        return lines;
    }

    // Helper function to parse transport state from tab-separated response
    static bool parseTransportState(const std::string &response, reaper::TransportState &transport_state)
    {
        auto items = parseTabSeparatedResponse(response);
        if (items.size() >= 4)
        {
            transport_state.play_state = std::stoi(items[1]);
            transport_state.position_seconds = std::stod(items[2]);
            transport_state.repeat_enabled = (items[3] == "1");
            transport_state.position_bars_beats = items[4];
            transport_state.success = true;
            return true;
        }
        return false;
    }

    // Helper function to parse individual tabs from tab data string
    static std::vector<reaper::TabInfo> parseTabData(const std::string &tab_data)
    {
        std::vector<reaper::TabInfo> tabs;

        // Tab data is JSON format: [{"length":297,"name":"Believer.RPP","index":0,"dirty":false},...]
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, tab_data);

        if (error)
        {
            LOG_ERROR("parseTabData", "Failed to parse JSON: {}", error.c_str());
            return tabs;
        }

        if (!doc.is<JsonArray>())
        {
            LOG_ERROR("parseTabData", "Tab data is not a JSON array");
            return tabs;
        }

        JsonArray tabArray = doc.as<JsonArray>();
        for (JsonObject tabObj : tabArray)
        {
            if (tabObj["length"].is<float>() && tabObj["name"].is<const char *>() && tabObj["index"].is<int>())
            {
                reaper::TabInfo tab;
                try
                {
                    tab.length = tabObj["length"].as<float>();
                    tab.name = tabObj["name"].as<std::string>();
                    tab.index = tabObj["index"].as<int>();

                    // Remove .rpp or .RPP extension if present
                    if (tab.name.size() >= 4)
                    {
                        std::string extension = tab.name.substr(tab.name.size() - 4);
                        if (extension == ".rpp" || extension == ".RPP")
                        {
                            tab.name = tab.name.substr(0, tab.name.size() - 4);
                        }
                    }

                    tabs.push_back(tab);
                }
                catch (const std::exception &e)
                {
                    LOG_ERROR("parseTabData", "Failed to parse tab object: {}", e.what());
                }
            }
            else
            {
                LOG_WARNING("parseTabData", "Tab object missing required fields");
            }
        }

        LOG_DEBUG("parseTabData", "Successfully parsed {} tabs from JSON", tabs.size());
        return tabs;
    }

    // ChangeTabJob implementation
    std::unique_ptr<HttpJobResult> ChangeTabJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<ChangeTabResult>(new ChangeTabResult(job_id));

        LOG_DEBUG("ChangeTabJob", "Executing job {} (direction: {})", job_id,
                  direction == TabDirection::NEXT ? "NEXT" : "PREVIOUS");

        // Single HTTP call with all commands batched
        std::string tab_command = (direction == TabDirection::NEXT) ? commands::NEXT_TAB : commands::PREVIOUS_TAB;

        std::vector<std::string> batch_commands = {
            tab_command,                           // Change tab
            commands::SET_OPERATION_GET_OPEN_TABS, // Set operation
            script_action_id,                      // Execute script action
            commands::GET_TABS,                    // Get tabs
            commands::GET_ACTIVE_INDEX,            // Get active index
            commands::TRANSPORT                    // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("ChangeTabJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 3)
        {
            LOG_ERROR("ChangeTabJob", "Invalid batch response - expected 3 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from last line (index 2)
        if (parseTransportState(lines[2], result->transport_state))
        {
            LOG_DEBUG("ChangeTabJob", "Successfully parsed transport state");
        }

        // Parse tabs from line 0 (GET_TABS response, 0-indexed so line 0)
        auto tab_items = parseTabSeparatedResponse(lines[0]);
        if (tab_items.size() >= 4 && tab_items[0] == commands::EXTSTATE_PREFIX &&
            tab_items[1] == commands::REAPER_SETLIST && tab_items[2] == "tabs")
        {
            std::string tab_data = tab_items[3];
            result->reaper_state.tabs = parseTabData(tab_data);
            LOG_DEBUG("ChangeTabJob", "Parsed {} tabs", result->reaper_state.tabs.size());
        }

        // Parse active index from line 1 (GET_ACTIVE_INDEX response, 0-indexed so line 1)
        auto index_items = parseTabSeparatedResponse(lines[1]);
        if (index_items.size() >= 4 && index_items[0] == commands::EXTSTATE_PREFIX &&
            index_items[1] == commands::REAPER_SETLIST && index_items[2] == "activeIndex")
        {
            try
            {
                result->reaper_state.active_index = std::stoi(index_items[3]);
                LOG_DEBUG("ChangeTabJob", "Got active index: {}", result->reaper_state.active_index);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("ChangeTabJob", "Failed to parse active index: {}", e.what());
            }
        }

        result->success = true;
        result->reaper_state.success = true;
        LOG_DEBUG("ChangeTabJob", "Job {} completed successfully", job_id);

        return result;
    }

    // ChangePlaystateJob implementation
    std::unique_ptr<HttpJobResult> ChangePlaystateJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<ChangePlaystateResult>(new ChangePlaystateResult(job_id));

        LOG_DEBUG("ChangePlaystateJob", "Executing job {} (action: {})", job_id, static_cast<int>(action));

        // Single HTTP call with both commands batched
        std::string command;
        switch (action)
        {
        case PlayAction::PLAY:
            command = commands::PLAY;
            break;
        case PlayAction::STOP:
            command = commands::STOP;
            break;
        }

        std::vector<std::string> batch_commands = {
            command,            // Change playstate
            commands::TRANSPORT // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);
        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("ChangePlaystateJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 1)
        {
            LOG_ERROR("ChangePlaystateJob", "Invalid batch response - expected 1 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from first line (index 0)
        if (parseTransportState(lines[0], result->transport_state))
        {
            result->success = true;
            LOG_DEBUG("ChangePlaystateJob", "Successfully parsed transport state");
        }

        LOG_DEBUG("ChangePlaystateJob", "Job {} completed successfully", job_id);

        return result;
    }

    // GetStatusJob implementation
    std::unique_ptr<HttpJobResult> GetStatusJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetStatusResult>(new GetStatusResult(job_id));

        LOG_DEBUG("GetStatusJob", "Executing job {}", job_id);

        // Single HTTP call with all commands batched
        std::vector<std::string> batch_commands = {
            commands::SET_OPERATION_GET_OPEN_TABS, // Set operation
            script_action_id,                      // Execute script action
            commands::GET_TABS,                    // Get tabs
            commands::GET_ACTIVE_INDEX,            // Get active index
            commands::TRANSPORT                    // Get transport state
        };

        std::string batch_url = buildCommandUrl(base_url, batch_commands);
        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(batch_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetStatusJob", "Batch request failed: status {}", status_code);
            return result;
        }

        // Parse batch response (newline-separated)
        auto lines = parseBatchResponse(response);
        if (lines.size() < 3)
        {
            LOG_ERROR("GetStatusJob", "Invalid batch response - expected 3 lines, got {}", lines.size());
            return result;
        }

        // Parse transport state from last line (index 2)
        if (parseTransportState(lines[2], result->transport_state))
        {
            LOG_DEBUG("GetStatusJob", "Successfully parsed transport state");
        }

        // Parse tabs from line 0 (GET_TABS response, 0-indexed so line 0)
        auto tab_items = parseTabSeparatedResponse(lines[0]);
        if (tab_items.size() >= 4 && tab_items[0] == commands::EXTSTATE_PREFIX &&
            tab_items[1] == commands::REAPER_SETLIST && tab_items[2] == "tabs")
        {
            std::string tab_data = tab_items[3];
            result->reaper_state.tabs = parseTabData(tab_data);
            LOG_DEBUG("GetStatusJob", "Parsed {} tabs", result->reaper_state.tabs.size());
        }

        // Parse active index from line 1 (GET_ACTIVE_INDEX response, 0-indexed so line 1)
        auto index_items = parseTabSeparatedResponse(lines[1]);
        if (index_items.size() >= 4 && index_items[0] == commands::EXTSTATE_PREFIX &&
            index_items[1] == commands::REAPER_SETLIST && index_items[2] == "activeIndex")
        {
            try
            {
                result->reaper_state.active_index = std::stoi(index_items[3]);
                LOG_DEBUG("GetStatusJob", "Got active index: {}", result->reaper_state.active_index);
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("GetStatusJob", "Failed to parse active index: {}", e.what());
            }
        }

        result->success = true;
        result->reaper_state.success = true;
        LOG_DEBUG("GetStatusJob", "Job {} completed successfully", job_id);

        return result;
    }

    // GetScriptActionIdJob implementation
    std::unique_ptr<HttpJobResult> GetScriptActionIdJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetScriptActionIdResult>(new GetScriptActionIdResult(job_id));

        LOG_DEBUG("GetScriptActionIdJob", "Executing job {}", job_id);

        // Get ReaperSetlist script action ID
        std::string command_url = buildCommandUrl(base_url, commands::GET_SCRIPT_ACTION_ID);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(command_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetScriptActionIdJob", "Script action ID request failed: status {}", status_code);
            return result;
        }

        // Parse the response - expected format: "EXTSTATE\tReaperSetlist\tScriptActionId\t{actual_id}"
        auto items = parseTabSeparatedResponse(response);
        if (items.size() >= 4 && items[0] == commands::EXTSTATE_PREFIX && items[1] == commands::REAPER_SETLIST && items[2] == commands::SCRIPT_ACTION_ID_KEY)
        {
            result->script_action_id = items[3];
            result->success = true;
            LOG_INFO("GetScriptActionIdJob", "Got ReaperSetlist script action ID: {}", result->script_action_id);
        }
        else
        {
            LOG_ERROR("GetScriptActionIdJob", "Invalid script action ID response format");
        }

        LOG_DEBUG("GetScriptActionIdJob", "Job {} completed", job_id);

        return result;
    }

    std::unique_ptr<HttpJobResult> GetTransportJob::execute(hal::INetworkManager *network_mgr, const std::string &base_url)
    {
        auto result = std::unique_ptr<GetTransportResult>(new GetTransportResult(job_id));

        LOG_DEBUG("GetTransportJob", "Executing job {}", job_id);

        // Get transport state only
        std::string command_url = buildCommandUrl(base_url, commands::TRANSPORT);

        std::string response;
        int status_code;

        if (!network_mgr->httpGetBlocking(command_url.c_str(), response, status_code) || status_code != 200)
        {
            LOG_ERROR("GetTransportJob", "Transport request failed: status {}", status_code);
            return result;
        }

        // Parse the transport state using existing helper function
        if (parseTransportState(response, result->transport_state))
        {
            result->success = true;
            LOG_DEBUG("GetTransportJob", "Got transport state: play_state={}, position={:.2f}s",
                      result->transport_state.play_state, result->transport_state.position_seconds);
        }
        else
        {
            LOG_ERROR("GetTransportJob", "Failed to parse transport response");
        }

        LOG_DEBUG("GetTransportJob", "Job {} completed", job_id);

        return result;
    }

} // namespace http
