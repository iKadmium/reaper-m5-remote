"""
WiFi connection utility for M5Stack Core
"""

import time

class WiFiManager:
    def __init__(self):
        try:
            import network
            self.wlan = network.WLAN(network.STA_IF)
        except ImportError:
            # Mock for simulator
            class MockWLAN:
                def __init__(self):
                    self._active = False
                    self._connected = False
                    self._ip = None
                
                def active(self, state=None):
                    if state is not None:
                        self._active = state
                    return self._active
                
                def connect(self, ssid, password):
                    print(f"[WIFI MOCK] Attempting to connect to {ssid}...")
                    # Simulate connection attempt
                    import time
                    time.sleep(0.5)  # Simulate connection delay
                    
                    # Simulate successful connection
                    self._connected = True
                    self._ip = "192.168.1.50"
                    print(f"[WIFI MOCK] Connected to {ssid} with IP {self._ip}")
                    return True
                
                def isconnected(self):
                    return self._connected
                
                def disconnect(self):
                    self._connected = False
                    self._ip = None
                
                def ifconfig(self):
                    return [self._ip, "255.255.255.0", "192.168.1.1", "8.8.8.8"]
                
                def scan(self):
                    return [(b"TestWiFi", None, None, -50), (b"AnotherNet", None, None, -70)]
            
            self.wlan = MockWLAN()
        
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
