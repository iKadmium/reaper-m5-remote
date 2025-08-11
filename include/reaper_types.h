#pragma once

#include <string>
#include <vector>

namespace reaper
{
    // Structure for transport state information
    struct TransportState
    {
        int play_state;                  // 0=stopped, 1=playing, 2=paused, 5=recording, 6=record paused
        double position_seconds;         // Position in seconds
        bool repeat_enabled;             // Repeat on/off
        std::string position_bars_beats; // Position as bars.beats string
        bool success;                    // Whether parsing succeeded

        TransportState()
            : play_state(0), position_seconds(0.0), repeat_enabled(false), success(false)
        {
        }
    };

    // Structure for individual tab information
    struct TabInfo
    {
        float length;       // Length in seconds
        std::string name;   // Filename without .rpp/.RPP extension
        unsigned int index; // Array index

        TabInfo() : length(0.0f), index(0) {}
    };

    // Structure for Reaper setlist state
    struct ReaperState
    {
        std::vector<TabInfo> tabs;
        unsigned int active_index;
        bool success;

        ReaperState() : active_index(0), success(false) {}
    };

} // namespace reaper
