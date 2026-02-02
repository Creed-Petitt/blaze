#!/bin/bash
set -e

export PATH=$PATH:/usr/bin:/bin:/usr/sbin:/sbin

# Configuration
SERVER_BIN="./build/tests/integration_app/blaze_integration_app"
HOST="http://127.0.0.1:8080"

# Smart Binary Lookup
WRK_BIN=$(command -v wrk || echo "/usr/bin/wrk")
GREP_BIN=$(command -v grep || echo "/usr/bin/grep")
AWK_BIN=$(command -v awk || echo "/usr/bin/awk")

# Lua Script Generation (On-the-fly)
LUA_POST="/tmp/blaze_post.lua"
LUA_LOGIN="/tmp/blaze_login.lua"
LUA_USER="/tmp/blaze_user.lua"
LUA_INT="/tmp/blaze_int.lua"

generate_lua() {
    echo 'wrk.method = "POST"; wrk.body = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]"; wrk.headers["Content-Type"] = "application/json"' > $LUA_POST
    echo 'wrk.method = "POST"; wrk.body = "{\"username\": \"admin\", \"password\": \"password\"}"; wrk.headers["Content-Type"] = "application/json"' > $LUA_LOGIN
    echo 'wrk.method = "POST"; wrk.body = "{\"id\": 1, \"name\": \"BenchmarkUser\"}"; wrk.headers["Content-Type"] = "application/json"' > $LUA_USER
    echo 'wrk.method = "POST"; wrk.body = "123"; wrk.headers["Content-Type"] = "application/json"' > $LUA_INT
}

# Default Mode (Stress)
CONNS=100
THREADS=4
DURATION=5s
MODE="STRESS"

# Parse Arguments
if [ "$1" == "--ci" ]; then
    CONNS=10
    THREADS=2
    DURATION=3s
    MODE="CI"
fi

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== Blaze Framework Benchmark Suite ($MODE Mode) ===${NC}"

# 0. Cleanup Zombies
pkill -9 -f "blaze_integration_app" || true
fuser -k 8080/tcp || true 2>/dev/null

# Generate Lua scripts
generate_lua

# 1. Startup & Health Check
if pgrep -f "blaze_integration_app" > /dev/null; then
    echo "Server already running. Using existing instance."
    EXISTING_SERVER=1
else
    echo "Starting server..."
    if [ ! -f "$SERVER_BIN" ]; then
        echo -e "${RED}Error: Server binary not found at $SERVER_BIN${NC}"
        exit 1
    fi
    $SERVER_BIN &
    SERVER_PID=$!
    EXISTING_SERVER=0
    
    # Wait for health check
    echo "Waiting for healthy status..."
    for i in {1..30}; do
        if curl -s "$HOST/health" > /dev/null; then
            echo -e "${GREEN}Server is UP!${NC}"
            break
        fi
        sleep 0.5
    done
fi

# 2. Sanity Check (Ping Every Route)
echo -e "\n${BLUE}--- Phase 1: Sanity Check (Ping) ---${NC}"
ROUTES=(
    "/health"
    "/modern-json"
    "/locator"
    "/openapi.json"
    "/docs"
)

FAILED=0
for route in "${ROUTES[@]}"; do
    HTTP_CODE=$(curl -o /dev/null -s -w "%{http_code}" "$HOST$route" || echo "CURL_FAIL")
    if [ "$HTTP_CODE" == "200" ]; then
        echo -e "[PASS] $route ($HTTP_CODE)"
    else
        echo -e "${RED}[FAIL] $route returned '$HTTP_CODE'${NC}"
        FAILED=1
    fi
done

if [ "$FAILED" -eq 1 ]; then
    echo -e "${RED}Sanity Check Failed! Aborting benchmarks.${NC}"
    if [ "$EXISTING_SERVER" -eq "0" ]; then kill $SERVER_PID; fi
    exit 1
fi

# 3. Performance Benchmarks
echo -e "\n${BLUE}--- Phase 2: Performance Stress ($MODE) ---${NC}"
echo -e "| Endpoint | Requests/Sec | Latency (Avg) |"
echo -e "|----------|--------------|---------------|"

run_bench() {
    NAME=$1
    PATH=$2
    SCRIPT=$3
    
    if [ -n "$SCRIPT" ]; then
        OUTPUT=$($WRK_BIN -t$THREADS -c$CONNS -d$DURATION -s "$SCRIPT" "$HOST$PATH")
    else
        OUTPUT=$($WRK_BIN -t$THREADS -c$CONNS -d$DURATION "$HOST$PATH")
    fi
    
    RPS=$(echo "$OUTPUT" | $GREP_BIN "Requests/sec" | $AWK_BIN '{print $2}')
    LAT=$(echo "$OUTPUT" | $GREP_BIN "Latency" | $AWK_BIN '{print $2}')
    
    echo -e "| $NAME | $RPS | $LAT |"
}

# Run Suite
run_bench "/health" "/health" ""
run_bench "/modern-api" "/modern-api" "$LUA_POST"
run_bench "/test/strict-json" "/test/strict-json" "$LUA_INT"
run_bench "/locator" "/locator" ""
run_bench "/abstract" "/abstract" ""
run_bench "/users" "/users" ""
run_bench "/login" "/login" "$LUA_LOGIN"

# Cleanup
if [ "$EXISTING_SERVER" -eq "0" ]; then
    echo -e "\n${GREEN}Stopping server (PID $SERVER_PID)...${NC}"
    kill $SERVER_PID
fi

echo -e "\n${GREEN}=== Suite Complete ===${NC}"
