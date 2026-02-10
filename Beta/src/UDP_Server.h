// Header file for receving and handling commands from the server

#ifndef UDP_SERVER

#define UDP_SERVER

using namespace std;

#include <WiFi.h>
#include <AsyncUDP.h>
#include <ArduinoJson.h>
#include "Beta.h"

#define PROTOCOL_VER "robot-net/1.0"

// WiFi information
#define WIFI_SSID "vomi tino"
#define WIFI_PASSWORD "c0nn3tt1t1"

// Server's local port
#define LOCAL_SERVER_PORT 8000

//* All commands generate a Feedback message as a response from the MCU

typedef struct IncomingMessageStructure
{
    const char *protocol;
    CommandType messageType;
    uint8_t robotID;
    uint16_t requestID;
    NavigationType mode;
    uint32_t timestamp;
    //! payload
} IncomingMessageStructure;

// Command types that can be received from server
typedef enum CommandType
{
    MOVE, //! NO, refers to autonomous movement
    MOTOR_CONTROL,
    SET_PROPERTY,
    GET_PROPERTY,
    EMERGENCY_STOP,
    RESET,
} CommandType;

typedef struct MotorControlPayload
{
    // Multiple motors can be activated at the same time
    // An empty vector indicates the intention to drive the robot as a car
    MotorID *motorIDs;
    Direction direction;
    uint8_t speed; // Range 0-100
    int angle;     // Range -360 to +360, on-spot rotation
    uint32_t duration;
};

typedef struct SetPropertyPayload
{
    Properties property; // Property to be changed (one at a time)
    void *newValue;      // New value to be set
} SetPropertyPayload;

typedef struct GetPropertyPayload
{
    Properties property;
} GetPropertyPayload;

typedef struct EmergencyStopPayload
{
    // See if the "stop" keyword is matched
    // Used to stop any currently running tasks
    const char *stop;
};

typedef struct ResetPayload
{
    // See if the "reset" keyword is matched
    // Used to reset the board or clear error/emergency stop state
    const char *reset;
};

void startUDPServer();
void handleIncomingPacket(AsyncUDPPacket packet); //! function type likely to be changed

// Other utilities
string stringifyData(uint8_t *packetData, size_t dataLength);
void printDataAsString(uint8_t *packetData, size_t dataLength);

#endif