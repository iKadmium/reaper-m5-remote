

#pragma once

#include "hal_interfaces.h"
#include <string>
#include <vector>
#include <functional>

namespace reaper
{

    // Configuration for ReaperClient
    struct ReaperConfig
    {
        std::string server;
        int port;

        ReaperConfig(const std::string &srv, int p) : server(srv), port(p) {}
        ReaperConfig(const char *srv, int p) : server(srv), port(p) {}
    };

    // Structure to hold parsed response data
    struct CommandResponse
    {
        std::vector<std::string> items; // Tab-separated items from response
        bool success;                   // Whether the command succeeded
        std::string raw_response;       // Original response text
    };

    // Structure for batch command responses
    struct BatchResponse
    {
        std::vector<CommandResponse> responses; // One response per command
        bool success;                           // Whether the batch succeeded
        std::string raw_response;               // Original full response
    };

    // Structure for transport state information
    struct TransportState
    {
        int play_state;                  // 0=stopped, 1=playing, 2=paused, 5=recording, 6=record paused
        double position_seconds;         // Position in seconds
        bool repeat_enabled;             // Repeat on/off
        std::string position_bars_beats; // Position as bars.beats string
        bool success;                    // Whether parsing succeeded
    };

    // Structure for individual tab information
    struct TabInfo
    {
        float length;       // Length in seconds
        std::string name;   // Filename without .rpp/.RPP extension
        unsigned int index; // Array index
    };

    // Structure for Reaper setlist state
    struct ReaperState
    {
        std::vector<TabInfo> tabs;
        unsigned int active_index;
        bool success;
        TransportState transport;
    };

    // Callback types
    using CommandCallback = std::function<void(const CommandResponse &response)>;
    using BatchCallback = std::function<void(const BatchResponse &response)>;
    using TransportCallback = std::function<void(const TransportState &state)>;
    using ReaperStateCallback = std::function<void(const ReaperState &state)>;

    class ReaperClient
    {
    private:
        hal::INetworkManager *network_mgr;
        std::string base_url;
        std::string script_id;          // Store script action ID
        bool script_id_pending = false; // Prevent multiple concurrent requests

        // Helper to parse tab-separated response
        CommandResponse parseResponse(const std::string &response_text, bool success);
        BatchResponse parseBatchResponse(const std::string &response_text, bool success);
        TransportState parseTransportState(const CommandResponse &response);
        ReaperState parseReaperState(const BatchResponse &batch_response);

    public:
        ReaperClient(hal::INetworkManager *network, const ReaperConfig &config);

        // Send single command
        void sendCommand(const std::string &command, CommandCallback callback);

        // Send multiple commands in one request
        void sendBatch(const std::vector<std::string> &commands, BatchCallback callback);

        // Transport commands
        void getTransportState(TransportCallback callback);       // Get full transport state
        void play(CommandCallback callback = nullptr);            // Start/resume playback
        void togglePlayPause(CommandCallback callback = nullptr); // Toggle play/pause
        void stop(CommandCallback callback = nullptr);
        void record(CommandCallback callback = nullptr);

        void rewind(CommandCallback callback = nullptr);
        void fastForward(CommandCallback callback = nullptr);

        // Tab navigation
        void nextTab(CommandCallback callback = nullptr);
        void previousTab(CommandCallback callback = nullptr);

        void getBPM(CommandCallback callback);
        void getProjectName(CommandCallback callback);

        // State management
        void getState(const std::string &section, const std::string &state_key, CommandCallback callback);
        void setState(const std::string &section, const std::string &state_key, const std::string &value, CommandCallback callback = nullptr);

        // High-level Reaper operations
        void getCurrentReaperState(ReaperStateCallback callback);     // Gets tabs and activeIndex from Reaper
        void fetchScriptActionId(CommandCallback callback = nullptr); // Fetches and stores the script ID

        // Script ID management
        void setScriptId(const std::string &id) { script_id = id; }
        const std::string &getScriptId() const { return script_id; }
        bool hasScriptId() const { return !script_id.empty(); }

        // Utility
        const std::string &getBaseUrl() const { return base_url; }
    };

    // Helper class for building batch commands
    class BatchBuilder
    {
    private:
        std::vector<std::string> commands;

    public:
        BatchBuilder &add(const std::string &command);

        std::vector<std::string> build() const { return commands; }
        void clear() { commands.clear(); }
        size_t size() const { return commands.size(); }
    };

} // namespace reaper
