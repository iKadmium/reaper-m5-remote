"""
WiFi connection utility for M5Stack Core
"""

import network
import time

class WiFiManager:
    def __init__(self):
        self.wlan = network.WLAN(network.STA_IF)
        
    def connect(self, ssid, password, timeout=10):
        """Connect to WiFi network"""
        if self.is_connected():
            print("Already connected to WiFi")
            return True
            
        self.wlan.active(True)
        print(f"Connecting to {ssid}...")
        
        self.wlan.connect(ssid, password)
        
        # Wait for connection
        start_time = time.time()
        while not self.wlan.isconnected():
            if time.time() - start_time > timeout:
                print("WiFi connection timeout")
                return False
            time.sleep(0.1)
        
        print("WiFi connected!")
        print("IP address:", self.wlan.ifconfig()[0])
        return True
    
    def disconnect(self):
        """Disconnect from WiFi"""
        if self.wlan.isconnected():
            self.wlan.disconnect()
            print("WiFi disconnected")
    
    def is_connected(self):
        """Check if connected to WiFi"""
        return self.wlan.isconnected()
    
    def get_ip(self):
        """Get IP address"""
        if self.is_connected():
            return self.wlan.ifconfig()[0]
        return None
    
    def scan_networks(self):
        """Scan for available networks"""
        self.wlan.active(True)
        networks = self.wlan.scan()
        return [(net[0].decode(), net[3]) for net in networks]  # (SSID, signal strength)
