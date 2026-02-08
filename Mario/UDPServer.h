#include <AsyncUDP.h>

#define PROTOCOL "robot-net/1.0"
#define UDP_PORT 8000
#define CMD_FIELDS_NUM 6

// Server-Common Structures
typedef enum MessageMode
{
    MANUAL = 0,
    AUTO = 1
} MessageMode;
typedef enum
{
    HOME = 0, // starting point
    LOAD = 1, // position of the load to collect 
    BASE = 2  // load destination
} DestinationCheckpoint;
typedef enum
{
    MANUAL = 0,
    CHECKPOINT = 1,
    GRID = 2,
    FREE_MOVE = 3
} NavigationType;
typedef enum
{
    SHORTEST = 0,
    SAFEST = 1,
    FAST = 2
} RoutePolicy;
typedef enum
{
    FORWARD = 0,
    BACKWARD = 1,
    LEFT = 2,
    RIGHT = 3,
    STOP = 4
} Direction;
typedef enum
{
    SLOW = 0,   // reduced speed of all functionalities with possible forced idle cycles (mainly for debug)
    NORMAL = 1,  // normal flow
    FAST = 2    // faster decisions and functionalities (for future extensions)
} SpeedLevel;
typedef enum
{
    OFF = 0,    // don't
    BASIC = 1,  // warning messages only
    FULL = 2    // debug all possible messages and events
} DebugLevel;
typedef enum
{
    SILENT = 0, // basic feedback needed for server
    MINIMAL = 1, // feedback messages with more details/info
    DEBUG = 2   // send back to server a debug-level feedback
} FeedbackLevel;
typedef enum
{
    END_MOT = -1, // end of motor list
    RES = 0,      // reserved for now
    MOT1 = 1,     // left car movement motor
    MOT2 = 2,     // right car movement motor
    MOT3 = 3,     // additional motors
    MOT4 = 4,     // additional motors
    MOT5 = 5,     // ...
    MOT6 = 6      // ...
} Motors;
typedef enum
{
    SPEED = 0,
    FEEDBACK = 1,
    DEBUG = 2,
    NAVIGATION_TYPE = 3,
    ROUTE_POLICY = 4,
    RADAR = 5,
    SCREEN = 6,
    OBSTACLE_CLEANER = 10, // range gap for future updates
    OBJECT_LOADER = 11,
    OBJECT_UNLOADER = 12,
    OBJECT_COMPACTER = 13
} ConfigFields;
// endof Server-Common Structures

typedef const char *Fields_t[CMD_FIELDS_NUM];
Fields_t CommandFields = {"protocol", "message_type", "request_id", "mode", "payload", "timestamp"};

typedef struct MovePayload
{
    int destination_x;
    int destination_y;
    DestinationCheckpoint destination_checkpoint;
    NavigationType navigation_type;
    RoutePolicy route_policy;
} MovePayload;
typedef struct MotorControlPayload
{
    Motors *motor_ids;   // can activate multiple motors at once, if the vector is empty the intended action is to drive it as a car
    Direction direction; // can be LEFT or RIGHT only if motor_ids is empty, ignored otherwise
    uint8_t speed;       // 0-100, basically the direct pwm ratio
    int angle;           // from -360 to +360, on spot rotation angle
    uint32_t duration;   // can be 0 or greater (0 -> keep running, otherwise set a timeout)
} MotorControlPayload;
typedef struct SetConfigPayload
{
    ConfigFields prop; // single prop from ConfigFields list
    void *new_value;   // new value type depends on what's the property
} SetConfigPayload;
typedef struct GetConfigPayload
{
    ConfigFields prop; // expected one of the ConfigFields to return its value
} GetConfigPayload;
typedef struct EmergencyStopPayload
{
    const char *stop; // gotta simply match the "stop" keyword to stop the tasks currently running
} EmergencyStopPayload;
typedef struct ResetPayload
{
    const char *reset; // gotta simply match the "reset" keyword to reset the board (idle/busy) or clear the error state (or clear the active emergency stop state)
} ResetPayload;

// start a local AsyncUDP server, will be listening for packets coming from everyone on the defined port
// Incoming packets are serialized JSON structures, need to decode them and check for the correct fields presence
// The parsed payload is passed as parameters to the specific handler
int StartServer(int port);

// Check packet integrity (structured as protocol)
// and packet meaning -> decide which callback to use
int PacketHandler(AsyncUDPPacket packet);

// Decide which action to perform according to the current state and the received Move command
int MoveHandler(MovePayload data);

// Decide which action to perform according to the current state and the received Motor Control command
int MotorControlHandler(MotorControlPayload data);

// Decide which action to perform according to the current state and the received Set Config command
int SetConfigHandler(SetConfigPayload data);

// Decide which action to perform according to the current state and the received Get Config command
int GetConfigHandler(GetConfigPayload data);

// Decide which action to perform according to the current state and the received Emergency Stop command
int EmergencyStopHandler(EmergencyStopPayload data);

// Decide which action to perform according to the current state and the received Reset command
int ResetHandler(ResetPayload data);
