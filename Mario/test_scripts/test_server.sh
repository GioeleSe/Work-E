#!/bin/bash

set -e

echo "==> Compiling server..."
g++ udp_server.cpp robot_server.cpp -o robot_server

echo "==> Compiling client..."
g++ robot_client_test.cpp -o robot_client_test

echo "==> Starting server in background..."
./robot_server &
SERVER_PID=$!

sleep 0.5  # give the server a moment to bind

echo "==> Starting client..."
./robot_client_test

echo "==> Client done. Stopping server (PID $SERVER_PID)..."
kill -SIGINT  $SERVER_PID
wait $SERVER_PID
