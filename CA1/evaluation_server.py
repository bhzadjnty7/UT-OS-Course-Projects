#!/usr/bin/env python3
"""
Code Evaluation Server for Programming Contest.
This server receives code from the main C++ server and evaluates it.
"""

import socket
import threading
import sys
import io
import traceback
from contextlib import redirect_stdout, redirect_stderr

# Server configuration
HOST = '127.0.0.1'
PORT = 65432
BUFFER_SIZE = 4096

# Test cases for each problem
TEST_CASES = {
    'add_numbers': [
        {'input': (1, 2), 'expected': 3},
        {'input': (-1, 5), 'expected': 4},
        {'input': (0, 0), 'expected': 0},
        {'input': (100, 200), 'expected': 300},
        {'input': (-10, -20), 'expected': -30}
    ],
    'reverse_string': [
        {'input': ('hello',), 'expected': 'olleh'},
        {'input': ('',), 'expected': ''},
        {'input': ('a',), 'expected': 'a'},
        {'input': ('12345',), 'expected': '54321'},
        {'input': ('racecar',), 'expected': 'racecar'}
    ],
    'is_palindrome': [
        {'input': ('racecar',), 'expected': True},
        {'input': ('hello',), 'expected': False},
        {'input': ('',), 'expected': True},
        {'input': ('a',), 'expected': True},
        {'input': ('12321',), 'expected': True}
    ]
}

def evaluate_code(problem_id, code):
    """
    Evaluate the submitted code against test cases.
    
    Args:
        problem_id (str): The problem identifier.
        code (str): The Python code to evaluate.
    
    Returns:
        bool: True if all test cases pass, False otherwise.
    """
    print(f"Evaluating code for problem: {problem_id}")
    print(f"Code:\n{code}")
    
    # Check if problem exists
    if problem_id not in TEST_CASES:
        print(f"Unknown problem: {problem_id}")
        return False
    
    # Prepare environment
    namespace = {}
    
    try:
        # Execute the code
        exec(code, namespace)
    except Exception as e:
        print(f"Code execution error: {e}")
        traceback.print_exc()
        return False
    
    # Check if required function exists
    if problem_id not in namespace:
        print(f"Function '{problem_id}' not found in submitted code")
        return False
    
    # Run test cases
    test_cases = TEST_CASES[problem_id]
    function = namespace[problem_id]
    
    for i, test_case in enumerate(test_cases):
        inputs = test_case['input']
        expected = test_case['expected']
        
        try:
            # Capture stdout and stderr
            stdout = io.StringIO()
            stderr = io.StringIO()
            
            with redirect_stdout(stdout), redirect_stderr(stderr):
                result = function(*inputs)
            
            if result != expected:
                print(f"Test case {i+1} failed. Expected {expected}, got {result}")
                return False
        except Exception as e:
            print(f"Error during test case {i+1}: {e}")
            traceback.print_exc()
            return False
    
    print("All test cases passed!")
    return True

def handle_client(conn, addr):
    """
    Handle a client connection.
    
    Args:
        conn: The client connection.
        addr: The client address.
    """
    print(f"Connected by {addr}")
    
    # Receive data
    data = conn.recv(BUFFER_SIZE).decode('utf-8')
    
    # Parse data
    lines = data.strip().split('\n', 1)
    if len(lines) != 2:
        print("Invalid request format")
        conn.sendall(b"FAIL")
        conn.close()
        return
    
    problem_id = lines[0].strip()
    code = lines[1]
    
    # Evaluate code
    result = evaluate_code(problem_id, code)
    
    # Send result
    if result:
        conn.sendall(b"PASS")
    else:
        conn.sendall(b"FAIL")
    
    # Close connection
    conn.close()
    print(f"Connection closed with {addr}")

def main():
    """Main server function."""
    # Create server socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        # Allow address reuse
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Bind to port
        server_socket.bind((HOST, PORT))
        
        # Listen for connections
        server_socket.listen()
        
        print(f"Evaluation server listening on {HOST}:{PORT}")
        
        try:
            while True:
                # Accept connection
                conn, addr = server_socket.accept()
                
                # Create thread to handle client
                client_thread = threading.Thread(target=handle_client, args=(conn, addr))
                client_thread.daemon = True
                client_thread.start()
        except KeyboardInterrupt:
            print("Server shutting down...")
            sys.exit(0)

if __name__ == "__main__":
    main() 
 