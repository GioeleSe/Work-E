#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>
#include <cstring>
//
enum CommandType {
    MOVE,
    MOTOR_CONTROL,
    SET_CONFIG,
    EMERGENCY_STOP,
    RESET
};

enum RobotResponse {
    HEARTBEAT,
    EVENT,
    FEEDBACK,
    ERROR
};

// -------------- COMMANDS FROM SERVER TO ROBOT --------------
// For MOVE command payload
  typedef enum
  {
    HOME = 0, // starting point
    LOAD = 1, // position of the load to collect 
    BASE = 2  // load destination
  } DestinationCheckpoint;
  typedef enum {
    MANUAL = 0, // manual GUI, robot as dummy collection of motors
    CHECKPOINT = 1, //home-load-target navigation
    GRID = 2, // prefixed grid pattern (90° rotations only at turning points, like a chess board)
    FREE_MOVE = 3 // no target, no grid, just an happy robot going around (for future extensions obv)
  } NavigationType;
  typedef enum {
    SHORTEST = 0,
    SAFEST = 1,
    FAST = 2
  } RoutePolicy;
  // Payload of the incoming message:
  typedef struct MovePayload{
      int destination_x;  // 2d coordinates for grid movements, the robot will follow a grid pattern to the defined destination
      int destination_y;  // ...
      DestinationCheckpoint destination_checkpoint;
      NavigationType navigation_type;
      RoutePolicy route_policy;
  } MovePayload;

// for Motor control command payload
  // Server-Common Structures
  typedef enum{
      END_MOT = -1, // end of motor list
      RES = 0,    // reserved for now
      MOT1 = 1,   // left car movement motor
      MOT2 = 2,   // right car movement motor
      MOT3 = 3,   // additional motors
      MOT4 = 4,   // additional motors
      MOT5 = 5,   // ...
      MOT6 = 6    // ...
  } Motors;
  typedef enum{
    FORWARD = 0,
    BACKWARD = 1,
    LEFT = 2,
    RIGHT = 3,
    STOP = 4 // only for motors, it's a simple way to stop them NOT the emergency one
  } Direction;
  // Payload of the incoming message:
  typedef struct MotorControlPayload{
    Motors* motor_ids;     // can activate multiple motors at once, if the vector is empty the intended action is to drive it as a car
    Direction direction;    // can be LEFT or RIGHT only if motor_ids is empty, ignored otherwise
    uint8_t speed;      // 0-100, basically the direct pwm ratio
    int angle;          // from -360 to +360, on spot rotation angle
    uint32_t duration;  // can be 0 or greater (0 -> keep running, otherwise set a timeout)
  } MotorControlPayload;
// for Set Config
  // Server-Common Structures
  typedef enum{
    SPEED = 0,
    CONFIG_FEEDBACK = 1,
    DEBUG = 2,
    NAVIGATION_TYPE = 3,
    ROUTE_POLICY = 4,
    RADAR = 5,
    SCREEN = 6,
    OBSTACLE_CLEANER = 10,  // range gap for future updates
    OBJECT_LOADER = 11,
    OBJECT_UNLOADER = 12,
    OBJECT_COMPACTER = 13
  }ConfigFields;
  typedef struct SetConfigPayload
  {
    ConfigFields prop; // single prop from ConfigFields list
    void *new_value;  // new value type depends on what's the property
  } SetConfigPayload;

// for get config
  // the configfields are the same as set config
  typedef struct GetConfigPayload
  {
    ConfigFields prop; // expected one of the ConfigFields to return its value
  } GetConfigPayload;

//for set config
  typedef struct EmergencyStopPayload
  {
    const char *stop; // gotta simply match the "stop" keyword to stop the tasks currently running
  } EmergencyStopPayload;
  
//for reset
  typedef struct ResetPayload
  {
    const char *reset; // gotta simply match the "reset" keyword to reset the board (idle/busy) or clear the error state (or clear the active emergency stop state)
  } ResetPayload;
  //-------------------------------------------------------------
// -------------- RESPONSES FROM ROBOT TO SERVER --------------
//---------------------------------------------------------------
// for heartbeat 
typedef enum{
    IDLE = 0,
    ROBOT_BUSY = 1,
    ERR = 2
}RobotState;


//for feedback
  // Server-Common Structures
  typedef enum{
    SUCCESS = 0,
    FAILURE = 1,
    ACTION_PENDING = 2
  }ActionResult;
  typedef int ErrorCode_t;

  typedef struct FeedbackMsg_t{
    ActionResult status;
    ErrorCode_t error_code;
    char* error_message;
  }FeedbackMsg_t;

  //for event
    // Server-Common Structures
  typedef enum{
    obstacle_detected = 10,
    obstacle_removed =11,
    poi_reached = 12,
    load_collected = 13,
    load_disposed = 14,
    reroute_required = 15,
    missing_load = 16
  }FeedbackEvent_t;

  typedef struct EventMsg_t{
    FeedbackEvent_t event;
    const char* details; 
  } EventMsg_t;

  //for debug
    typedef struct DebugMsg_t{
    const char* message;    // the only field for now, could be extended!
  }DebugMsg_t;
  //for error
    typedef struct ErrorMsg_t{
        ErrorCode_t error_code;
        const char* error_message;
    }ErrorMsg_t;

  


void initializeWiFi();
void startUDPServer();
void commsLoop();

void handlePacket(AsyncUDPPacket packet);
void sendHeartbeat();
void sendFeedback(uint16_t request_id, const JsonObject& payload, const char* status);
void sendFeedbackSuccess(uint16_t request_id, uint16_t mode_bit);