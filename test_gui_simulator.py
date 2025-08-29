#!/usr/bin/env python3
"""
Test script for M5Stack GUI Simulator
Run this to see the visual simulator in action
"""

import sys
import time
import os

# Add lib directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'lib'))

# Import the GUI simulator
from m5stack_gui_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press

def test_display():
    """Test the display functionality"""
    print("Testing M5Stack GUI Simulator...")
    
    # Clear screen
    lcd.clear(lcd.BLACK)
    time.sleep(0.5)
    
    # Set title
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setTextSize(2)
    lcd.setCursor(10, 10)
    lcd.print("M5Stack Test")
    time.sleep(1)
    
    # Add some colored text
    lcd.setTextColor(lcd.GREEN, lcd.BLACK)
    lcd.setTextSize(1)
    lcd.setCursor(10, 50)
    lcd.print("GUI Simulator Working!")
    time.sleep(1)
    
    # Draw some shapes
    lcd.fillRect(10, 80, 100, 20, lcd.BLUE)
    time.sleep(0.5)
    
    lcd.fillCircle(200, 90, 15, lcd.RED)
    time.sleep(0.5)
    
    # Add status message
    lcd.setTextColor(lcd.YELLOW, lcd.BLACK)
    lcd.setCursor(10, 120)
    lcd.print("Press buttons to test")
    time.sleep(0.5)
    
    # Test button callbacks
    def button_a_callback(pin):
        lcd.setTextColor(lcd.CYAN, lcd.BLACK)
        lcd.setCursor(10, 150)
        lcd.print("Button A pressed!")
        print("Button A callback triggered!")
    
    def button_b_callback(pin):
        lcd.setTextColor(lcd.MAGENTA, lcd.BLACK)
        lcd.setCursor(10, 170)
        lcd.print("Button B pressed!")
        print("Button B callback triggered!")
    
    def button_c_callback(pin):
        lcd.setTextColor(lcd.WHITE, lcd.BLACK)
        lcd.setCursor(10, 190)
        lcd.print("Button C pressed!")
        print("Button C callback triggered!")
    
    # Register button callbacks
    buttonA.wasPressed(button_a_callback)
    buttonB.wasPressed(button_b_callback) 
    buttonC.wasPressed(button_c_callback)
    
    print("GUI test complete! Try clicking the buttons in the simulator window.")
    print("The window should remain open for interaction.")
    
    # Keep the program running to interact with GUI
    try:
        print("Press Ctrl+C to exit...")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Exiting...")

if __name__ == "__main__":
    test_display()
