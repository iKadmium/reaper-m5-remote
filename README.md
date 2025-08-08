# M5Stack Core MicroPython Project

This project is set up for developing MicroPython applications for the M5Stack Core device.

## Project Structure

```
.
├── main.py          # Main application entry point
├── boot.py          # Boot configuration script
├── lib/             # Custom libraries and modules
├── examples/        # Example scripts
└── README.md        # This file
```

## Hardware

- **Device**: M5Stack Core (ESP32-based)
- **Display**: 320x240 TFT LCD
- **Buttons**: A, B, C buttons
- **Connectivity**: WiFi, Bluetooth
- **Sensors**: Accelerometer, Gyroscope (depending on model)

## Getting Started

1. **Rebuild the dev container** to get Python and development tools installed
2. **Flash MicroPython firmware** to your M5Stack Core (if not already done)
3. **Upload files** to the device:
   ```bash
   ./dev_tools.sh /dev/ttyUSB0 upload
   ```
4. **Reset the device** to start the application:
   ```bash
   ./dev_tools.sh /dev/ttyUSB0 reset
   ```

## Development Environment

This project uses a dev container with:
- **MicroPython Unix port** for local testing
- **Python 3** for development tools
- **Development tools**: ampy, mpfshell, rshell
- **Serial tools**: screen, minicom

## File Upload

The project includes a helper script `dev_tools.sh` with commands:

```bash
# Upload all files to M5Stack
./dev_tools.sh /dev/ttyUSB0 upload

# Start serial monitor  
./dev_tools.sh /dev/ttyUSB0 monitor

# Reset the device
./dev_tools.sh /dev/ttyUSB0 reset

# Start REPL session
./dev_tools.sh /dev/ttyUSB0 repl

# List files on device
./dev_tools.sh /dev/ttyUSB0 list-files
```

You can also use the tools directly:
```bash
ampy --port /dev/ttyUSB0 put main.py
ampy --port /dev/ttyUSB0 put boot.py
ampy --port /dev/ttyUSB0 put -r lib
```

## Development

The main application is in `main.py` and demonstrates:
- LCD display usage
- Button handling
- Basic application loop

Modify `main.py` to implement your specific application logic.

## Libraries

The M5Stack libraries provide access to:
- `m5stack.lcd` - Display control
- `m5stack.buttonA/B/C` - Button handling
- `machine` - Hardware interfaces
- `network` - WiFi connectivity

## Troubleshooting

- Check serial connection: `screen /dev/ttyUSB0 115200` or similar
- Monitor output for error messages
- Use `machine.reset()` to restart the device
- Check available memory with `gc.mem_free()`
