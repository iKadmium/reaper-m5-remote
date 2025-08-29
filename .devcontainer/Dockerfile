FROM micropython/unix:latest

# Switch to root to install packages
USER root

# Install Python, pip, development tools, and GUI libraries
RUN apt-get update && apt-get install -y \
    sudo \
    coreutils \
    python3 \
    python3-pip \
    python3-venv \
    python3-tk \
    git \
    curl \
    screen \
    minicom \
    && rm -rf /var/lib/apt/lists/*

# Create vscode user and add to sudo group
RUN useradd vscode -m -s /bin/bash && \
    usermod -aG sudo vscode && \
    echo 'vscode ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Install MicroPython development tools globally
RUN pip3 install --break-system-packages adafruit-ampy mpfshell rshell esptool

# Switch to vscode user
USER vscode

ENV PATH=/home/vscode/.local/bin:$PATH