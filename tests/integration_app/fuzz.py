import socket
import time
import sys

TARGET_HOST = "127.0.0.1"
TARGET_PORT = 8080

def send_payload(name, payload, expected_status=None):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2.0)
        s.connect((TARGET_HOST, TARGET_PORT))
        s.send(payload)
        
        response = b""
        try:
            response = s.recv(4096)
        except socket.timeout:
            pass
        except ConnectionResetError:
            pass
            
        s.close()

        if expected_status:
            status_line = response.split(b"\r\n")[0].decode(errors='ignore')
            if str(expected_status) not in status_line:
                print(f"[FAIL] {name} - Expected {expected_status}, got: {status_line}")
                return False
            print(f"[PASS] {name} -> {status_line}")
        else:
            print(f"[PASS] {name} (No Crash)")

        return True
    except Exception as e:
        print(f"[FAIL] {name} - Error: {e}")
        return False

def health_check():
    valid_req = b"GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
    return send_payload("Health Check", valid_req, 200)

def run_fuzz_suite():
    print(f"STARTING FUZZ ATTACK ON {TARGET_HOST}:{TARGET_PORT}")
    
    if not health_check():
        print("CRITICAL: Server is not running or already dead.")
        sys.exit(1)

    attacks = [
        # --- PARSER STRESS ---
        ("Huge Header", b"GET / HTTP/1.1\r\nHost: localhost\r\n" + (b"X-Junk: " + b"A"*10000 + b"\r\n") + b"\r\n", 400),
        ("No Newlines", b"GET / HTTP/1.1Host: localhost", None), # Might close connection or 400
        ("Binary Garbage", b"\x00\xff\xfe\x01" * 100, None),
        ("Method Overflow", b"A"*1000 + b" / HTTP/1.1\r\nHost: localhost\r\n\r\n", 404),
        ("Negative Content-Length", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -50\r\n\r\nBody", 400),
        ("Slowloris Partial", b"GET / HTTP/1.1\r\nHost: localhost\r\n", None),
        ("Double CL", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\nHello", 400),
        ("UTF-8 Bombs", b"GET /" + b"\xe2\x98\x83"*500 + b" HTTP/1.1\r\n\r\n", 404),
        
        # --- LOGIC ATTACKS ---
        ("Type Poisoning (String -> Int)", b"POST /test/strict-json HTTP/1.1\r\nHost: localhost\r\nContent-Length: 20\r\nContent-Type: application/json\r\n\r\n{\"val\": \"not_an_int\"}", 400),
        ("Malformed URL Encoding", b"GET /test/%G1%G2 HTTP/1.1\r\nHost: localhost\r\n\r\n", 404),
        
        # --- RESILIENCE CHECKS ---
        ("Circuit Breaker Trip", b"GET /test/circuit-breaker HTTP/1.1\r\nHost: localhost\r\n\r\n", 503),
    ]

    for name, payload, status in attacks:
        if not send_payload(name, payload, status):
            print("!!! TEST FAILED OR SERVER CRASHED !!!")
            sys.exit(1)
        
        if not health_check():
            print(f"!!! SERVER DIED AFTER {name} !!!")
            sys.exit(1)
            
    print("FUZZ COMPLETE: ALL PASSED")

if __name__ == "__main__":
    run_fuzz_suite()
