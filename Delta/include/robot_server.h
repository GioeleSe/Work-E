#ifndef ROBOT_SERVER_H
#define ROBOT_SERVER_H

#include <ArduinoJson.h>
#include "udp_server.h"
#include "robot_types.h"

#define PROTOCOL "robot-net/1.0"
#define REFUSE_BUSY_TASK 1                                          // if 1 refuse new task while robot is busy replying with RobotState_BUSY
#define REFUSE_BUSY_TASK_MSG "Task refused cause robot was busy"
#define REFUSE_INTERNAL_ERROR_MSG "Task refused cause robot was confused"
#define TASK_DONE_MSG "Task done successfully c:"



typedef struct PropertyMsg_t{
    ConfigFields_t prop;
    int value;
} PropertyMsg_t;

typedef struct FeedbackMsg_t{
    ActionResult status;
    ErrorCode_t error_code;
    char error_message[MAX_CHAR_MSG];
}FeedbackMsg_t;

// Check packet integrity (structured as protocol)
// and packet meaning -> decide which callback to use
int PacketHandler(char* packet, ssize_t packet_size);

// Decide which action to perform according to the current state and the received Move command
int MoveHandler(MovePayload_t data);

// Decide which action to perform according to the current state and the received Motor Control commandb
int MotorControlHandler(MotorControlPayload_t data);

// Decide which action to perform according to the current state and the received Set Config command
int SetConfigHandler(SetConfigPayload_t data);

// Decide which action to perform according to the current state and the received Get Config command
int GetConfigHandler(GetConfigPayload_t data);

// Decide which action to perform according to the current state and the received Emergency Stop command
int EmergencyStopHandler(EmergencyStopPayload_t data);

// Decide which action to perform according to the current state and the received Reset command
int ResetHandler(ResetPayload_t data);

// Start this server to listen for packets and managing commands
int RobotStartServer();

// Diagnostic counters (for display without serial)
int robot_server_get_packet_count();   // packets that entered PacketHandler
int robot_server_get_fields_ok();      // packets that passed check_fields
int robot_server_get_cmd_count();      // packets that reached motor_control_handler

#endif