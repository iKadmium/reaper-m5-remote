#!/bin/bash

# M5Stack Core MicroPython Development Scripts

# Parse arguments - handle optional device port
if [[ $1 == /dev/* ]] || [[ $1 == COM* ]]; then
    # First argument is a device port
    DEVICE_PORT=$1
    COMMAND=$2
else
    # First argument is a command, use default port
    DEVICE_PORT="/dev/ttyACM0"
    COMMAND=$1
fi

BAUD_RATE=115200

echo "M5Stack Core Development Helper"
echo "Device port: $DEVICE_PORT"
echo ""

# Check if tools are installed
check_tool() {
    if ! command -v $1 &> /dev/null; then
        echo "$1 not found. Install with: pip3 install $2"
        return 1
    fi
    return 0
}

case "$COMMAND" in
    "check-micropython")
        echo "Checking if MicroPython is running on device..."
        if check_tool "ampy" "adafruit-ampy"; then
            timeout 5 ampy --port $DEVICE_PORT ls > /dev/null 2>&1
            if [ $? -eq 0 ]; then
                echo "✓ MicroPython is running and responsive"
            else
                echo "✗ Device is not running MicroPython or not responsive"
                echo "  - If you see continuous output, the device has native firmware"
                echo "  - Use 'erase-flash' and 'flash-micropython' to install MicroPython"
            fi
        fi
        ;;
    "upload")
        echo "Uploading files to M5Stack..."
        if check_tool "ampy" "adafruit-ampy"; then
            ampy --port $DEVICE_PORT put boot.py
            ampy --port $DEVICE_PORT put config.py
            ampy --port $DEVICE_PORT put main.py
            if [ -d "lib" ]; then
                echo "Uploading lib directory..."
                ampy --port $DEVICE_PORT put lib
            fi
            echo "Files uploaded successfully!"
        fi
        ;;
    "monitor")
        echo "Starting serial monitor (Ctrl+A, X to exit minicom)..."
        if command -v minicom &> /dev/null; then
            minicom -D $DEVICE_PORT -b $BAUD_RATE
        elif command -v screen &> /dev/null; then
            screen $DEVICE_PORT $BAUD_RATE
        else
            echo "No serial monitor found. Install screen or minicom."
        fi
        ;;
    "reset")
        echo "Resetting M5Stack..."
        if check_tool "ampy" "adafruit-ampy"; then
            echo "import machine; machine.reset()" | ampy --port $DEVICE_PORT run
        fi
        ;;
    "repl")
        echo "Starting REPL session..."
        if check_tool "mpfshell" "mpfshell"; then
            mpfshell -c "open $DEVICE_PORT; repl"
        fi
        ;;
    "install-tools")
        echo "Installing development tools..."
        pip3 install --user adafruit-ampy mpfshell rshell esptool
        echo "Tools installed!"
        ;;
    "list-files")
        echo "Listing files on device..."
        if check_tool "ampy" "adafruit-ampy"; then
            ampy --port $DEVICE_PORT ls
        fi
        ;;
    "check-device")
        echo "Checking device communication..."
        echo "Attempting to read from device for 3 seconds..."
        timeout 3 cat $DEVICE_PORT
        echo ""
        echo "If you see continuous output above, the device is running native firmware."
        echo "Use 'check-micropython' to test MicroPython connectivity."
        ;;
    "stop-program")
        echo "Sending interrupt signal to stop running program..."
        echo "Press Ctrl+C to send interrupt to device..."
        echo -e "\x03" > $DEVICE_PORT
        sleep 1
        echo "Interrupt sent. Try 'check-micropython' to test connectivity."
        ;;
    "erase-flash")
        echo "Erasing device flash..."
        if check_tool "esptool" "esptool"; then
            esptool --port $DEVICE_PORT erase-flash
            echo "Flash erased. Use 'flash-micropython' to install MicroPython firmware."
        fi
        ;;
    "flash-micropython")
        echo "Flashing MicroPython firmware to M5Stack Core..."
        echo "Note: You need to download the MicroPython firmware first."
        echo "Download from: https://micropython.org/download/m5stack-core/"
        echo ""
        if [ -f "firmware/micropython_firmware.bin" ]; then
            if check_tool "esptool" "esptool"; then
                echo "Flashing firmware..."
                esptool --chip esp32 --port $DEVICE_PORT --baud 460800 write-flash -z 0x1000 firmware/micropython_firmware.bin
                echo "MicroPython firmware flashed! Device should now be ready for MicroPython development."
            fi
        else
            echo "Error: firmware/micropython_firmware.bin not found in current directory."
            echo "Please download the M5Stack Core MicroPython firmware and save it as 'firmware/micropython_firmware.bin'"
        fi
        ;;
    *)
        echo "Usage: $0 [device_port] <command>"
        echo ""
        echo "Commands:"
        echo "  check-micropython - Check if device is running MicroPython"
        echo "  upload       - Upload boot.py, main.py and lib/ to device"
        echo "  monitor      - Start serial monitor"
        echo "  reset        - Reset the device"
        echo "  repl         - Start REPL session"
        echo "  install-tools- Install development tools"
        echo "  list-files   - List files on device"
        echo "  check-device - Show device communication status"
        echo "  stop-program - Try to stop running program (Ctrl+C)"
        echo "  erase-flash  - Erase device flash completely"
        echo "  flash-micropython - Flash MicroPython firmware to device"
        echo ""
        echo "Default device port: /dev/ttyACM0"
        echo ""
        echo "Examples:"
        echo "  $0 check-micropython   (check if MicroPython is running)"
        echo "  $0 upload              (uses default port /dev/ttyACM0)"
        echo "  $0 monitor             (uses default port /dev/ttyACM0)"
        echo "  $0 /dev/ttyUSB0 upload (uses custom port)"
        echo "  $0 erase-flash         (erase flash to remove native firmware)"
        echo "  $0 flash-micropython   (flash MicroPython firmware)"
        echo "  $0 install-tools"
        ;;
esac
