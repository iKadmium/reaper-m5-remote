"""
M5Stack Core GUI Simulator using tkinter
Provides a visual simulation of the M5Stack hardware with proper display and buttons
"""

import tkinter as tk
from tkinter import ttk
import time
import threading
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

def rgb565_to_rgb(color):
    """Convert RGB565 color to RGB tuple"""
    r = (color >> 11) & 0x1F
    g = (color >> 5) & 0x3F
    b = color & 0x1F
    
    # Scale to 8-bit
    r = (r * 255) // 31
    g = (g * 255) // 63
    b = (b * 255) // 31
    
    return f"#{r:02x}{g:02x}{b:02x}"

class M5StackGUI:
    """Main GUI window for M5Stack simulator"""
    
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("M5Stack Core Simulator")
        self.root.geometry("400x350")
        self.root.resizable(False, False)
        
        # Display canvas (320x240)
        self.canvas = tk.Canvas(self.root, width=320, height=240, bg='black')
        self.canvas.pack(pady=10)
        
        # Button frame
        button_frame = tk.Frame(self.root)
        button_frame.pack(pady=10)
        
        # Create buttons
        self.button_a = tk.Button(button_frame, text="A", width=8, height=2, 
                                 command=lambda: self._button_pressed('A'))
        self.button_b = tk.Button(button_frame, text="B", width=8, height=2,
                                 command=lambda: self._button_pressed('B'))
        self.button_c = tk.Button(button_frame, text="C", width=8, height=2,
                                 command=lambda: self._button_pressed('C'))
        
        self.button_a.pack(side=tk.LEFT, padx=5)
        self.button_b.pack(side=tk.LEFT, padx=5)
        self.button_c.pack(side=tk.LEFT, padx=5)
        
        # Status label
        self.status_label = tk.Label(self.root, text="M5Stack Simulator Ready", 
                                    fg="blue", font=("Arial", 10))
        self.status_label.pack(pady=5)
        
        # Display state
        self.rotation = 0
        self.text_size = 1
        self.text_color = Colors.WHITE
        self.bg_color = Colors.BLACK
        self.cursor_x = 0
        self.cursor_y = 0
        
        # Button callbacks
        self.button_callbacks = {'A': None, 'B': None, 'C': None}
        self.button_states = {'A': False, 'B': False, 'C': False}
        
        # Threading support
        self.update_queue = []
        self._running = True
        
        # Start update thread
        self.update_thread = threading.Thread(target=self._update_loop, daemon=True)
        self.update_thread.start()
        
        # Handle window close
        self.root.protocol("WM_DELETE_WINDOW", self._on_closing)
    
    def _on_closing(self):
        """Handle window close event"""
        self._running = False
        self.root.quit()
        self.root.destroy()
    
    def _update_loop(self):
        """Background update loop for thread-safe GUI updates"""
        while self._running:
            if self.update_queue:
                try:
                    self.root.after_idle(self._process_updates)
                except tk.TclError:
                    break
            time.sleep(0.01)
    
    def _process_updates(self):
        """Process queued GUI updates"""
        while self.update_queue:
            try:
                update_func = self.update_queue.pop(0)
                update_func()
            except (IndexError, tk.TclError):
                break
    
    def _button_pressed(self, button_name):
        """Handle button press"""
        print(f"[BUTTON {button_name}] Pressed")
        self.button_states[button_name] = True
        
        # Call registered callback if exists
        callback = self.button_callbacks.get(button_name)
        if callback:
            # Simulate pin parameter that M5Stack callbacks expect
            MockPin = namedtuple('MockPin', ['value'])
            pin = MockPin(value=0)  # Pressed state
            callback(pin)
    
    def queue_update(self, func):
        """Queue a GUI update for thread-safe execution"""
        self.update_queue.append(func)
    
    def run(self):
        """Start the GUI main loop"""
        try:
            self.root.mainloop()
        except KeyboardInterrupt:
            self._on_closing()

