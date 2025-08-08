"""
Unit tests for M5Stack application
"""

import sys
sys.path.append('lib')

from m5stack_simulator import lcd, buttonA, buttonB, buttonC, simulate_button_press, get_display_state

def test_lcd_basic():
    """Test basic LCD operations"""
    print("Testing LCD basic operations...")
    
    # Clear the buffer
    lcd.clear_buffer()
    
    # Test clear
    lcd.clear()
    assert len(lcd.get_buffer()) == 1
    assert "CLEAR" in lcd.get_buffer()[0]
    
    # Test text operations
    lcd.setTextSize(2)
    lcd.setTextColor(lcd.WHITE, lcd.BLACK)
    lcd.setCursor(10, 20)
    lcd.print("Hello World")
    
    state = get_display_state()
    assert state['text_size'] == 2
    assert state['cursor'] == (10, 20)
    
    print("✓ LCD basic operations passed")

def test_buttons():
    """Test button functionality"""
    print("Testing button functionality...")
    
    # Reset press counts
    buttonA._press_count = 0
    buttonB._press_count = 0
    buttonC._press_count = 0
    
    # Test button presses
    simulate_button_press('A')
    simulate_button_press('B')
    simulate_button_press('C')
    
    assert buttonA.get_press_count() == 1
    assert buttonB.get_press_count() == 1
    assert buttonC.get_press_count() == 1
    
    print("✓ Button functionality passed")

def test_drawing():
    """Test drawing operations"""
    print("Testing drawing operations...")
    
    lcd.clear_buffer()
    
    # Test rectangle
    lcd.fillRect(10, 10, 50, 30, lcd.RED)
    
    # Test circle
    lcd.fillCircle(100, 100, 25, lcd.BLUE)
    
    # Test pixel
    lcd.drawPixel(200, 200, lcd.GREEN)
    
    buffer = lcd.get_buffer()
    assert len(buffer) == 3
    assert "FILL_RECT" in buffer[0]
    assert "FILL_CIRCLE" in buffer[1]
    assert "PIXEL" in buffer[2]
    
    print("✓ Drawing operations passed")

def test_colors():
    """Test color constants"""
    print("Testing color constants...")
    
    assert lcd.BLACK == 0x0000
    assert lcd.WHITE == 0xFFFF
    assert lcd.RED == 0xF800
    assert lcd.GREEN == 0x07E0
    assert lcd.BLUE == 0x001F
    
    print("✓ Color constants passed")

def run_all_tests():
    """Run all tests"""
    print("=== Running M5Stack Simulator Tests ===\n")
    
    try:
        test_lcd_basic()
        test_buttons()
        test_drawing()
        test_colors()
        
        print("\n✅ All tests passed!")
        return True
        
    except AssertionError as e:
        print(f"\n❌ Test failed: {e}")
        return False
    except Exception as e:
        print(f"\n💥 Test error: {e}")
        return False

if __name__ == "__main__":
    run_all_tests()
