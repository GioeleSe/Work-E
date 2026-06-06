#include <Wire.h>
#ifndef BETA_H // Header file for defining the Software/Hardware interface and controlling the robot

#define BETA_H

#define SERIAL_BAUD 115200  // serial speed here for easier adjustments
#define SELF_ROBOT_ID 3     // ID assigned only here to make usage easier
#define MAX_SPEED 2.55

// Control pins for motors 1-2 (Car movements) and trunk
#define PIN_DRIVER_1_MOTOR_1A 25
#define PIN_DRIVER_1_MOTOR_1B 23
#define PIN_DRIVER_1_MOTOR_2A 22
#define PIN_DRIVER_1_MOTOR_2B 21
#define PIN_DRIVER_1_SLEEP 16

#define PIN_DRIVER_2_MOTOR_1A 25
#define PIN_DRIVER_2_MOTOR_1B 23
#define PIN_DRIVER_2_SLEEP 17

const int MOTOR_MAPPING_CAR_LEFT = 1;
const int MOTOR_MAPPING_CAR_RIGHT = 2;
const int MOTOR_MAPPING_TRUNK = 3;
const int MOTOR_MAPPING_RADAR = 4;
typedef enum
{
    MotorID_END_MOT = -1,
    MotorID_RES = 0,
    MotorID_MOT1 = 1, // Wheel DC motor 1
    MotorID_MOT2 = 2, // Wheel DC motor 2
    MotorID_MOT3 = 3, // Trunk DC motor
    MotorID_MOT4 = 4, // Radar Servo motor
} MotorRobotToID;

// serial and screen properties and defintions
#define SERIAL_PRINT
#define PIN_OLED_SDA 33
#define PIN_OLED_SCL 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



struct Beta_t{
    RobotState_t robot_state = RobotState_t::RobotState_IDLE;
    int speed = 0;
    int feedback = 0;
    int debug = 0;
    NavigationType_t navigation_type = NavigationType_t::NavigationType_MANUAL;
    RoutePolicy_t route_policy = RoutePolicy_t::RoutePolicy_FAST;
    int radar = 0;
    int screen = 0;
    int obstacle_cleaner = 0;
    int object_loader = 0;
    int object_unloader = 0;
    int object_compacter = 0;

    ActionResult lastActionResult;
    // Booleans to be used to determine currentState
    int isRequiredReset;    // used for soft reset (stop actions before rebooting the board)
    int isRequiredStop;     // used for emergency stop
    int isServerConnected;
    int isCarMoving;
    int isRadarMoving;
    int isOpenTrunk;
    int isBuzzerPlaying;
} robot_beta;   // defined and initialized only here


const char* logSeverityLabels[] = {
    "INFO",
    "WARN",
    "ERROR"
};

void logMessage(ErrorSeverity_t severity, const char* message);
void oledInit();    // call this in the setup before usage
void oledPrint(const char* message);

void rebootTimerCallback(TimerHandle_t xTimer);
#endif