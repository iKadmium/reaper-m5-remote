FROM docker.io/micropython/unix:latest

# Switch to root to install packages
USER root

# Install Python, pip, and development tools
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    git \
    curl \
    screen \
    minicom \
    && rm -rf /var/lib/apt/lists/*

# Create vscode user
RUN useradd vscode -m -s /bin/bash

# Install MicroPython development tools globally
RUN pip3 install --break-system-packages adafruit-ampy mpfshell rshell

# Switch to vscode user
USER vscode