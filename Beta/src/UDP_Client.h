// Header file for sending feedback and other messages to the server

#ifndef UDP_CLIENT

#define UDP_CLIENT

#include <WiFi.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>

// WiFi information
#define WIFI_SSID "vomi tino"
#define WIFI_PASSWORD "c0nn3tt1t1"

// Server's address and port
#define SERVER_IP (192, 168, 137, 1)
#define SERVER_PORT 8000

void connectToServer();
void sendMessage(JsonDocument message);
JsonDocument heartbeatMessage();
JsonDocument feedbackMessage();
JsonDocument eventMessage();
JsonDocument errorMessage();

// Other utilities
void checkConnection();

#endif