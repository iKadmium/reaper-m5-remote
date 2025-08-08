"""
M5Stack Core MicroPython Application
Main entry point for the application
"""

import machine
import time

# Try to import real hardware, fall back to simulator
try:
    from m5stack import lcd, buttonA, buttonB, buttonC
    USING_SIMULATOR = False
    print("Using real M5Stack hardware")
except ImportError:
    import sys
    sys.path.append('lib')
    from m5stack_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press
    USING_SIMULATOR = True
    print("Using M5Stack simulator")

# Initialize the LCD
lcd.clear()
lcd.setRotation(1)  # Landscape orientation

# Display welcome message
lcd.setTextColor(lcd.WHITE, lcd.BLACK)
lcd.setTextSize(2)
lcd.setCursor(10, 10)
lcd.print("M5Stack Core")
lcd.setCursor(10, 40)
lcd.print("MicroPython App")

# Status display
lcd.setTextSize(1)
lcd.setCursor(10, 80)
lcd.print("Press A/B/C for actions")

def button_a_callback(pin):
    """Handle button A press"""
    lcd.fillRect(10, 100, 200, 20, lcd.BLACK)
    lcd.setCursor(10, 100)
    lcd.print("Button A pressed!")
    print("Button A pressed")

def button_b_callback(pin):
    """Handle button B press"""
    lcd.fillRect(10, 100, 200, 20, lcd.BLACK)
    lcd.setCursor(10, 100)
    lcd.print("Button B pressed!")
    print("Button B pressed")

def button_c_callback(pin):
    """Handle button C press"""
    lcd.fillRect(10, 100, 200, 20, lcd.BLACK)
    lcd.setCursor(10, 100)
    lcd.print("Button C pressed!")
    print("Button C pressed")

# Set up button interrupts
buttonA.wasPressed(button_a_callback)
buttonB.wasPressed(button_b_callback)
buttonC.wasPressed(button_c_callback)

def main():
    """Main application loop"""
    counter = 0
    
    # In simulator mode, run a demo and then exit
    if USING_SIMULATOR:
        print("\n=== Running M5Stack Simulator Demo ===")
        print("Watch the console output to see display commands")
        
        for i in range(10):
            # Update counter display
            lcd.fillRect(10, 120, 200, 20, lcd.BLACK)
            lcd.setCursor(10, 120)
            lcd.print(f"Counter: {counter}")
            
            # Simulate some button presses during demo
            if i == 3:
                print("\n--- Simulating button A press ---")
                simulate_button_press('A')
            elif i == 6:
                print("\n--- Simulating button B press ---")
                simulate_button_press('B')
            elif i == 8:
                print("\n--- Simulating button C press ---")
                simulate_button_press('C')
            
            counter += 1
            time.sleep(0.5)  # Faster for demo
        
        print("\n=== Demo Complete ===")
        print("To run interactively, use main_test.py")
        return
    
    # Real hardware mode - run continuous loop
    while True:
        # Update counter display
        lcd.fillRect(10, 120, 200, 20, lcd.BLACK)
        lcd.setCursor(10, 120)
        lcd.print(f"Counter: {counter}")
        
        counter += 1
        time.sleep(1)

if __name__ == "__main__":
    main()
