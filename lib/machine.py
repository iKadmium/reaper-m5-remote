"""
Mock machine module for simulator
Provides basic machine functionality for testing
"""

import time

def freq(frequency=None):
    """Get or set CPU frequency"""
    if frequency is not None:
        print(f"[MACHINE] CPU frequency set to {frequency} Hz")
        return frequency
    return 240000000  # Mock ESP32 frequency

def reset():
    """Reset the machine"""
    print("[MACHINE] Machine reset requested")
    # In simulator, we can't actually reset
    
def unique_id():
    """Get unique machine ID"""
    return b'\x24\x0a\xc4\x12\x34\x56'

def idle():
    """Enter idle mode"""
    time.sleep(0.001)

# Pin class for compatibility
class Pin:
    IN = 0
    OUT = 1
    PULL_UP = 2
    
    def __init__(self, pin, mode=None, pull=None):
        self.pin = pin
        self.mode = mode
        self.pull = pull
        self._value = 0
        print(f"[MACHINE] Pin {pin} initialized (mode={mode}, pull={pull})")
    
    def value(self, val=None):
        if val is not None:
            self._value = val
            print(f"[MACHINE] Pin {self.pin} set to {val}")
        return self._value
    
    def on(self):
        self.value(1)
    
    def off(self):
        self.value(0)

# Timer class for compatibility  
class Timer:
    def __init__(self, timer_id):
        self.timer_id = timer_id
        self._callback = None
        self._period = 1000
        print(f"[MACHINE] Timer {timer_id} created")
    
    def init(self, period=None, mode=None, callback=None):
        if period:
            self._period = period
        if callback:
            self._callback = callback
        print(f"[MACHINE] Timer {self.timer_id} initialized (period={self._period}ms)")
    
    def deinit(self):
        print(f"[MACHINE] Timer {self.timer_id} deinitialized")

print("[MACHINE] Mock machine module loaded")
