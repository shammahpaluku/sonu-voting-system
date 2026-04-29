#!/bin/bash

echo "Testing admin login to server at 10.45.218.186:9100"
echo "Expected: admin / admin123"
echo ""

# Test using netcat/nc to send the command
echo "ADMIN_LOGIN admin admin123" | nc -u 10.45.218.186 9100

echo ""
echo "If you see 'OK' above, authentication works."
echo "If you see 'ERR_AUTH_FAIL' or nothing, there's still an issue."
