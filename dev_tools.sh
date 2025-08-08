#!/bin/bash

# M5Stack Core MicroPython Development Scripts

DEVICE_PORT=${1:-/dev/ttyUSB0}
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

case "$2" in
    "upload")
        echo "Uploading files to M5Stack..."
        if check_tool "ampy" "adafruit-ampy"; then
            ampy --port $DEVICE_PORT put boot.py
            ampy --port $DEVICE_PORT put main.py
            if [ -d "lib" ]; then
                echo "Uploading lib directory..."
                ampy --port $DEVICE_PORT put lib
            fi
            echo "Files uploaded successfully!"
        fi
        ;;
    "monitor")
        echo "Starting serial monitor (Ctrl+A, K to exit screen)..."
        if command -v screen &> /dev/null; then
            screen $DEVICE_PORT $BAUD_RATE
        elif command -v minicom &> /dev/null; then
            minicom -D $DEVICE_PORT -b $BAUD_RATE
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
        pip3 install --user adafruit-ampy mpfshell rshell
        echo "Tools installed!"
        ;;
    "list-files")
        echo "Listing files on device..."
        if check_tool "ampy" "adafruit-ampy"; then
            ampy --port $DEVICE_PORT ls
        fi
        ;;
    *)
        echo "Usage: $0 [device_port] [command]"
        echo ""
        echo "Commands:"
        echo "  upload       - Upload boot.py, main.py and lib/ to device"
        echo "  monitor      - Start serial monitor"
        echo "  reset        - Reset the device"
        echo "  repl         - Start REPL session"
        echo "  install-tools- Install development tools"
        echo "  list-files   - List files on device"
        echo ""
        echo "Examples:"
        echo "  $0 /dev/ttyUSB0 upload"
        echo "  $0 /dev/ttyUSB0 monitor"
        echo "  $0 /dev/ttyUSB0 install-tools"
        ;;
esac
