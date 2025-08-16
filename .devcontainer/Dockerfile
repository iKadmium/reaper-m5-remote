# Dockerfile
FROM mcr.microsoft.com/devcontainers/cpp:bookworm

# Add your LVGL and simulator-specific packages
RUN sudo apt-get update && sudo apt-get install -y \
  libsdl2-dev \
  libsdl2-image-dev \
  libwayland-dev \
  python3 \
  python3-pip \
  python3-venv \
  libspdlog-dev \
  udev \
  usbutils \
  picocom \
  minicom \
  screen

# Create a dedicated virtual environment for PlatformIO

USER vscode

# Create venv in home directory with proper permissions
RUN python3 -m venv /home/vscode/platformio-venv && \
    chmod +x /home/vscode/platformio-venv/bin/activate

# Install PlatformIO into the virtual environment
RUN /home/vscode/platformio-venv/bin/pip install platformio

# Copy platformio.ini to pre-install dependencies
COPY platformio.ini /tmp/platformio.ini

# Pre-install ESP32 platform and dependencies to cache them
WORKDIR /tmp
RUN /home/vscode/platformio-venv/bin/pio platform install espressif32@6.12.0 && \
    mkdir -p src && \
    echo "// dummy file" > src/main.cpp && \
    /home/vscode/platformio-venv/bin/pio pkg install -e m5stack-core-esp32-16M && \
    /home/vscode/platformio-venv/bin/pio pkg install -e native

# Clean up temporary files
RUN sudo rm -rf /tmp/platformio.ini /tmp/src

# Add the venv bin directory to PATH
ENV PATH="/home/vscode/platformio-venv/bin:$PATH"
ENV PLATFORMIO_CORE_DIR="/home/vscode/.platformio"