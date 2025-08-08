"""
Power Management for M5Stack Core
Handles sleep/wake cycles to conserve battery during playback and idle periods
"""

import time
try:
    import machine
    HAS_MACHINE = True
except ImportError:
    HAS_MACHINE = False
    print("[POWER] Mock power management for simulator")

class PowerManager:
    def __init__(self):
        self.last_activity_time = time.time()
        self.idle_timeout = 60  # 60 seconds idle timeout
        self.song_wake_buffer = 5  # Wake up 5 seconds before song ends
        self.post_play_delay = 5  # Wait 5 seconds after play before sleeping
        self.is_sleeping = False
        
        # Battery simulation (since we can't read real battery in simulator)
        self.battery_start_time = time.time()
        self.estimated_battery_capacity = 110  # mAh
        
    def get_estimated_battery_percent(self):
        """Estimate battery percentage based on runtime (simulation)"""
        if not HAS_MACHINE:
            # Simulate battery drain for testing
            runtime_minutes = (time.time() - self.battery_start_time) / 60
            # Assume 110mAh lasts about 45-60 minutes of active use
            drain_rate = 100 / 52.5  # 100% over 52.5 minutes average
            battery_percent = max(0, 100 - (runtime_minutes * drain_rate))
            return int(battery_percent)
        
        try:
            # On real hardware, read actual battery voltage
            # This is a placeholder - actual implementation would read ADC
            return 75  # Placeholder
        except:
            return 50  # Fallback
        
    def reset_activity_timer(self):
        """Reset the activity timer (call when user interacts)"""
        self.last_activity_time = time.time()
        print("[POWER] Activity timer reset")
    
    def get_idle_time(self):
        """Get how long we've been idle"""
        return time.time() - self.last_activity_time
    
    def should_sleep_idle(self):
        """Check if we should sleep due to idle timeout"""
        return self.get_idle_time() >= self.idle_timeout
    
    def should_sleep_during_playback(self, time_info, track_length_sec):
        """
        Check if we should sleep during playback
        Returns (should_sleep, sleep_duration)
        """
        if not time_info.get("is_playing", False):
            return False, 0
            
        # Parse current position
        position_str = time_info.get("position", "0:00")
        try:
            parts = position_str.split(":")
            current_sec = int(parts[0]) * 60 + int(parts[1])
        except (ValueError, IndexError):
            current_sec = 0
        
        # If no track length available, don't sleep during playback
        if track_length_sec <= 0:
            return False, 0
        
        # Calculate remaining time
        remaining_sec = track_length_sec - current_sec
        
        # If less than wake buffer + post_play_delay remaining, don't sleep
        if remaining_sec <= (self.song_wake_buffer + self.post_play_delay):
            return False, 0
        
        # Calculate sleep duration (wake up before song ends)
        sleep_duration = remaining_sec - self.song_wake_buffer
        
        # Only sleep if we can sleep for at least 10 seconds
        if sleep_duration >= 10:
            return True, sleep_duration
        
        return False, 0
    
    def enter_light_sleep(self, duration_sec):
        """Enter light sleep for specified duration"""
        if not HAS_MACHINE:
            print(f"[POWER] Mock sleep for {duration_sec} seconds")
            time.sleep(min(2, duration_sec))  # Simulate shorter sleep in simulator
            return
        
        try:
            print(f"[POWER] Entering light sleep for {duration_sec} seconds")
            self.is_sleeping = True
            
            # Configure wake sources (buttons)
            # Note: Actual implementation would configure GPIO wake sources
            # For now, use basic sleep
            machine.lightsleep(int(duration_sec * 1000))  # Convert to milliseconds
            
        except Exception as e:
            print(f"[POWER] Sleep error: {e}")
        finally:
            self.is_sleeping = False
            print("[POWER] Woke up")
            self.reset_activity_timer()
    
    def enter_idle_sleep(self):
        """Enter sleep due to idle timeout"""
        print("[POWER] Entering idle sleep (60s timeout)")
        # In idle sleep, we sleep until button press
        # For safety, limit to 10 minutes max
        self.enter_light_sleep(600)
    
    def prepare_for_sleep(self, lcd):
        """Prepare display for sleep"""
        try:
            # Show sleep message
            lcd.fillRect(10, 40, 300, 180, 0x0)  # Clear area
            lcd.setTextColor(0x7BEF, 0x0)  # Dim white
            lcd.setTextSize(1)
            lcd.setCursor(10, 100)
            lcd.print("Sleeping to save battery...")
            lcd.setCursor(10, 120)
            lcd.print("Press any button to wake")
            
            # Dim the display (if supported)
            if hasattr(lcd, 'setBrightness'):
                lcd.setBrightness(10)  # Very dim
        except Exception as e:
            print(f"[POWER] Error preparing display for sleep: {e}")
    
    def wake_from_sleep(self, lcd):
        """Wake up actions"""
        try:
            # Restore display brightness
            if hasattr(lcd, 'setBrightness'):
                lcd.setBrightness(100)  # Full brightness
            
            print("[POWER] Waking up from sleep")
            self.reset_activity_timer()
        except Exception as e:
            print(f"[POWER] Error waking from sleep: {e}")
