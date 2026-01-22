import socket
import time
import sys

TARGET_HOST = "127.0.0.1"
TARGET_PORT = 8080

def send_payload(name, payload):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2.0)
        s.connect((TARGET_HOST, TARGET_PORT))
        s.send(payload)
        
        try:
            s.recv(4096)
        except socket.timeout:
            pass
        except ConnectionResetError:
            pass
            
        s.close()
        print(f"[PASS] {name}")
        return True
    except Exception as e:
        print(f"[FAIL] {name} - Error: {e}")
        return False

def health_check():
    valid_req = b"GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
    return send_payload("Health Check", valid_req)

def run_fuzz_suite():
    print(f"STARTING FUZZ ATTACK ON {TARGET_HOST}:{TARGET_PORT}")
    
    if not health_check():
        print("CRITICAL: Server is not running or already dead.")
        sys.exit(1)

    attacks = [
        ("Huge Header", b"GET / HTTP/1.1\r\nHost: localhost\r\n" + (b"X-Junk: " + b"A"*10000 + b"\r\n") + b"\r\n"),
        ("No Newlines", b"GET / HTTP/1.1Host: localhost"),
        ("Binary Garbage", b"\x00\xff\xfe\x01" * 100),
        ("Method Overflow", b"A"*1000 + b" / HTTP/1.1\r\nHost: localhost\r\n\r\n"),
        ("Negative Content-Length", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -50\r\n\r\nBody"),
        ("Slowloris Partial", b"GET / HTTP/1.1\r\nHost: localhost\r\n"),
        ("Double CL", b"POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\nHello"),
        ("UTF-8 Bombs", b"GET /" + b"\xe2\x98\x83"*500 + b" HTTP/1.1\r\n\r\n"),
    ]

    for name, payload in attacks:
        if not send_payload(name, payload):
            print("!!! SERVER CRASHED !!!")
            sys.exit(1)
        
        if not health_check():
            print(f"!!! SERVER DIED AFTER {name} !!!")
            sys.exit(1)
            
    print("FUZZ COMPLETE: ALL PASSED")

if __name__ == "__main__":
    run_fuzz_suite()
