#!/bin/bash

# M5Stack Device Helper Script

echo "=== M5Stack Device Setup ==="

echo "1. First, connect your M5Stack device and find its device path on the host:"
echo "   Run on HOST: ls /dev/tty*"
echo "   Look for devices like /dev/ttyUSB0, /dev/ttyACM0, etc."
echo ""

echo "2. Update your devcontainer.json to share the specific device:"
echo "   Uncomment and modify the runArgs section:"
echo '   "runArgs": ['
echo '     "--device=/dev/ttyUSB0:/dev/ttyUSB0"'
echo '   ],'
echo ""

echo "3. Rebuild the devcontainer to apply changes"
echo ""

echo "4. In the container, you can then:"
echo "   - Build: pio run -e m5stack-core-esp32-16M"
echo "   - Upload: pio run -e m5stack-core-esp32-16M --target upload"
echo "   - Monitor: pio device monitor"
echo ""

echo "Current container serial devices:"
ls -la /dev/tty* 2>/dev/null | grep -E "(USB|ACM)" || echo "No USB serial devices found in container"

echo ""
echo "If you see your device above, you're ready to flash!"
echo "If not, make sure you've updated devcontainer.json and rebuilt the container."