class MockLCD:
    """Mock LCD display with GUI visualization"""
    
    def __init__(self, gui):
        self.gui = gui
        self.width = 320
        self.height = 240
        
        # Add color constants as attributes
        self.BLACK = Colors.BLACK
        self.WHITE = Colors.WHITE
        self.RED = Colors.RED
        self.GREEN = Colors.GREEN
        self.BLUE = Colors.BLUE
        self.YELLOW = Colors.YELLOW
        self.CYAN = Colors.CYAN
        self.MAGENTA = Colors.MAGENTA
    
    @property
    def text_size(self):
        return self.gui.text_size
    
    @property 
    def text_color(self):
        return self.gui.text_color
    
    @property
    def bg_color(self):
        return self.gui.bg_color
    
    @property
    def cursor_x(self):
        return self.gui.cursor_x
    
    @property
    def cursor_y(self):
        return self.gui.cursor_y
    
    @property
    def rotation(self):
        return self.gui.rotation
    
    def clear(self, color=None):
        if color is None:
            color = Colors.BLACK
        
        def update():
            bg_color = rgb565_to_rgb(color)
            self.gui.canvas.configure(bg=bg_color)
            self.gui.canvas.delete("all")
        
        self.gui.queue_update(update)
        print(f"[LCD] Screen cleared with color {hex(color)}")
    
    def setRotation(self, rotation):
        self.gui.rotation = rotation
        print(f"[LCD] Rotation set to {rotation}")
    
    def setTextColor(self, color, bg_color=None):
        self.gui.text_color = color
        if bg_color is not None:
            self.gui.bg_color = bg_color
        print(f"[LCD] Text color: {hex(color)}, background: {hex(bg_color) if bg_color is not None else 'transparent'}")
    
    def setTextSize(self, size):
        self.gui.text_size = size
        print(f"[LCD] Text size set to {size}")
    
    def setCursor(self, x, y):
        self.gui.cursor_x = x
        self.gui.cursor_y = y
        print(f"[LCD] Cursor set to ({x}, {y})")
    
    def print(self, text):
        def update():
            color = rgb565_to_rgb(self.gui.text_color)
            font_size = 8 * self.gui.text_size
            font = ("Courier", font_size)
            
            self.gui.canvas.create_text(
                self.gui.cursor_x, self.gui.cursor_y,
                text=str(text), fill=color, font=font, anchor="nw"
            )
            
            # Advance cursor
            self.gui.cursor_y += font_size + 2
        
        self.gui.queue_update(update)
        print(f"[LCD] Text: '{text}' at ({self.gui.cursor_x}, {self.gui.cursor_y})")
    
    def fillRect(self, x, y, w, h, color):
        def update():
            fill_color = rgb565_to_rgb(color)
            self.gui.canvas.create_rectangle(
                x, y, x + w, y + h,
                fill=fill_color, outline=""
            )
        
        self.gui.queue_update(update)
        print(f"[LCD] Fill rectangle: ({x}, {y}) {w}x{h} color {hex(color)}")
    
    def fillCircle(self, x, y, radius, color):
        def update():
            fill_color = rgb565_to_rgb(color)
            self.gui.canvas.create_oval(
                x - radius, y - radius, x + radius, y + radius,
                fill=fill_color, outline=""
            )
        
        self.gui.queue_update(update)
        print(f"[LCD] Fill circle: center ({x}, {y}) radius {radius} color {hex(color)}")
    
    def drawPixel(self, x, y, color):
        def update():
            pixel_color = rgb565_to_rgb(color)
            self.gui.canvas.create_rectangle(
                x, y, x + 1, y + 1,
                fill=pixel_color, outline=""
            )
        
        self.gui.queue_update(update)
        print(f"[LCD] Pixel: ({x}, {y}) color {hex(color)}")

class MockButton:
    """Mock button with GUI integration"""
    
    def __init__(self, name, gui):
        self.name = name
        self.gui = gui
        self.callback = None
        self._press_count = 0
    
    def wasPressed(self, callback=None):
        """Set or check if button was pressed"""
        if callback:
            self.callback = callback
            self.gui.button_callbacks[self.name] = callback
            print(f"[BUTTON {self.name}] Callback registered")
            return
        
        # Check if button was pressed and reset state
        if self.gui.button_states[self.name]:
            self.gui.button_states[self.name] = False
            return True
        return False
    
    def isPressed(self):
        """Check if button is currently pressed"""
        return self.gui.button_states[self.name]
    
    def simulate_press(self):
        """Simulate a button press programmatically"""
        self.gui._button_pressed(self.name)
        self._press_count += 1
    
    def get_press_count(self):
        """Get number of times button was pressed (for testing)"""
        return self._press_count

# Global GUI instance (created when needed)
_gui_instance = None
_gui_thread = None

def get_gui():
    """Get or create the GUI instance"""
    global _gui_instance, _gui_thread
    
    if _gui_instance is None:
        _gui_instance = M5StackGUI()
        
        # Start GUI in separate thread to avoid blocking
        def run_gui():
            _gui_instance.run()
        
        _gui_thread = threading.Thread(target=run_gui, daemon=True)
        _gui_thread.start()
        
        # Give GUI time to initialize
        time.sleep(0.5)
    
    return _gui_instance

# Create global instances
gui = get_gui()
lcd = MockLCD(gui)
buttonA = MockButton("A", gui)
buttonB = MockButton("B", gui)
buttonC = MockButton("C", gui)

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
        'cursor': (lcd.cursor_x, lcd.cursor_y)
    }

print("[M5Stack GUI Simulator] Visual simulator initialized")
print("Available functions:")
print("  - simulate_button_press('A'|'B'|'C')")
print("  - get_display_state()")
print("  - lcd, buttonA, buttonB, buttonC objects")
print("  - GUI window should be visible")
