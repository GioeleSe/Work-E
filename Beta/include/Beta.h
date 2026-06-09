
#ifndef BETA_H
#define BETA_H
#include "robot_types.h"
#include "main_robot.h"
#include "car_motion.h"
#include "trunk_motion.h"
#include "radar_motion.h"
#include "dcmotor_motion.h"
#include <Wire.h>
#include <ESP32Servo.h>
#include <VL53L0X.h>
#include <Adafruit_SSD1306.h>

#define ROBOT_ID 3 // copied in robot_beta struct for easier usage
#define SERIAL_BAUD 115200  // serial speed here for easier adjustments
#define MAX_SPEED 100   // max percentage speed
#define WIFI_SSID "local_hotspot"
#define WIFI_PASSWORD "esp32_mcu"

// serial and screen properties and defintions
#define PRINT_ONLY_TEST     // test Beta functions only by printing the called one
#define PIN_OLED_SDA 33
#define PIN_OLED_SCL 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1 
#define SCREEN_ADDRESS 0x3C

// Control pins for motors 1-2 (Car movements) and trunk
#define DRIVER_PWM_FREQ 20000
#define DRIVER_PWM_RES 8

#define PIN_DRIVER_1_MOTOR_1A 21
#define PIN_DRIVER_1_MOTOR_1B 22
#define PIN_DRIVER_1_MOTOR_2A 25
#define PIN_DRIVER_1_MOTOR_2B 23
#define PIN_DRIVER_1_SLEEP 16

#define PIN_DRIVER_2_MOTOR_1A 18
#define PIN_DRIVER_2_MOTOR_1B 19
#define PIN_DRIVER_2_SLEEP 17
#define PIN_TRUNK_LIMIT 34

#define MOTOR_CAR_LEFT  Motors_t::Motors_MOT1
#define MOTOR_CAR_RIGHT Motors_t::Motors_MOT2
#define MOTOR_TRUNK     Motors_t::Motors_MOT3
#define MOTOR_RADAR     Motors_t::Motors_MOT4

typedef enum {
    TRUNK_IDLE,
    TRUNK_CLOSING, // tracking only closing movement as it's the only one with a limit switch
    TRUNK_READY,
    TRUNK_OPENING,
    TRUNK_ERROR
} TrunkState_t;

// control pins and definitions for the radar structure (proximity sensor + servo motor)
#define PIN_SERVO_CONTROL 13
#define PIN_RADAR_SDA 26
#define PIN_RADAR_SCL 27
#define SERVO_STOP_US           1500    // pulse-width value (microseconds) to stop the motor
#define SERVO_CW_US             1300    // pulse-width value (microseconds) to spin the motor clockwise
#define SERVO_CCW_US            1700    // pulse-width value (microseconds) to spin the motor counter-clockwise
#define SERVO_DEG_PER_MS_CW     0.25f   // deegrees rotation per millisecond (clockwise)
#define SERVO_DEG_PER_MS_CCW    0.22f   // deegrees rotation per millisecond (counter-clockwise)
#define RADAR_CONE_DEG 30               // radar's field of view
#define RADAR_STEP_DEG 7                // angular distance between each radar measurement
#define RADAR_SAMPLES (RADAR_CONE_DEG / RADAR_STEP_DEG + 1)
#define RADAR_OBSTACLE_MM 100 // threshold distance for obstacles alarms
#define RADAR_SCAN_INTERVAL_MS 5000

typedef struct {
    Servo    motorServo;
    VL53L0X  distanceSensor;
    int      currentRadarAngle;
    float    lastMinDistance;
    bool     obstacleDetected;
    bool     scanInProgress;
    int      scanStep;
    float    scanSum;
    int      scanValidCount;
} Radar_t;


#define CAR_ROTATE_TIME 2000 // time to activate the left and right wheels for
#define PIN_BUZZER 4
#define BUZZER_BEEP_ON_MS    100
#define BUZZER_BEEP_OFF_MS   100
#define BUZZER_BEEP_COUNT    3
void setupBuzzer();
void updateBuzzer();

typedef struct Beta_t {
    const int robotID                = ROBOT_ID;
    RobotState_t robot_state         = RobotState_t::RobotState_IDLE;
    int speed                        = 100;
    int feedback                     = 1;
    int debug                        = 1;
    NavigationType_t navigation_type = NavigationType_t::NavigationType_MANUAL;
    RoutePolicy_t route_policy       = RoutePolicy_t::RoutePolicy_FAST;
    int radar_prop                   = 1;
    int screen                       = 1;
    int lights                       = 1;
    int obstacle_cleaner             = 0;
    int object_loader                = 0;
    int object_unloader              = 1;
    int object_compacter             = 0;
    IPAddress assignedIP;
    ActionResult lastActionResult;
    int isRequiredReset              = 0;
    int isRequiredStop               = 0;
    int isCarMoving                  = 0;
    Radar_t radar;
    TrunkState_t trunkState          = TRUNK_IDLE;
    unsigned long trunkStartTime     = 0;
    int moveTrunkCmd                 = -1;
    int buzzer                       = 0;
    Adafruit_SSD1306 display;
    TwoWire radarWire;

    Beta_t() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
               radarWire(1)
    {}
} Beta_t;

void setupMotors();
void checkMotorTimeouts();
int parseMotorID(Motors_t motor_id);

void setupWiFi(); 

void setupOled(); 
void oledPrint(const char* message);

void initDebugLEDs();
void updateDebugLEDs();

// hw_timer_t *heartbeatTimer = NULL;
// void setupHeartbeatTimer();
// void IRAM_ATTR onHeartbeatTimer();
void logMessage(ErrorSeverity_t severity, const char* message); // flexible function to print messages with specific formats or different methods
void rebootTimerCallback(TimerHandle_t xTimer);


extern const char* logSeverityLabels[];
extern const DC_Motor_Config_t motor_pins[];
extern const int NUM_MOTORS;
extern Motor_Timeout_t motorTimeouts[];

#endif