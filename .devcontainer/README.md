# M5Stack Development Container Setup

This devcontainer is configured for M5Stack development with PlatformIO.

## USB Device Sharing

To flash and debug your M5Stack device, you need to share the USB device with the container.

### Setup Steps:

1. **Connect your M5Stack** and find its device path on the host:
   ```bash
   # On the host machine
   ls /dev/tty*
   # Look for devices like /dev/ttyUSB0, /dev/ttyACM0, etc.
   ```

2. **Update devcontainer.json** to share the specific device:
   ```json
   "runArgs": [
     "--device=/dev/ttyUSB0:/dev/ttyUSB0"
   ],
   ```
   Replace `/dev/ttyUSB0` with your actual device path.

3. **Rebuild the devcontainer** to apply the changes.

4. **Verify the device** is available in the container:
   ```bash
   # In the container
   ls /dev/tty*
   ```

### Development Commands:

```bash
# Build for M5Stack
pio run -e m5stack-core-esp32-16M

# Upload to M5Stack
pio run -e m5stack-core-esp32-16M --target upload

# Monitor serial output
pio device monitor

# Build native simulator
pio run -e native

# Run native simulator
./.pio/build/native/program
```

### Helper Script:

Run the helper script for setup guidance:
```bash
./.devcontainer/m5stack-helper.sh
```

## Security Notes:

This setup uses minimal privileges by only sharing the specific USB device needed for the M5Stack, rather than mounting all of `/dev` or using privileged mode.
