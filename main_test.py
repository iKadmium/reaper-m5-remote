"""
Test version of main.py that works with the simulator
"""

import sys
import time

# Use simulator instead of real hardware
try:
    # Try to import real m5stack (will fail on Unix)
    from m5stack import lcd, buttonA, buttonB, buttonC
    print("Using real M5Stack hardware")
except ImportError:
    # Use simulator
    sys.path.append('lib')
    from m5stack_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press
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

def test_mode():
    """Interactive test mode for simulator"""
    print("\n=== M5Stack Simulator Test Mode ===")
    print("Commands:")
    print("  a, b, c  - Press button A, B, or C")
    print("  status   - Show display state")
    print("  quit     - Exit test mode")
    print("  help     - Show this help")
    
    while True:
        try:
            cmd = input("\nTest> ").strip().lower()
            
            if cmd in ['a', 'b', 'c']:
                simulate_button_press(cmd)
            elif cmd == 'status':
                from m5stack_simulator import get_display_state
                state = get_display_state()
                print("Display State:")
                for key, value in state.items():
                    if key == 'commands':
                        print(f"  {key}: {len(value)} commands")
                        for i, cmd in enumerate(value[-5:]):  # Show last 5
                            print(f"    {i}: {cmd}")
                    else:
                        print(f"  {key}: {value}")
            elif cmd == 'quit':
                break
            elif cmd == 'help':
                print("Commands: a, b, c, status, quit, help")
            else:
                print(f"Unknown command: {cmd}")
                
        except (EOFError, KeyboardInterrupt):
            break
    
    print("Exiting test mode")

def main():
    """Main application loop"""
    # Check if we're in simulator mode and running interactively
    if 'simulate_button_press' in globals():
        print("\nRunning in simulator mode!")
        print("You can use test_mode() for interactive testing")
        
        # Run a brief demo
        counter = 0
        for i in range(5):
            # Update counter display
            lcd.fillRect(10, 120, 200, 20, lcd.BLACK)
            lcd.setCursor(10, 120)
            lcd.print(f"Counter: {counter}")
            
            # Simulate some button presses during demo
            if i == 2:
                simulate_button_press('A')
            elif i == 4:
                simulate_button_press('B')
            
            counter += 1
            time.sleep(1)
        
        print("\nDemo complete. Use test_mode() for interactive testing.")
        return
    
    # Real hardware mode - run continuous loop
    counter = 0
    while True:
        # Update counter display
        lcd.fillRect(10, 120, 200, 20, lcd.BLACK)
        lcd.setCursor(10, 120)
        lcd.print(f"Counter: {counter}")
        
        counter += 1
        time.sleep(1)

if __name__ == "__main__":
    main()
