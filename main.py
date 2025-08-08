"""
M5Stack Reaper Remote Controller
Main application for controlling Reaper DAW
"""

import time
import sys

# Configuration
from config import (
    WIFI_SSID, WIFI_PASSWORD, STATUS_UPDATE_INTERVAL,
    DISPLAY_ROTATION, TEXT_SIZE_LARGE, TEXT_SIZE_NORMAL,
    COLOR_BG, COLOR_TEXT, COLOR_SUCCESS, COLOR_ERROR, COLOR_WARNING
)

# Try to import real hardware, fall back to simulator
try:
    from m5stack import lcd, buttonA, buttonB, buttonC
    import machine
    USING_SIMULATOR = False
    print("Using real M5Stack hardware")
except ImportError:
    sys.path.append('lib')
    from m5stack_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press
    import machine  # Mock machine module for simulator
    USING_SIMULATOR = True
    print("Using M5Stack simulator")

# Import our custom modules
sys.path.append('lib')
from wifi_manager import WiFiManager
from reaper_client import ReaperClient

class ReaperRemote:
    def __init__(self):
        self.wifi = WiFiManager()
        self.reaper = ReaperClient()
        self.last_status_update = 0
        self.script_id = None
        self.current_status = None
        
        # Display state
        self.current_screen = "startup"
        self.status_line = "Starting..."
        
        # Initialize display
        self.init_display()
    
    def init_display(self):
        """Initialize the display"""
        lcd.clear(COLOR_BG)
        lcd.setRotation(DISPLAY_ROTATION)
        lcd.setTextColor(COLOR_TEXT, COLOR_BG)
        
        # Title
        lcd.setTextSize(TEXT_SIZE_LARGE)
        lcd.setCursor(10, 10)
        lcd.print("Reaper Remote")
        
        # Status area
        lcd.setTextSize(TEXT_SIZE_NORMAL)
        self.update_status_display("Initializing...")
    
    def update_status_display(self, message, color=None):
        """Update the status line on display"""
        if color is None:
            color = COLOR_TEXT
            
        # Clear status area
        lcd.fillRect(10, 40, 300, 180, COLOR_BG)
        
        # Show status
        lcd.setTextColor(color, COLOR_BG)
        lcd.setCursor(10, 40)
        lcd.print("Status:")
        lcd.setCursor(10, 60)
        lcd.print(message)
        
        self.status_line = message
        print(f"[STATUS] {message}")
    
    def update_track_display(self):
        """Update the main track display"""
        if not self.current_status or "error" in self.current_status:
            # No valid status, show connection info instead
            self.update_connection_info()
            return
        
        # Clear display area
        lcd.fillRect(10, 40, 300, 180, COLOR_BG)
        
        tabs = self.current_status.get("tabs", [])
        active_index = self.current_status.get("active_index", -1)
        active_tab = self.current_status.get("active_tab")
        
        if not tabs:
            lcd.setTextColor(COLOR_WARNING, COLOR_BG)
            lcd.setCursor(10, 60)
            lcd.print("No tabs open")
            return
        
        # Track counter: [1 of 4]
        lcd.setTextColor(COLOR_TEXT, COLOR_BG)
        lcd.setTextSize(TEXT_SIZE_NORMAL)
        lcd.setCursor(10, 50)
        lcd.print(f"[{active_index + 1} of {len(tabs)}]")
        
        # Track name with play status
        if active_tab:
            track_name = active_tab.get("name", "Unknown Track")
            # Truncate if too long
            if len(track_name) > 18:
                track_name = track_name[:15] + "..."
            
            # Get play state for display
            time_info = self.reaper.get_play_time()
            play_indicator = "▶" if time_info.get("is_playing", False) else "■"
            
            lcd.setTextColor(COLOR_SUCCESS, COLOR_BG)
            lcd.setTextSize(TEXT_SIZE_LARGE)
            lcd.setCursor(10, 75)
            lcd.print(f"{play_indicator} {track_name}")
        
        # Time display - get actual time from Reaper
        time_info = self.reaper.get_play_time()
        
        # Get track length from tab data if available
        track_length = "0:00"
        if active_tab and "length" in active_tab:
            length_sec = active_tab["length"]
            if length_sec > 0:
                minutes = int(length_sec // 60)
                secs = int(length_sec % 60)
                track_length = f"{minutes}:{secs:02d}"
        
        lcd.setTextColor(COLOR_TEXT, COLOR_BG)
        lcd.setTextSize(TEXT_SIZE_NORMAL)
        lcd.setCursor(10, 110)
        lcd.print(f"{time_info['position']} / {track_length}")
        
        # Control buttons info - update based on play state
        is_playing = time_info.get("is_playing", False)
        play_button_label = "Stop" if is_playing else "Play"
        lcd.setCursor(10, 135)
        lcd.print(f"A:Prev B:{play_button_label} C:Next")
        
        # Connection status (small)
        lcd.setTextSize(1)
        lcd.setCursor(10, 220)
        if self.wifi.is_connected():
            lcd.setTextColor(COLOR_SUCCESS, COLOR_BG)
            lcd.print("WiFi OK")
        else:
            lcd.setTextColor(COLOR_ERROR, COLOR_BG)
            lcd.print("WiFi ERR")
    
    def update_connection_info(self):
        """Update connection information on display"""
        y_offset = 80
        
        # WiFi status
        if self.wifi.is_connected():
            lcd.setTextColor(COLOR_SUCCESS, COLOR_BG)
            lcd.setCursor(10, y_offset)
            lcd.print(f"WiFi: Connected")
            lcd.setCursor(10, y_offset + 15)
            lcd.print(f"IP: {self.wifi.get_ip()}")
        else:
            lcd.setTextColor(COLOR_ERROR, COLOR_BG)
            lcd.setCursor(10, y_offset)
            lcd.print("WiFi: Disconnected")
        
        y_offset += 40
        
        # Reaper status
        lcd.setTextColor(COLOR_SUCCESS, COLOR_BG)
        lcd.setCursor(10, y_offset)
        lcd.print("Reaper: Connected")
        if self.script_id:
            lcd.setCursor(10, y_offset + 15)
            lcd.print(f"Script: {self.script_id[:20]}")
    
    def ensure_wifi_connection(self):
        """Ensure WiFi is connected"""
        if not self.wifi.is_connected():
            self.update_status_display("Connecting to WiFi...", COLOR_WARNING)
            
            if self.wifi.connect(WIFI_SSID, WIFI_PASSWORD):
                self.update_status_display("WiFi connected!", COLOR_SUCCESS)
                return True
            else:
                self.update_status_display("WiFi connection failed!", COLOR_ERROR)
                return False
        return True
    
    def ensure_script_id(self):
        """Ensure we have the Reaper script ID"""
        if self.script_id is None:
            self.update_status_display("Finding Reaper script...", COLOR_WARNING)
            
            if self.reaper.find_script_id():
                self.script_id = self.reaper.script_id
                self.update_status_display("Script ID found!", COLOR_SUCCESS)
                return True
            else:
                self.update_status_display("Script ID not found!", COLOR_ERROR)
                return False
        return True
    
    def update_reaper_status(self):
        """Update status from Reaper if needed"""
        current_time = time.time()
        
        if current_time - self.last_status_update >= STATUS_UPDATE_INTERVAL:
            self.update_status_display("Updating status...", COLOR_WARNING)
            
            status = self.reaper.get_current_status()
            self.current_status = status
            self.last_status_update = current_time
            
            if "error" in status:
                self.update_status_display(f"Error: {status['error']}", COLOR_ERROR)
            else:
                self.update_status_display("Status updated", COLOR_SUCCESS)
            
            return True
        return False
    
    def setup_buttons(self):
        """Setup button callbacks"""
        def button_a_callback(pin):
            print("[BUTTON] A pressed - Previous Track")
            self.reaper.prev_tab()
            
        def button_b_callback(pin):
            print("[BUTTON] B pressed - Play/Stop")
            self.reaper.toggle_play_stop()
            
        def button_c_callback(pin):
            print("[BUTTON] C pressed - Next Track")
            self.reaper.next_tab()
        
        buttonA.wasPressed(button_a_callback)
        buttonB.wasPressed(button_b_callback)
        buttonC.wasPressed(button_c_callback)
    
    def run_demo_sequence(self):
        """Run a demo sequence for simulator mode - REMOVED, use real logic instead"""
        # This method is no longer used - we run the real logic in simulator mode too
        pass
    
    def run(self):
        """Main application loop"""
        print("[REAPER REMOTE] Starting...")
        
        # Setup buttons
        self.setup_buttons()
        
        # Main loop - same logic for both simulator and real hardware
        loop_count = 0
        max_loops = 20 if USING_SIMULATOR else float('inf')  # Limit loops in simulator
        
        while loop_count < max_loops:
            try:
                loop_count += 1
                print(f"\n[LOOP {loop_count}] Starting iteration...")
                
                # Step 1: Ensure WiFi connection
                if not self.ensure_wifi_connection():
                    print("[LOOP] WiFi connection failed, retrying in 5 seconds...")
                    time.sleep(5)
                    continue
                
                # Step 2: Ensure we have script ID
                if not self.ensure_script_id():
                    print("[LOOP] Script ID not found, retrying in 5 seconds...")
                    time.sleep(5)
                    continue
                
                # Step 3: Update status if needed
                status_updated = self.update_reaper_status()
                if status_updated:
                    print("[LOOP] Status updated successfully")
                
                # Update display with track info or connection info
                self.update_track_display()
                
                # In simulator mode, simulate some button presses
                if USING_SIMULATOR and loop_count % 5 == 0:
                    print(f"\n[SIMULATOR] Simulating button press (loop {loop_count})")
                    button_choice = ['A', 'B', 'C'][loop_count % 3]
                    simulate_button_press(button_choice)
                
                # Small delay to prevent tight loop
                sleep_time = 2 if USING_SIMULATOR else 0.1
                time.sleep(sleep_time)
                
            except KeyboardInterrupt:
                print("\n[REAPER REMOTE] Shutting down...")
                break
            except Exception as e:
                print(f"[REAPER REMOTE] Error in main loop: {e}")
                self.update_status_display(f"Error: {e}", COLOR_ERROR)
                time.sleep(1)
        
        if USING_SIMULATOR:
            print(f"\n[REAPER REMOTE] Simulator completed {loop_count} loops")

def main():
    """Entry point"""
    app = ReaperRemote()
    app.run()

if __name__ == "__main__":
    main()
