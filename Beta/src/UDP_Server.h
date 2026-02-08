// Header file for receving and handling commands to the server

#ifndef UDP_SERVER

#define UDP_SERVER

#include <WiFi.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>

// WiFi information
#define WIFI_SSID "vomi tino"
#define WIFI_PASSWORD "c0nn3tt1t1"

// Server's local port
#define LOCAL_SERVER_PORT 8000

void startUDPServer();
void handleIncomingPacket(AsyncUDPPacket packet); //! type is likely to be changed

#endif