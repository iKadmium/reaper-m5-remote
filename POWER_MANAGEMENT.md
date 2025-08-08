# Power Management for M5Stack Reaper Remote

## Overview
The M5Stack Core has a 110mAh battery which provides approximately 45-60 minutes of continuous operation. To maximize battery life during live performances, the remote implements intelligent power management.

## Sleep Modes

### 1. Playback Sleep Mode
- **Trigger**: Song is playing and we've waited 5 seconds after pressing play
- **Duration**: Sleep until 5 seconds before the song ends
- **Wake Condition**: Button press or automatic wake before song end
- **Purpose**: Save battery during long songs when no interaction is needed

### 2. Idle Sleep Mode  
- **Trigger**: 60 seconds of no button activity (when not playing)
- **Duration**: Sleep until button press (max 10 minutes for safety)
- **Wake Condition**: Any button press
- **Purpose**: Save battery during breaks between songs

## Power Indicators

### Idle Timer
- **Display**: Bottom of screen shows "Idle:Xs"
- **Colors**: 
  - Green (0-29s): Normal activity
  - Yellow (30-49s): Approaching timeout
  - Red (50-59s): About to sleep
- **Reset**: Any button press resets to 0

### Battery Status
- **Display**: Bottom of screen shows "Bat:XX%"
- **Colors**:
  - Green (>50%): Good battery
  - Yellow (21-50%): Moderate battery
  - Red (≤20%): Low battery warning
- **Estimation**: Based on runtime (simulated) or voltage reading (real hardware)

## Implementation Details

### Sleep Calculation
```python
# For a 4:08 song at 0:10 position:
remaining_time = 248 - 10 = 238 seconds
sleep_duration = 238 - 5 = 233 seconds (wake 5s before end)
```

### Wake Sources
- Button A (Previous Track)
- Button B (Play/Stop)  
- Button C (Next Track)
- Timer expiration (for song end wake-up)

### Safety Features
- **Simulator Mode**: Sleep disabled for testing
- **Maximum Sleep**: Limited to 10 minutes to prevent getting stuck
- **Post-Play Delay**: Wait 5 seconds after play before sleep eligibility
- **Minimum Sleep**: Only sleep if duration ≥10 seconds

## Battery Life Estimates

### Without Power Management
- Continuous operation: ~45-60 minutes
- During 60-minute set: Battery depleted

### With Power Management  
- Typical song (4 minutes): Sleep for ~3.5 minutes = 87% battery saved
- 60-second idle periods: Full sleep = 100% battery saved
- **Estimated set duration**: 3-4 hours with power management

## Configuration
```python
# In power_manager.py
idle_timeout = 60          # Idle sleep after 60 seconds
song_wake_buffer = 5       # Wake 5 seconds before song end
post_play_delay = 5        # Wait 5 seconds after play
estimated_capacity = 110   # mAh battery capacity
```
