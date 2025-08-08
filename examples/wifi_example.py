"""
Simple WiFi connection example for M5Stack Core
"""

import sys
sys.path.append('/lib')

from wifi_manager import WiFiManager
from m5stack import lcd
import time

# WiFi credentials (replace with your network)
SSID = "your_wifi_ssid"
PASSWORD = "your_wifi_password"

def main():
    lcd.clear()
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setTextSize(2)
    lcd.setCursor(10, 10)
    lcd.print("WiFi Example")
    
    # Initialize WiFi manager
    wifi = WiFiManager()
    
    lcd.setTextSize(1)
    lcd.setCursor(10, 40)
    lcd.print("Scanning networks...")
    
    # Scan for networks
    networks = wifi.scan_networks()
    print("Available networks:")
    for ssid, strength in networks[:5]:  # Show first 5
        print(f"  {ssid}: {strength} dBm")
    
    # Connect to WiFi
    lcd.setCursor(10, 60)
    lcd.print(f"Connecting to {SSID}...")
    
    if wifi.connect(SSID, PASSWORD):
        lcd.fillRect(10, 60, 300, 20, lcd.BLACK)
        lcd.setCursor(10, 60)
        lcd.print("Connected!")
        lcd.setCursor(10, 80)
        lcd.print(f"IP: {wifi.get_ip()}")
    else:
        lcd.fillRect(10, 60, 300, 20, lcd.BLACK)
        lcd.setCursor(10, 60)
        lcd.print("Connection failed!")
    
    # Keep running
    while True:
        time.sleep(1)

if __name__ == "__main__":
    main()
