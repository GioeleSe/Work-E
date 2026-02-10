// Header file for defining the Software/Hardware interface and controlling the robot

#ifndef BETA

#define BETA

using namespace std;

#define ROBOT_ID 1

// Control pins for motors 1-2
//? placeholders
#define MOTOR_1A_PIN 0
#define MOTOR_1B_PIN 1
#define MOTOR_2A_PIN 2
#define MOTOR_2B_PIN 3
#define ULT_PIN 4 // To enable sleep mode for the motors

#define MAX_SPEED 2.55

//!!! remake all these according to protocol !!!
typedef enum RobotState
{
    IDLE = 0,
    BUSY = 1,
    ERROR = 2,
} RobotState;

typedef enum ActionResult
{
    SUCCESS = 0,
    FAILURE = 1,
    PENDING = 2,
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
    LOW = 0,
    MID = 1,
    HIGH = 2,
} ErrorSeverity;

typedef enum NavigationType
{
    RADAR,       // Automatic
    POSITIONING, // Manual/Debug
} NavigationType;

typedef enum Direction
{
    FORWARD = 0,
    BACKWARD = 1,
    LEFT = 2,
    RIGHT = 3,
    STOP = 4,
} Direction;

typedef enum MotorID
{
    END_MOT = -1,
    RES = 0,
    MOT1 = 1, // Wheel DC motor 1
    MOT2 = 2, // Wheel DC motor 2
    MOT3 = 3, // Trunk DC motor
    MOT4 = 4, // Radar Servo motor
} MotorID;

//! you may NOT need all of these
typedef enum Properties
{
    SPEED = 0,       // slow | normal | fast
    FEEDBACK = 1,    // silent | minimal | debug
    DEBUG,           // off | basic | full
    NAVIGATION_TYPE, // manual | checkpoint | grid | free_move
    ROUTE_POLICY,    // shortest | safest | fast
    RADAR,           // 0 | 1 (OFF | ON)
    SCREEN,          // 0 | 1
    OBJECT_LOADER,   // 0 | 1
    OBJECT_UNLOADER, // 0 | 1
} Properties;

//* 0 = OK
typedef int errorCode_t;

typedef struct Beta
{
    NavigationType navigationMode; //! NO
    RobotState currentState;
    Properties *properties;
    ActionResult lastActionResult;
    // Booleans to be used to determine currentState
    // bool connected;
    // bool moving;
} Beta;

void move(int dutyCycle, Direction dir);
void spinClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);
void spinAntiClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed);
void stop();

#endif