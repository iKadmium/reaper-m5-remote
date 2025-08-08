"""
Reaper API client for M5Stack Core
Handles communication with Reaper DAW via web interface
"""

try:
    import urequests as requests
except ImportError:
    try:
        import requests  # Use real requests for testing
        print("[REAPER] Using standard requests library")
    except ImportError:
        print("[REAPER] No HTTP library available")

import json
import time
from config import REAPER_BASE, STATE_SCRIPT_ID_KEY

# Type definitions (for documentation - MicroPython doesn't have full typing support)
class ReaperTab:
    """
    Represents a Reaper tab/project
    Expected structure:
    {
        "name": str,        # Tab/project name
        "path": float,        # File path
        "index": int,       # Tab index
        "modified": bool    # Has unsaved changes
    }
    """
    def __init__(self, name="", length=0.0, index=0, dirty=False):
        self.name = name
        self.length = length
        self.index = index
        self.dirty = dirty

    @classmethod
    def from_dict(cls, data):
        """Create ReaperTab from dictionary"""
        return cls(
            name=data.get("name", ""),
            length=data.get("length", ""),
            index=data.get("index", 0),
            dirty=data.get("dirty", False)
        )

class ReaperStatus:
    """
    Represents current Reaper status
    Expected structure:
    {
        "script_id": str,
        "timestamp": float,
        "connected": bool,
        "tabs": list[ReaperTab],
        "active_index": int,
        "active_tab": ReaperTab or None
    }
    """
    def __init__(self):
        self.script_id = None
        self.timestamp = 0.0
        self.connected = False
        self.tabs = []
        self.active_index = -1
        self.active_tab = None

