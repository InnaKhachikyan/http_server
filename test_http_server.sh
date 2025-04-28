#!/usr/bin/env bash
# Test suite for http_server
# This script launches the server, runs tests, then cleans up.

# Configuration\ nSERVER_BINARY="./http_server"
SERVER_URL="http://127.0.0.1:8080"
AUTH_USER="Alice"
AUTH_PASS="0123456"

# Launch server in background
"$SERVER_BINARY" &
SERVER_PID=$!
# Ensure we kill the server on exit
trap 'kill $SERVER_PID' EXIT

# Give the server a moment to start listening
sleep 1

# Helper: perform a request and compare HTTP status
# Args: description, [curl flags], URL, expected_status
run_test() {
    local desc="$1"; shift
    local expected_status="${@: -1}"
    local args=("${@:1:$#-1}")

    status=$(curl -s -o /dev/null -w "%{http_code}" "${args[@]}")
    if [[ "$status" == "$expected_status" ]]; then
        echo "[PASS] $desc → $status"
    else
        echo "[FAIL] $desc → got $status, expected $expected_status"
    fi
}

# 1. Missing auth header → 401
run_test "GET / without auth" "$SERVER_URL/" 401

# 2. Valid auth, root → 200
run_test "GET / with valid auth" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/" 200

# 3. Valid auth, index.html → 200
run_test "GET /index.html with valid auth" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/index.html" 200

# 4. Non-existent resource → 404
run_test "GET /nope.txt" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/nope.txt" 404

# 5. Directory traversal attempt → 400
run_test "GET /../secret" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/../secret" 400

# 6. Wrong credentials → 403
run_test "GET / with wrong auth" -u "Alice:wrongpass" "$SERVER_URL/" 403

# 7. Unsupported method → 405
run_test "POST / with valid auth" -u "$AUTH_USER:$AUTH_PASS" -X POST "$SERVER_URL/" 405

# 8. Trailing slash directory test for real dir → 200
run_test "GET / (trailing slash) directory" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/" 200

# 9. Concurrency test: 5 parallel GET /
echo -e "\n[INFO] Running concurrency test (5 parallel requests)"
for i in {1..5}; do
    curl -s -o /dev/null -w "%{http_code}" -u "$AUTH_USER:$AUTH_PASS" "$SERVER_URL/" &
done > results.txt
wait

# Summarize
echo -e "\nConcurrency test results:"
sort results.txt | uniq -c | while read count code; do
    echo "  $count requests returned HTTP $code"
done
rm results.txt

