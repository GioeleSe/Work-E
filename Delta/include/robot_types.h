#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H
#pragma once
#include <stdint.h>

#define MAX_MOTORS_COUNT 8                                          // avoid heap allocation, preallocate full size (int) array
#define MAX_CHAR_MSG 40

typedef enum{
    ErrorCode_NO_ERROR = 0,
    ErrorCode_ROBOT_ERROR_STATE = 100,
    ErrorCode_ROBOT_BUSY_STATE = 200                                // command ignored cause it was busy
}ErrorCode_t;
typedef enum{
    RobotState_IDLE = 0,
    RobotState_BUSY = 1,
    RobotState_ERR = 2
}RobotState_t;
typedef enum MessageMode
{
    MessageMode_MANUAL = 0,
    MessageMode_AUTO = 1
} MessageMode_t;
typedef enum MessageType
{
    MessageType_COMMAND = 0,
    MessageType_HEARTBEAT = 1,
    MessageType_EVENT = 2,
    MessageType_PROPERTY = 3,
    MessageType_FEEDBACK = 4,
    MessageType_DEBUG = 5,
    MessageType_ERROR = 6

}MessageType_t;
typedef enum
{
    CommandType_GET_PROPERTY = 0,
    CommandType_SET_PROPERTY = 1,
    CommandType_MOTOR_CONTROL = 2,                                  // used to manually drive the car 
    CommandType_MOVE = 3,                                           // for autonomous movements with POIs
    CommandType_EMERGENCY_STOP = 4,
    CommandType_RESET = 5
}CommandType_t;
typedef enum ActionResult{
    ActionResult_SUCCESS = 0,
    ActionResult_FAILURE = 1,
    ActionResult_PENDING = 2
}ActionResult_t;
typedef enum
{
    DestinationCheckpoint_HOME = 0, // starting point
    DestinationCheckpoint_LOAD = 1, // position of the load to collect 
    DestinationCheckpoint_BASE = 2  // load destination
} DestinationCheckpoint_t;
typedef enum
{
    NavigationType_MANUAL = 0,
    NavigationType_CHECKPOINT = 1,
    NavigationType_GRID = 2,
    NavigationType_FREE_MOVE = 3
} NavigationType_t;
typedef enum
{
    RoutePolicy_SHORTEST = 0,
    RoutePolicy_SAFEST = 1,
    RoutePolicy_FAST = 2
} RoutePolicy_t;
typedef enum
{
    Direction_FORWARD = 0,
    Direction_BACKWARD = 1,
    Direction_LEFT = 2,
    Direction_RIGHT = 3,
    Direction_STOP = 4
} Direction_t;
typedef enum
{
    SpeedLevel_SLOW = 0,   // reduced speed of all functionalities with possible forced idle cycles (mainly for debug)
    SpeedLevel_NORMAL = 1,  // normal flow
    SpeedLevel_FAST = 2    // faster decisions and functionalities (for future extensions)
} SpeedLevel_t;
typedef enum
{
    DebugLevel_OFF = 0,    // don't
    DebugLevel_BASIC = 1,  // warning messages only
    DebugLevel_FULL = 2    // debug all possible messages and events
} DebugLevel_t;
typedef enum
{
    FeedbackLevel_SILENT = 0, // basic feedback needed for server
    FeedbackLevel_MINIMAL = 1, // feedback messages with more details/info
    FeedbackLevel_DEBUG = 2   // send back to server a debug-level feedback
} FeedbackLevel_t;
typedef enum{
    ErrorSeverity_t_LOW = 0,
    ErrorSeverity_t_MID = 1,
    ErrorSeverity_t_HIGH = 2
} ErrorSeverity_t;
typedef enum
{
    Motors_END_MOT = -1, // end of motor list
    Motors_RES = 0,      // reserved for now
    Motors_MOT1 = 1,     // left car movement motor
    Motors_MOT2 = 2,     // right car movement motor
    Motors_MOT3 = 3,     // additional motors
    Motors_MOT4 = 4,     // additional motors
    Motors_MOT5 = 5,     // ...
    Motors_MOT6 = 6      // ...
} Motors_t;
typedef enum
{
    ConfigFields_SPEED = 0,
    ConfigFields_FEEDBACK = 1,
    ConfigFields_DEBUG = 2,
    ConfigFields_NAVIGATION_TYPE = 3,
    ConfigFields_ROUTE_POLICY = 4,
    ConfigFields_RADAR = 5,
    ConfigFields_SCREEN = 6,
    ConfigFields_OBSTACLE_CLEANER = 10, // range gap for future updates
    ConfigFields_OBJECT_LOADER = 11,
    ConfigFields_OBJECT_UNLOADER = 12,
    ConfigFields_OBJECT_COMPACTER = 13
} ConfigFields_t;
// endof Server-Common Structures

typedef struct MovePayload
{
    int destination_x;
    int destination_y;
    DestinationCheckpoint_t destination_checkpoint;
    NavigationType_t navigation_type;
    RoutePolicy_t route_policy;
} MovePayload_t;
typedef struct MotorControlPayload
{
    Motors_t motor_ids[MAX_MOTORS_COUNT];   // can activate multiple motors at once, if the vector is empty the intended action is to drive it as a car
    Direction_t direction; // Direction_FORWARD or Direction_BACKWARD for direct motor control. Can be Direction_LEFT or Direction_RIGHT only if motor_ids is empty
    uint8_t speed;       // 0-100, basically the direct pwm ratio
    int angle;           // from -360 to +360, on spot rotation angle
    uint32_t duration;   // can be 0 or greater (0 -> keep running, otherwise set a timeout)
} MotorControlPayload_t;
typedef struct SetConfigPayload
{
    ConfigFields_t prop; // single prop from ConfigFields_t list
    int new_value;   // new value type depends on what's the property but transmitted as integer
} SetConfigPayload_t;
typedef struct GetConfigPayload
{
    ConfigFields_t prop; // expected one of the ConfigFields_t to return its value
} GetConfigPayload_t;
typedef struct EmergencyStopPayload
{
    char stop[MAX_CHAR_MSG]; // gotta simply match the "stop" keyword to stop the tasks currently running
} EmergencyStopPayload_t;
typedef struct ResetPayload
{
    char reset[MAX_CHAR_MSG]; // gotta simply match the "reset" keyword to reset the board (idle/busy) or clear the error state (or clear the active emergency stop state)
} ResetPayload_t;

#endif // ROBOT_TYPES_H