class ReaperClient:
    def __init__(self, base_url=None):
        self.base_url = base_url or REAPER_BASE
        self.script_id = None
        self.last_update = 0

    def run_commands(self, commands):
        url = f"{self.base_url}/{str.join(";", commands)}"
        return requests.get(url, timeout=5)

    def get_state_var(self, state_key):
        """
        Get a state variable from Reaper
        Returns the value as a string, or None if failed
        """
        try:
            command = f"GET/EXTSTATE/ReaperSetlist/{state_key}"
            print(f"[REAPER] Getting state: {command}")

            response = self.run_commands([command])
            
            if response.status_code == 200:
                # Response is tab-delimited, fourth item is the value
                parts = response.text.strip().split('\t')
                if len(parts) >= 4:
                    value = parts[3]
                    print(f"[REAPER] State {state_key} = '{value}'")
                    return value
                else:
                    print(f"[REAPER] Invalid response format: {response.text}")
                    return None
            else:
                print(f"[REAPER] HTTP error {response.status_code}")
                return None
                
        except Exception as e:
            print(f"[REAPER] Error getting state {state_key}: {e}")
            return None
        finally:
            try:
                response.close()
            except:
                pass
    
    def find_script_id(self):
        """Find and cache the Reaper script ID"""
        if self.script_id is None:
            print("[REAPER] Looking for script ID...")
            script_id = self.get_state_var(STATE_SCRIPT_ID_KEY)
            if script_id and script_id.strip():
                self.script_id = script_id.strip()
                print(f"[REAPER] Found script ID: {self.script_id}")
                return True
            else:
                print("[REAPER] Script ID not found or empty")
                return False
        return True
    
    def get_current_status(self):
        """
        Get current status from Reaper
        Returns a ReaperStatus-like dictionary with tab information
        """
        try:
            # First ensure we have the script ID
            if not self.find_script_id():
                return {"error": "Script ID not found"}
            
            commands = [
                f"SET/EXTSTATE/ReaperSetlist/Operation/getOpenTabs",
                f"{self.script_id}",
                f"GET/EXTSTATE/ReaperSetlist/tabs",
                f"GET/EXTSTATE/ReaperSetlist/activeIndex"
            ]
            response = self.run_commands(commands)
            if response.status_code != 200:
                print(f"[REAPER] Error getting status: {response.status_code}")
                return {"error": f"HTTP {response.status_code}"}
            
            raw_output = response.text.strip()
            lines = raw_output.split('\n')
            
            # Parse tabs JSON
            tabs_raw = lines[0].split('\t')[3] if len(lines) > 0 else ""
            active_index = -1
            
            # Parse active index
            if len(lines) > 1:
                try:
                    active_index = int(lines[1].split('\t')[3])
                except (ValueError, IndexError):
                    active_index = -1
            
            # Parse tabs
            tabs = []
            active_tab = None
            
            try:
                tabs = json.loads(tabs_raw) if tabs_raw else []
                
                # Set active tab if we have a valid active index
                if 0 <= active_index < len(tabs):
                    active_tab = tabs[active_index]
                        
            except (json.JSONDecodeError, AttributeError) as e:
                print(f"[REAPER] Error parsing tabs JSON: {e}")
                tabs = []
                active_tab = None
            
            print(f"[REAPER] Status: {len(tabs)} tabs, active: {active_index}")
            if active_tab:
                print(f"[REAPER] Active tab: '{active_tab['name']}'")
            
            self.connected = True
            self.last_update = time.time()
            
            # Return structured data
            return {
                "tabs": tabs,
                "active_index": active_index,
                "active_tab": active_tab,
                "connected": True
            }
            
        except Exception as e:
            print(f"[REAPER] Error getting status: {e}")
            self.connected = False
            return {"error": str(e), "connected": False}    
        
    def get_play_time(self):
        """Get current playback position and play state"""
        try:
            commands = [
                "TRANSPORT"  # Get transport info: TRANSPORT\t1\t96.106667\t0\t35.1.30\t35.1.30
            ]
            response = self.run_commands(commands)
            if response.status_code != 200:
                return {"position": "0:00", "length": "0:00", "is_playing": False}
            
            lines = response.text.strip().split('\n')
            
            # Parse transport response: TRANSPORT	1	96.106667	0	35.1.30	35.1.30
            position_sec = 0
            is_playing = False
            
            if len(lines) > 0:
                try:
                    parts = lines[0].split('\t')
                    if len(parts) >= 3 and parts[0] == "TRANSPORT":
                        is_playing = parts[1] == "1"  # 1 = playing, 0 = stopped
                        position_sec = float(parts[2])  # Current position in seconds
                except (ValueError, IndexError):
                    position_sec = 0
                    is_playing = False
            
            # Convert to MM:SS format
            def format_time(seconds):
                minutes = int(seconds // 60)
                secs = int(seconds % 60)
                return f"{minutes}:{secs:02d}"
            
            return {
                "position": format_time(position_sec),
                "length": "0:00",  # We'll get length from tab data instead
                "is_playing": is_playing
            }
            
        except Exception as e:
            print(f"[REAPER] Error getting play time: {e}")
            return {"position": "0:00", "length": "0:00", "is_playing": False}
    
    def next_tab(self):
        """Switch to next tab"""
        try:
            command = "40861"  # Next project tab
            response = self.run_commands([command])
            if response.status_code == 200:
                print("[REAPER] Switched to next tab")
                return True
            else:
                print(f"[REAPER] Failed to switch to next tab: {response.status_code}")
                return False
        except Exception as e:
            print(f"[REAPER] Error switching to next tab: {e}")
            return False
    
    def prev_tab(self):
        """Switch to previous tab"""
        try:
            command = "40862"  # Previous project tab
            response = self.run_commands([command])
            if response.status_code == 200:
                print("[REAPER] Switched to previous tab")
                return True
            else:
                print(f"[REAPER] Failed to switch to previous tab: {response.status_code}")
                return False
        except Exception as e:
            print(f"[REAPER] Error switching to previous tab: {e}")
            return False
    
    def play(self):
        """Start playback"""
        try:
            command = "1007"  # Transport: Play
            response = self.run_commands([command])
            if response.status_code == 200:
                print("[REAPER] Started playback")
                return True
            else:
                print(f"[REAPER] Failed to start playback: {response.status_code}")
                return False
        except Exception as e:
            print(f"[REAPER] Error starting playback: {e}")
            return False
    
    def stop(self):
        """Stop playback"""
        try:
            command = "1016"  # Transport: Stop
            response = self.run_commands([command])
            if response.status_code == 200:
                print("[REAPER] Stopped playback")
                return True
            else:
                print(f"[REAPER] Failed to stop playback: {response.status_code}")
                return False
        except Exception as e:
            print(f"[REAPER] Error stopping playback: {e}")
            return False
    
    def toggle_play_stop(self):
        """Toggle between play and stop based on current state"""
        try:
            # Get current play state
            time_info = self.get_play_time()
            is_playing = time_info.get("is_playing", False)
            
            if is_playing:
                print("[REAPER] Currently playing, stopping...")
                return self.stop()
            else:
                print("[REAPER] Currently stopped, playing...")
                return self.play()
        except Exception as e:
            print(f"[REAPER] Error toggling play/stop: {e}")
            return False

    def toggle_play_pause(self):
        """Toggle play/pause - DEPRECATED, use toggle_play_stop instead"""
        try:
            command = "40044"  # Transport: Play/pause
            response = self.run_commands([command])
            if response.status_code == 200:
                print("[REAPER] Toggled play/pause")
                return True
            else:
                print(f"[REAPER] Failed to toggle play/pause: {response.status_code}")
                return False
        except Exception as e:
            print(f"[REAPER] Error toggling play/pause: {e}")
            return False

    def test_connection(self):
        """Test connection to Reaper"""
        print(f"[REAPER] Testing connection to {self.base_url}")
        try:
            # Try to get any state variable to test connectivity
            result = self.get_state_var("test")
            # Even if the state doesn't exist, we should get a response
            self.connected = True
            print("[REAPER] Connection test successful")
            return True
        except Exception as e:
            print(f"[REAPER] Connection test failed: {e}")
            self.connected = False
            return False
