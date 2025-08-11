#include "reaper_client.h"
#include "config.h"
#include "log.h"
#include <sstream>
#include <cstdio>
#include <ArduinoJson.h>

namespace reaper
{

    ReaperClient::ReaperClient(hal::INetworkManager *network, const ReaperConfig &config)
        : network_mgr(network)
    {
        char url_buffer[256];
        snprintf(url_buffer, sizeof(url_buffer), "http://%s:%d/_", config.server.c_str(), config.port);
        base_url = url_buffer;

        LOG_INFO("ReaperClient", "Initialized with base URL: {}", base_url.c_str());
    }

    CommandResponse ReaperClient::parseResponse(const std::string &response_text, bool success)
    {
        CommandResponse response;
        response.success = success;
        response.raw_response = response_text;

        if (!success || response_text.empty())
        {
            LOG_ERROR("ReaperClient", "parseResponse: Empty or failed response (success={}, length={})",
                      success, response_text.length());
            return response;
        }

        LOG_DEBUG("ReaperClient", "parseResponse: Parsing response: '{}'", response_text.c_str());

        // Split by tabs
        std::stringstream ss(response_text);
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
                LOG_DEBUG("Reaper", "parseResponse: Found item: '{}'", item.c_str());
                response.items.push_back(item);
            }
        }

        LOG_DEBUG("Reaper", "parseResponse: Final item count: {}", response.items.size());
        return response;
    }

    BatchResponse ReaperClient::parseBatchResponse(const std::string &response_text, bool success)
    {
        BatchResponse batch_response;
        batch_response.success = success;
        batch_response.raw_response = response_text;

        if (!success || response_text.empty())
        {
            return batch_response;
        }

        // Split by newlines - each line is a response to one command
        std::stringstream ss(response_text);
        std::string line;

        while (std::getline(ss, line))
        {
            CommandResponse cmd_response = parseResponse(line, true);
            batch_response.responses.push_back(cmd_response);
        }

        return batch_response;
    }

    TransportState ReaperClient::parseTransportState(const CommandResponse &response)
    {
        TransportState state;
        state.success = false;

        if (!response.success || response.items.size() < 5)
        {
            return state;
        }

        // Expected format: TRANSPORT, play_state, position_seconds, repeat, position_bars_beats, position_bars_beats
        if (response.items[0] != "TRANSPORT")
        {
            return state;
        }

        try
        {
            state.play_state = std::stoi(response.items[1]);
            state.position_seconds = std::stod(response.items[2]);
            state.repeat_enabled = (response.items[3] == "1");
            state.position_bars_beats = response.items[4];
            state.success = true;
        }
        catch (const std::exception &e)
        {
            LOG_WARNING("Reaper", "Failed to parse transport state: {}", e.what());
        }

        return state;
    }

    ReaperState ReaperClient::parseReaperState(const BatchResponse &batch_response)
    {
        ReaperState state;
        state.success = false;
        state.active_index = 0;

        LOG_INFO("Reaper", "parseReaperState: batch_success={}, responses={}",
                 batch_response.success, batch_response.responses.size());

        if (!batch_response.success || batch_response.responses.size() < 2)
        {
            LOG_WARNING("Reaper", "parseReaperState: Invalid batch response (success={}, size={})",
                        batch_response.success, batch_response.responses.size());
            return state;
        }

        // Expected responses (only commands that return data):
        // 0: GET/EXTSTATE/ReaperSetlist/tabs
        // 1: GET/EXTSTATE/ReaperSetlist/activeIndex
        // (SET and script action commands don't return responses)

        // Parse tabs (response index 0, not 2)
        const auto &tabs_response = batch_response.responses[0];
        LOG_INFO("Reaper", "parseReaperState: tabs_response success={}, items={}",
                 tabs_response.success, tabs_response.items.size());

        if (tabs_response.success && tabs_response.items.size() >= 4)
        {
            LOG_INFO("Reaper", "parseReaperState: tabs_response items: {}, {}, {}",
                     tabs_response.items[0].c_str(), tabs_response.items[1].c_str(), tabs_response.items[2].c_str());
        }

        if (tabs_response.success && tabs_response.items.size() >= 4 &&
            tabs_response.items[0] == "EXTSTATE" &&
            tabs_response.items[1] == "ReaperSetlist" &&
            tabs_response.items[2] == "tabs")
        {
            std::string tabs_json = tabs_response.items[3];
            LOG_INFO("Reaper", "parseReaperState: tabs_response items: {}", tabs_json.c_str());

            // Parse JSON using ArduinoJson
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, tabs_json);

            if (error)
            {
                LOG_WARNING("Reaper", "parseReaperState: tabs_response items: {}", error.c_str());
            }
            else if (doc.is<JsonArray>())
            {
                JsonArray tabs_array = doc.as<JsonArray>();
                LOG_INFO("Reaper", "parseReaperState: tabs_response items: {}", tabs_array.size());

                for (JsonObject tab_obj : tabs_array)
                {
                    TabInfo tab;

                    // Extract values from JSON object
                    tab.length = tab_obj["length"] | 0.0f;
                    tab.index = tab_obj["index"] | 0;

                    // Extract name and remove .rpp/.RPP extension
                    const char *name_str = tab_obj["name"];
                    if (name_str)
                    {
                        std::string name = name_str;
                        // Remove .rpp or .RPP extension
                        if (name.length() > 4)
                        {
                            std::string ext = name.substr(name.length() - 4);
                            if (ext == ".rpp" || ext == ".RPP")
                            {
                                name = name.substr(0, name.length() - 4);
                            }
                        }
                        tab.name = name;
                    }

                    LOG_INFO("Reaper", "parseReaperState: tabs_response items: {}, {}, {}",
                             tab.index, tab.name.c_str(), tab.length);
                    state.tabs.push_back(tab);
                }
                LOG_INFO("Reaper", "parseReaperState: tabs_response items: {}", state.tabs.size());
            }
            else
            {
                LOG_WARNING("Reaper", "parseReaperState: JSON is not an array");
            }
        }
        else
        {
            LOG_WARNING("Reaper", "parseReaperState: tabs_response validation failed");
        }

        // Parse activeIndex (response index 1, not 3)
        const auto &active_response = batch_response.responses[1];
        LOG_INFO("Reaper", "parseReaperState: active_response items: {}, {}",
                 active_response.success, active_response.items.size());

        if (active_response.success && active_response.items.size() >= 4)
        {
            LOG_INFO("Reaper", "parseReaperState: active_response items: {}, {}, {}, {}",
                     active_response.items[0].c_str(), active_response.items[1].c_str(),
                     active_response.items[2].c_str(), active_response.items[3].c_str());
        }

        if (active_response.success && active_response.items.size() >= 4 &&
            active_response.items[0] == "EXTSTATE" &&
            active_response.items[1] == "ReaperSetlist" &&
            active_response.items[2] == "activeIndex")
        {
            try
            {
                state.active_index = std::stoul(active_response.items[3]);
                LOG_INFO("Reaper", "parseReaperState: active_index items: {}", state.active_index);
            }
            catch (const std::exception &e)
            {
                LOG_WARNING("Reaper", "parseReaperState: active_index error: {}", e.what());
            }
        }
        else
        {
            LOG_WARNING("Reaper", "parseReaperState: active_response validation failed");
        }

        // Parse transport state (response index 2, not 4)
        state.transport = parseTransportState(batch_response.responses[2]);

        state.success = !state.tabs.empty();
        LOG_INFO("Reaper", "parseReaperState: overall state: {}, {}, {}", state.success, state.tabs.size(), state.active_index);
        LOG_INFO("Reaper", "parseReaperState: transport state: {}, {}, {}, {}, {}",
                 state.transport.play_state, state.transport.position_seconds,
                 state.transport.repeat_enabled, state.transport.position_bars_beats.c_str(),
                 state.transport.success);
        return state;
    }

    void ReaperClient::sendCommand(const std::string &command, CommandCallback callback)
    {
        if (!network_mgr)
        {
            LOG_ERROR("Reaper", "Network manager not available");
            if (callback)
            {
                CommandResponse response;
                response.success = false;
                callback(response);
            }
            return;
        }

        std::string url = base_url + "/" + command;
        LOG_INFO("Reaper", "sendCommand: {}", command.c_str());

        network_mgr->httpGet(url.c_str(), [this, callback, command](const char *response, int status_code)
                             {
        bool success = (status_code >= 200 && status_code < 300);
        std::string response_text = response ? response : "";
        
        if (success) {
            LOG_INFO("Reaper", "sendCommand: {}, response: {}", command.c_str(), response_text.c_str());
            // Add detailed response debugging
            LOG_INFO("Reaper", "{} characters", response_text.length());
            LOG_TRACE("Reaper", "Response: {}", response_text.c_str());
        } else {
            LOG_WARNING("Reaper", "Failed to send command '{}', status code: {}", 
                        command.c_str(), status_code);
        }
        
        if (callback) {
            CommandResponse cmd_response = parseResponse(response_text, success);
            LOG_INFO("Reaper", "{} items from response", cmd_response.items.size());
            for (size_t i = 0; i < cmd_response.items.size(); i++) {
                LOG_DEBUG("Reaper", "{} item from response", i, cmd_response.items[i].c_str());
            }
            callback(cmd_response);
        } });
    }

    void ReaperClient::sendBatch(const std::vector<std::string> &commands, BatchCallback callback)
    {
        if (commands.empty())
        {
            if (callback)
            {
                BatchResponse response;
                response.success = true;
                callback(response);
            }
            return;
        }

        // Join commands with semicolons
        std::string batch_command;
        for (size_t i = 0; i < commands.size(); ++i)
        {
            if (i > 0)
                batch_command += ";";
            batch_command += commands[i];
        }

        std::string url = base_url + "/" + batch_command;
        LOG_INFO("Reaper", "", batch_command.c_str());

        network_mgr->httpGet(url.c_str(), [this, callback, commands](const char *response, int status_code)
                             {
        bool success = (status_code >= 200 && status_code < 300);
        std::string response_text = response ? response : "";
        
        if (success) {
            LOG_INFO("Reaper", "{} commands)", commands.size());
        } else {
            LOG_WARNING("Reaper", "Failed to send batch commands, status code: {}", status_code);
        }
        
        if (callback) {
            BatchResponse batch_response = parseBatchResponse(response_text, success);
            callback(batch_response);
        } });
    }

    // Transport methods
    void ReaperClient::getTransportState(TransportCallback callback)
    {
        sendCommand("TRANSPORT", [this, callback](const CommandResponse &response)
                    {
            if (callback) {
                TransportState state = parseTransportState(response);
                callback(state);
            } });
    }

    void ReaperClient::play(CommandCallback callback)
    {
        sendCommand("1007", callback);
    }

    void ReaperClient::togglePlayPause(CommandCallback callback)
    {
        sendCommand("TRANSPORT", callback);
    }

    void ReaperClient::stop(CommandCallback callback)
    {
        sendCommand("1016", callback);
    }

    void ReaperClient::record(CommandCallback callback)
    {
        sendCommand("RECORD", callback);
    }

    void ReaperClient::rewind(CommandCallback callback)
    {
        sendCommand("REWIND", callback);
    }

    void ReaperClient::fastForward(CommandCallback callback)
    {
        sendCommand("FASTFORWARD", callback);
    }

    // Tab navigation
    void ReaperClient::nextTab(CommandCallback callback)
    {
        sendCommand("40861", callback);
    }

    void ReaperClient::previousTab(CommandCallback callback)
    {
        sendCommand("40862", callback);
    }

    void ReaperClient::getBPM(CommandCallback callback)
    {
        sendCommand("GET/TEMPO", callback);
    }

    void ReaperClient::getProjectName(CommandCallback callback)
    {
        sendCommand("GET/PROJECT_TITLE", callback);
    }

    void ReaperClient::getState(const std::string &section, const std::string &state_key, CommandCallback callback)
    {
        char command[128];
        snprintf(command, sizeof(command), "GET/EXTSTATE/%s/%s", section.c_str(), state_key.c_str());
        sendCommand(command, callback);
    }

    void ReaperClient::setState(const std::string &section, const std::string &state_key, const std::string &value, CommandCallback callback)
    {
        char command[256];
        snprintf(command, sizeof(command), "SET/EXTSTATE/%s/%s/%s", section.c_str(), state_key.c_str(), value.c_str());
        sendCommand(command, callback);
    }

    void ReaperClient::getCurrentReaperState(ReaperStateCallback callback)
    {
        if (script_id.empty())
        {
            LOG_WARNING("Reaper", "Cannot get current state - ScriptActionId not available");
            if (callback)
            {
                ReaperState state;
                state.success = false;
                callback(state);
            }
            return;
        }

        // Build the batch command as described:
        // 1. Set "Operation" state to "getOpenTabs"
        // 2. Call the script ID as an action
        // 3. Get the "tabs" state
        // 4. Get the "activeIndex" state
        std::vector<std::string> commands;
        commands.push_back("SET/EXTSTATE/ReaperSetlist/Operation/getOpenTabs");
        commands.push_back(script_id); // Call script action
        commands.push_back("GET/EXTSTATE/ReaperSetlist/tabs");
        commands.push_back("GET/EXTSTATE/ReaperSetlist/activeIndex");
        commands.push_back("TRANSPORT");

        sendBatch(commands, [this, callback](const BatchResponse &batch_response)
                  {
            if (callback) {
                ReaperState state = parseReaperState(batch_response);
                callback(state);
            } });
    }

    void ReaperClient::fetchScriptActionId(CommandCallback callback)
    {
        if (script_id_pending)
        {
            LOG_TRACE("Reaper", "fetchScriptActionId: Request already pending, skipping");
            if (callback)
            {
                CommandResponse response;
                response.success = false;
                callback(response);
            }
            return;
        }

        script_id_pending = true;
        LOG_INFO("Reaper", "Getting Script ID");
        getState("ReaperSetlist", "ScriptActionId", [this, callback](const CommandResponse &response)
                 {
            script_id_pending = false; // Clear pending flag
            LOG_INFO("Reaper", "ScriptActionId: {}, Items: {}", response.success, response.items.size());
            if (response.success && response.items.size() >= 4) {
                script_id = response.items[3];
                LOG_INFO("Reaper", "ScriptActionId: {}", script_id.c_str());
            } else {
                LOG_WARNING("Reaper", "Failed to fetch ScriptActionId");
                if (response.items.size() > 0) {
                    for (size_t i = 0; i < response.items.size(); i++) {
                        LOG_INFO("Reaper", "'", i, response.items[i].c_str());
                    }
                }
            }
            
            if (callback) {
                callback(response);
            } });
    }

    // BatchBuilder implementation
    BatchBuilder &BatchBuilder::add(const std::string &command)
    {
        commands.push_back(command);
        return *this;
    }

} // namespace reaper
