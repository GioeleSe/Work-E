

#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include "fsm.h"
#include "configs.h"


extern AsyncUDP udp; // AsyncUDP instance
// udp.listen(port) to start listening
// udp.onPacket(callback) to set receive callback



extern char udpBuffer[]; // to store incoming UDP data
extern unsigned long timeOffset; // Offset to adjust millis() to epoch time if NTP fails
// timeOffset = epoch_time_ms - millis() at the moment of sync
// epoch_time_ms = millis() + timeOffset

void initUDP(); // init UDP communication and set up receive callback
void syncTimeNTP(); // synchronize time using NTP, fallback to millis() if fails



void buildEnvelope(JsonDocument& doc, const char* messageType, const char* mode);
// Helper to build message envelope with common fields

unsigned long getEpochTimeMs(); // get current time in epoch ms,
// using NTP if available, otherwise using millis() + offset



void sendUDP(const JsonDocument& doc);

// heartbeat - status each n seconds
void sendHeartbeat(RobotState_t state);


// feedback - response to commands
void sendFeedback(uint16_t reqId, int status, int errorCode, const char* errorMsg);

// event - notify important events (obstacles, errors, etc)
void sendEvent(int eventCode, const char* details);

// property - response to get_property
void sendProperty(const char* prop, const char* value);

// debug - debug messages
void sendDebug(const char* message);

// error - critical errors
// severity: 0=LOW, 1=MID, 2=HIGH
void sendError(int severity, int errorCode);




// Parses incoming UDP packets, validates and extracts commands, then calls handleCommand()
void parseUDPPacket(); 


// Command handling: parses the command and executes it, sending feedback as needed
void handleCommand(CommandType cmd, uint16_t reqId, const JsonDocument& payload);

void setupUDP();