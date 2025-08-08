"""
Test script for Reaper API functionality
"""

import sys
sys.path.append('lib')

from reaper_client import ReaperClient

def test_reaper_client():
    """Test the Reaper client with mock responses"""
    print("=== Testing Reaper Client ===")
    
    # Create client with mock URL for testing
    client = ReaperClient("http://localhost:8080/_")
    
    print(f"Base URL: {client.base_url}")
    print(f"Script ID: {client.script_id}")
    
    # Test connection (will fail in simulator, but that's expected)
    print("\n--- Testing Connection ---")
    result = client.test_connection()
    print(f"Connection test result: {result}")
    
    # Test getting state variable (will fail without real Reaper)
    print("\n--- Testing State Variable ---")
    script_id = client.get_state_var("ScriptActionId")
    print(f"Script ID from state: {script_id}")
    
    # Test finding script ID
    print("\n--- Testing Script ID Discovery ---")
    found = client.find_script_id()
    print(f"Script ID found: {found}")
    print(f"Client script ID: {client.script_id}")
    
    # Test getting status
    print("\n--- Testing Status Retrieval ---")
    status = client.get_current_status()
    print(f"Status: {status}")
    
    print("\n=== Test Complete ===")
    print("Note: Connection failures are expected without a real Reaper instance")

if __name__ == "__main__":
    test_reaper_client()
