# M5Stack Core Development Without Hardware

## What You Can Test on Linux/AMD64

### ✅ **Complete Testing Available**

1. **Application Logic**
   - All your Python/MicroPython code
   - Data processing, calculations
   - Control flow, loops, conditionals
   - Function calls and class methods

2. **Display Operations**
   - Text rendering (`lcd.print()`)
   - Graphics drawing (`fillRect`, `fillCircle`, `drawPixel`)
   - Color handling and constants
   - Screen clearing and rotation
   - Cursor positioning

3. **Button Interactions**
   - Button press callbacks
   - State management
   - Event handling logic

4. **Timing and Control Flow**
   - Main application loops
   - Sleep/delay timing
   - Counter logic
   - State machines

### ✅ **Partial Testing Available**

5. **Hardware Interfaces** (with mocks)
   - GPIO operations (can mock `machine` module)
   - I2C/SPI communication (can create simulators)
   - Sensor readings (can provide mock data)

6. **Network Operations**
   - WiFi connection logic (Unix MicroPython has `network`)
   - HTTP requests (`urequests` available)
   - Socket communication
   - API calls and data parsing

### ❌ **Cannot Test Without Hardware**

7. **Real Hardware Timing**
   - Exact performance characteristics
   - Real-world responsiveness
   - Hardware-specific quirks

8. **Physical Interactions**
   - Actual button feel and timing
   - Real display output quality
   - Hardware reliability

## Testing Commands

```bash
# Test main application with simulator
micropython main.py

# Test interactive mode
micropython main_test.py

# Test drawing demo
cd examples && micropython drawing_demo.py

# Run unit tests
micropython test_simulator.py

# Test individual components
micropython -c "
import sys; sys.path.append('lib')
from m5stack_simulator import *
lcd.clear()
lcd.print('Hello World')
simulate_button_press('A')
print(get_display_state())
"
```

## Development Workflow

1. **Write code** using real M5Stack APIs
2. **Test locally** with the simulator
3. **Debug and iterate** without hardware
4. **Upload to device** when ready
5. **Final testing** on real hardware

## What the Simulator Shows

- **Console output** of all display operations
- **Button press simulations** and callbacks
- **Visual representation** of what would appear on screen
- **Timing and sequence** of operations
- **Error detection** in your logic

## Example Output

```
[LCD] Screen cleared with color 0x0
[LCD] Text: 'M5Stack Core' at (10, 10)
[BUTTON A] Simulated press
[LCD] Fill rectangle: (10, 100) 200x20 color 0x0
[LCD] Text: 'Button A pressed!' at (10, 100)
```

This gives you a clear view of:
- What text appears where on screen
- What graphics are drawn
- When buttons are pressed
- How your app responds

## Benefits

- **Rapid development** - No hardware upload cycles
- **Easy debugging** - Full console output
- **Version control** - All changes trackable
- **Automated testing** - Can run tests in CI/CD
- **Collaborative development** - Others can test without hardware
