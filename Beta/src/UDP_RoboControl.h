// Header file for managing UDP connection with the server, processing and sending data

#ifndef UDP_ROBOCONTROL

#define UDP_ROBOCONTROL

#include <WiFi.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>

// WiFi information
const char *ssid = "vomi tino";
const char *password = "c0nn3tt1t1";

// Server's address and port
const IPAddress serverIP(0, 0, 0, 0); //? to be set after WiFi connection?
const uint16_t serverPort = 8000;

// Client side communication
void connectToServer();
void sendMessage(JsonDocument message);
JsonDocument heartbeatMessage();
JsonDocument feedbackMessage();
JsonDocument eventMessage();
JsonDocument errorMessage();

// Server side communication
void startUDPServer();

// Other utilities
void checkConnection();

#endif