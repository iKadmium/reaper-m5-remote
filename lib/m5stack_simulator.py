"""
M5Stack Core Hardware Simulator for Unix MicroPython
This module provides mock implementations of M5Stack hardware for testing
"""

import time
from collections import namedtuple

# Color constants (RGB565 format simulation)
class Colors:
    BLACK = 0x0000
    WHITE = 0xFFFF
    RED = 0xF800
    GREEN = 0x07E0
    BLUE = 0x001F
    YELLOW = 0xFFE0
    CYAN = 0x07FF
    MAGENTA = 0xF81F

class MockLCD:
    """Mock LCD display for testing"""
    
    def __init__(self):
        self.width = 320
        self.height = 240
        self.text_size = 1
        self.text_color = Colors.WHITE
        self.bg_color = Colors.BLACK
        self.cursor_x = 0
        self.cursor_y = 0
        self.rotation = 0
        self.buffer = []  # Store drawing commands for debugging
        
        # Add color constants as attributes
        self.BLACK = Colors.BLACK
        self.WHITE = Colors.WHITE
        self.RED = Colors.RED
        self.GREEN = Colors.GREEN
        self.BLUE = Colors.BLUE
        self.YELLOW = Colors.YELLOW
        self.CYAN = Colors.CYAN
        self.MAGENTA = Colors.MAGENTA
    
    def clear(self, color=None):
        if color is None:
            color = Colors.BLACK
        self.buffer.append(f"CLEAR(color={hex(color)})")
        print(f"[LCD] Screen cleared with color {hex(color)}")
    
    def setRotation(self, rotation):
        self.rotation = rotation
        self.buffer.append(f"ROTATION({rotation})")
        print(f"[LCD] Rotation set to {rotation}")
    
    def setTextColor(self, color, bg_color=None):
        self.text_color = color
        if bg_color is not None:
            self.bg_color = bg_color
        self.buffer.append(f"TEXT_COLOR({hex(color)}, {hex(bg_color) if bg_color is not None else 'None'})")
        print(f"[LCD] Text color: {hex(color)}, background: {hex(bg_color) if bg_color is not None else 'transparent'}")
    
    def setTextSize(self, size):
        self.text_size = size
        self.buffer.append(f"TEXT_SIZE({size})")
        print(f"[LCD] Text size set to {size}")
    
    def setCursor(self, x, y):
        self.cursor_x = x
        self.cursor_y = y
        self.buffer.append(f"CURSOR({x}, {y})")
        print(f"[LCD] Cursor set to ({x}, {y})")
    
    def print(self, text):
        self.buffer.append(f"PRINT('{text}' at {self.cursor_x},{self.cursor_y})")
        print(f"[LCD] Text: '{text}' at ({self.cursor_x}, {self.cursor_y})")
        # Simulate cursor advancement
        self.cursor_y += 8 * self.text_size
    
    def fillRect(self, x, y, w, h, color):
        self.buffer.append(f"FILL_RECT({x}, {y}, {w}, {h}, {hex(color)})")
        print(f"[LCD] Fill rectangle: ({x}, {y}) {w}x{h} color {hex(color)}")
    
    def fillCircle(self, x, y, radius, color):
        self.buffer.append(f"FILL_CIRCLE({x}, {y}, {radius}, {hex(color)})")
        print(f"[LCD] Fill circle: center ({x}, {y}) radius {radius} color {hex(color)}")
    
    def drawPixel(self, x, y, color):
        self.buffer.append(f"PIXEL({x}, {y}, {hex(color)})")
        print(f"[LCD] Pixel: ({x}, {y}) color {hex(color)}")
    
    def get_buffer(self):
        """Get the drawing command buffer for testing"""
        return self.buffer.copy()
    
    def clear_buffer(self):
        """Clear the drawing command buffer"""
        self.buffer.clear()

class MockButton:
    """Mock button for testing"""
    
    def __init__(self, name):
        self.name = name
        self.pressed = False
        self.was_pressed = False
        self.callback = None
        self._press_count = 0
    
    def wasPressed(self, callback=None):
        """Set or check if button was pressed"""
        if callback:
            self.callback = callback
            print(f"[BUTTON {self.name}] Callback registered")
            return
        
        if self.was_pressed:
            self.was_pressed = False
            return True
        return False
    
    def isPressed(self):
        """Check if button is currently pressed"""
        return self.pressed
    
    def simulate_press(self):
        """Simulate a button press for testing"""
        print(f"[BUTTON {self.name}] Simulated press")
        self.was_pressed = True
        self._press_count += 1
        if self.callback:
            # Simulate pin parameter that M5Stack callbacks expect
            MockPin = namedtuple('MockPin', ['value'])
            pin = MockPin(value=0)  # Pressed state
            self.callback(pin)
    
    def get_press_count(self):
        """Get number of times button was pressed (for testing)"""
        return self._press_count

# Create global instances
lcd = MockLCD()
buttonA = MockButton("A")
buttonB = MockButton("B") 
buttonC = MockButton("C")

def simulate_button_press(button_name):
    """Simulate pressing a button by name"""
    if button_name.upper() == "A":
        buttonA.simulate_press()
    elif button_name.upper() == "B":
        buttonB.simulate_press()
    elif button_name.upper() == "C":
        buttonC.simulate_press()
    else:
        print(f"Unknown button: {button_name}")

def get_display_state():
    """Get current display state for testing"""
    return {
        'rotation': lcd.rotation,
        'text_size': lcd.text_size,
        'text_color': lcd.text_color,
        'bg_color': lcd.bg_color,
        'cursor': (lcd.cursor_x, lcd.cursor_y),
        'commands': lcd.get_buffer()
    }

print("[M5Stack Simulator] Mock hardware initialized")
print("Available functions:")
print("  - simulate_button_press('A'|'B'|'C')")
print("  - get_display_state()")
print("  - lcd, buttonA, buttonB, buttonC objects")
