#include <AsyncUDP.h>
#include <ArduinoJson.h>

#define PROTOCOL "robot-net/1.0"
#define UDP_PORT 8000


typedef enum{
    RobotState_IDLE = 0,
    RobotState_BUSY = 1,
    RobotState_ERR = 2
}RobotState_t;
typedef enum{
    ActionResult_SUCCESS = 0,
    ActionResult_FAILURE = 1,
    ActionResult_PENDING = 2
}ActionResult_t;
typedef enum{
    FeedbackEvent_OBSTACLE_DETECTED = 10,
    FeedbackEvent_OBSTACLE_REMOVED = 11,
    FeedbackEvent_POI_REACHED = 12,
    FeedbackEvent_LOAD_COLLECTED = 13,
    FeedbackEvent_LOAD_DISPOSED = 14,
    FeedbackEvent_REROUTE_REQUIRED = 15,
    FeedbackEvent_MISSING_LOAD = 16
}FeedbackEvent_t;
typedef enum{
    ErrorSeverity_LOW = 0,
    ErrorSeverity_MID = 1,
    ErrorSeverity_HIGH = 2
} ErrorSeverity_t;

typedef int ErrorCode_t;
typedef struct FeedbackMsg_t{
    ActionResult_t status;
    ErrorCode_t error_code;
    char* error_message;
}FeedbackMsg_t;
typedef struct EventMsg_t{
    FeedbackEvent_t event;
    const char* details; 
}EventMsg_t;
typedef struct DebugMsg_t{
    const char* message;
}DebugMsg_t;
typedef struct ErrorMsg_t{
    ErrorSeverity_t severity;
    int error_code;
}ErrorMsg_t;

// send HeartBeat message (application level only here. 1 shot only)
// Create the v4rbeat message with fields "state" and "rssi", encode it in JSON and send it with SendMessage
// (no args needed for now)
int SendHeartbeat(void* args);

// send Feedback message
int SendFeedback(FeedbackMsg_t msg);

// send Event message
int SendEvent(EventMsg_t msg);

// send Debug message
int SendDebug(DebugMsg_t msg);

// send via UDP the encoded JSON message (only IP-level here)
int SendMessage(StaticJsonDocument msg);
