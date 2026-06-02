// Header file for defining the Software/Hardware interface and controlling the robot

#ifndef BETA

#define BETA

using namespace std;

#define ROBOT_ID 1

// Control pins for motors 1-2
//?? placeholders
#define MOTOR_1A_PIN 0
#define MOTOR_1B_PIN 1
#define MOTOR_2A_PIN 2
#define MOTOR_2B_PIN 3
#define ULT_PIN 4 // To enable sleep mode for the motors

#define MAX_SPEED 2.55

//!!! remake all these according to protocol !!!
typedef enum RobotState
{
    RobotState_IDLE = 0,
    RobotState_BUSY = 1,
    RobotState_ERROR = 2,
} RobotState;

typedef enum ActionResult
{
    ActionResult_SUCCESS = 0,
    ActionResult_FAILURE = 1,
    ActionResult_PENDING = 2,
} ActionResult;

typedef enum FeedbackEvent
{
    OBSTACLE_DETECTED = 10,
    OBSTACLE_REMOVED = 11,
    POI_REACHED = 12,
    LOAD_COLLECTED = 13,
    LOAD_DISPOSED = 14,
    REROUTE_REQUIRED = 15,
    MISSING_LOAD = 16
} FeedbackEvent;

typedef enum ErrorSeverity
{
    ErrorSeverity_LOW = 0,
    ErrorSeverity_MID = 1,
    ErrorSeverity_HIGH = 2,
} ErrorSeverity;

typedef enum NavigationType
{
    NavigationType_MANUAL = 0, // manual GUI, robot as dummy collection of motors
    NavigationType_CHECKPOINT = 1, // home-load-target navigation
    NavigationType_GRID = 2, // prefixed grid pattern (90° rotations only at turning points, like a chess board)
    NavigationType_FREE_MOVE = 3 // no target, no grid, just an happy robot going around (for future extensions obv)
} NavigationType;

typedef enum Direction
{
    Direction_FORWARD = 0,
    Direction_BACKWARD = 1,
    Direction_LEFT = 2,
    Direction_RIGHT = 3,
    Direction_STOP = 4,
} Direction;

typedef enum MotorID
{
    MotorID_END_MOT = -1,
    MotorID_RES = 0,
    MotorID_MOT1 = 1, // Wheel DC motor 1
    MotorID_MOT2 = 2, // Wheel DC motor 2
    MotorID_MOT3 = 3, // Trunk DC motor
    MotorID_MOT4 = 4, // Radar Servo motor
} MotorID;

//!! you may NOT need all of these
typedef enum Properties
{
    Properties_SPEED = 0,       // slow | normal | fast
    Properties_FEEDBACK = 1,    // silent | minimal | debug
    Properties_DEBUG = 2,           // off | basic | full
    Properties_NAVIGATION_TYPE = 3, // manual | checkpoint | grid | free_move
    Properties_ROUTE_POLICY = 4,    // shortest | safest | fast
    Properties_RADAR = 5,           // 0 | 1 (OFF | ON)
    Properties_SCREEN = 6,          // 0 | 1
    Properties_OBJECT_LOADER = 7,   // 0 | 1
    Properties_OBJECT_UNLOADER = 8, // 0 | 1
} Properties;

//** 0 = OK
typedef int errorCode_t;

typedef struct Beta
{
    NavigationType navigationMode; //!! NO
    RobotState currentState;
    Properties *properties;
    ActionResult lastActionResult;
    // Booleans to be used to determine currentState
    // bool connected;
    // bool moving;
} Beta;

void moveForward();
void moveBackward();
void stop();

//!! very high-level, it would be better to work terra terra first
void move(int dutyCycle, Direction dir);
void spinClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);
void spinAntiClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);

#endif