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
  libspdlog-dev

# Create a dedicated virtual environment for PlatformIO

USER vscode

# Create venv in home directory with proper permissions
RUN python3 -m venv /home/vscode/platformio-venv && \
    chmod +x /home/vscode/platformio-venv/bin/activate

# Install PlatformIO into the virtual environment
RUN /home/vscode/platformio-venv/bin/pip install platformio

# Add the venv bin directory to PATH
ENV PATH="/home/vscode/platformio-venv/bin:$PATH"
ENV PLATFORMIO_CORE_DIR="/home/vscode/.platformio"