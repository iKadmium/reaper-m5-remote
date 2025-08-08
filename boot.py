"""
Boot script for M5Stack Core
This file runs when the device boots up
"""

import gc
import machine
import micropython

# Enable garbage collection
gc.enable()

# Allocate emergency exception buffer
micropython.alloc_emergency_exception_buf(100)

# Optional: Set CPU frequency (240MHz is max for ESP32)
machine.freq(240000000)

print("M5Stack Core booting...")
print("Free memory:", gc.mem_free())
print("CPU frequency:", machine.freq(), "Hz")

# The main.py file will be imported automatically after boot.py
