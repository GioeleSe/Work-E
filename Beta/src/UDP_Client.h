// Header file for sending feedback and other messages to the server

#ifndef UDP_CLIENT

#define UDP_CLIENT

using namespace std;

#include <WiFi.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>
#include "Beta.h"

#define PROTOCOL_VER "robot-net/1.0"

// WiFi information
#define WIFI_SSID "vomi tino"
#define WIFI_PASSWORD "c0nn3tt1t1"

// Server's address and port
#define SERVER_IP (192, 168, 137, 1)
#define SERVER_PORT 8000

// Possible message types
typedef enum OutgoingMessageType
{
    ACTION_FEEDBACK,
    EVENT,
    DEBUG,
    ERROR,
    HEARTBEAT,
} OutgoingMessageType;

// Message structure
typedef struct OutgoingMessageStructure
{
    const char *protocol;
    uint8_t robotID;
    OutgoingMessageType messageType;
    uint16_t requestID;
    NavigationType mode;
    //! payload
    uint32_t timestamp;
} OutgoingMessageStructure;

// todo
typedef struct ActionFeedbackPayload
{
};

typedef struct EventPayload
{
};

typedef struct DebugPayload
{
};

typedef struct ErrorPayload
{
};

typedef struct HeartbeatPayload
{
};

void connectToServer();
void sendMessage(JsonDocument message);
//! WOMP WOMP, you have to remake these
JsonDocument heartbeatMessage();
JsonDocument feedbackMessage();
JsonDocument eventMessage();
JsonDocument errorMessage();

// Other utilities
void checkConnection();

#endif