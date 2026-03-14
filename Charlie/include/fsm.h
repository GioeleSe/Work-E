#pragma once

#include <Arduino.h>


typedef enum {
  ROBOT_IDLE = 0,
  ROBOT_BUSY,
  ROBOT_ERR
} RobotState_t;


// FSM STATE VARIABLES
extern RobotState_t state;
extern uint16_t lastRequestId;
extern bool busyStarted;
extern unsigned long busyStartTime;


// COMMAND TYPES
enum CommandType {
  CMD_NONE,
  CMD_MOVE,
  CMD_MOTOR_CONTROL,
  CMD_SET_PROPERTY,
  CMD_GET_PROPERTY,
  CMD_STOP
};


// FSM FUNCTIONS
void setupFSM();
void updateFSM(unsigned long now);

