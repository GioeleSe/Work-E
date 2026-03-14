#pragma once

#include <Arduino.h>
#include <WiFi.h>

// WiFi credentials

extern const char* SSID;
extern const char* PASSWORD;
extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress SERVER_IP;


// UDP settings

#define UDP_BUFFER_SIZE 512
#define JSON_DOC_SIZE 512
extern const int UDP_PORT;


// Robot settings

#define ROBOT_ID 2


// TIMING CONFIGURATION
extern const unsigned long HEARTBEAT_INTERVAL;

void setupSerial();
void setupWiFi();

#define NTP_SERVER "pool.ntp.org"


#define GMT_OFFSET_SEC (3600)
#define DAYLIGHT_OFFSET_SEC (3600)