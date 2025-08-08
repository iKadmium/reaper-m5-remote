"""
Button and LCD interaction example
"""

# Try to import real hardware, fall back to simulator
try:
    from m5stack import lcd, buttonA, buttonB, buttonC
    USING_SIMULATOR = False
except ImportError:
    import sys
    sys.path.append('../lib')
    from m5stack_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press
    USING_SIMULATOR = True
    print("Using M5Stack simulator")

import time
import random

# Colors
colors = [lcd.RED, lcd.GREEN, lcd.BLUE, lcd.YELLOW, lcd.CYAN, lcd.MAGENTA]
current_color_index = 0

def draw_circle():
    """Draw a random colored circle"""
    global current_color_index
    
    x = random.randint(50, 270)
    y = random.randint(50, 190)
    radius = random.randint(10, 30)
    color = colors[current_color_index]
    
    lcd.fillCircle(x, y, radius, color)
    
    # Update status
    lcd.fillRect(10, 200, 300, 20, lcd.BLACK)
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setCursor(10, 200)
    lcd.print(f"Circle at ({x},{y}) r={radius}")

def change_color():
    """Change the drawing color"""
    global current_color_index
    current_color_index = (current_color_index + 1) % len(colors)
    
    # Show current color
    color_name = ["RED", "GREEN", "BLUE", "YELLOW", "CYAN", "MAGENTA"][current_color_index]
    lcd.fillRect(10, 220, 300, 20, lcd.BLACK)
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setCursor(10, 220)
    lcd.print(f"Color: {color_name}")

def clear_screen():
    """Clear the drawing area"""
    lcd.fillRect(0, 40, 320, 150, lcd.BLACK)

def main():
    lcd.clear()
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setTextSize(2)
    lcd.setCursor(10, 10)
    lcd.print("Drawing Demo")
    
    lcd.setTextSize(1)
    lcd.setCursor(10, 30)
    lcd.print("A: Draw  B: Color  C: Clear")
    
    # Initial color display
    change_color()
    
    # Set up button callbacks
    buttonA.wasPressed(lambda p: draw_circle())
    buttonB.wasPressed(lambda p: change_color())
    buttonC.wasPressed(lambda p: clear_screen())
    
    # Simulator demo mode
    if USING_SIMULATOR:
        print("\n=== Drawing Demo ===")
        print("Simulating button presses...")
        
        # Demo sequence
        for i in range(8):
            if i % 3 == 0:
                print(f"\n--- Draw circle {i//3 + 1} ---")
                simulate_button_press('A')
            elif i % 3 == 1:
                print("--- Change color ---")
                simulate_button_press('B')
            else:
                print("--- Clear screen ---")
                simulate_button_press('C')
            
            time.sleep(0.5)
        
        print("\n=== Drawing Demo Complete ===")
        return
    
    # Real hardware mode - main loop
    while True:
        time.sleep(0.1)

if __name__ == "__main__":
    main()
