#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "driver/ledc.h"
#include "esp_system.h"
#include "robot_server.h"
#include "main_robot.h"

#define MAIN_FSM_SETUP       10
#define MAIN_FSM_IDLE        20
#define MAIN_FSM_CONNECTING  30
#define MAIN_FSM_ERROR       1000
#define MAIN_FSM_FATAL_ERROR 1010

#define TRUNK_OPEN_TIME (2000 * 100)
#define TRUNK_IS_OPEN() (digitalRead(PIN_TRUNK_SWITCH) == LOW)

typedef enum {
    MOTORID_ERR         = -1,
    MOTORID_RES         = 0,
    MOTORID_WHEEL_RIGHT = 1,
    MOTORID_WHEEL_LEFT  = 2,
    MOTORID_RADAR_SERVO = 3,
    MOTORID_TRUNK       = 4
} LocalMotors_t;

static TimerHandle_t motorStopTimer = NULL;
static TaskHandle_t  udp_server_task_handle = NULL;

RobotState_t      self_robot_state;
SemaphoreHandle_t self_robot_state_sem;

static SpeedLevel_t     prop_speed           = SpeedLevel_NORMAL;
static DebugLevel_t     prop_debug           = DebugLevel_BASIC;
static FeedbackLevel_t  prop_feedback        = FeedbackLevel_DEBUG;
static NavigationType_t prop_navigation_type = NavigationType_MANUAL;
static RoutePolicy_t    prop_route_policy    = RoutePolicy_SHORTEST;
static int prop_radar            = 1;
static int prop_screen           = 1;
static int prop_obstacle_cleaner = 1;
static int prop_object_loader    = 1;
static int prop_object_unloader  = 1;
static int prop_object_compacter = 1;

// ---- property getters ----

int self_prop_get_robot_id() {
#ifdef SELF_ROBOT_ID
    return SELF_ROBOT_ID;
#else
    return 0;
#endif
}

int self_prop_get_robot_state() {
    int val = -1;
    if (xSemaphoreTake(self_robot_state_sem, pdMS_TO_TICKS(20)) == pdTRUE) {
        val = (int)self_robot_state;
        xSemaphoreGive(self_robot_state_sem);
    }
    return val;
}

int self_prop_get_speed()            { return (int)prop_speed; }
int self_prop_get_feedback()         { return (int)prop_feedback; }
int self_prop_get_debug()            { return (int)prop_debug; }
int self_prop_get_navigation_type()  { return (int)prop_navigation_type; }
int self_prop_get_route_policy()     { return (int)prop_route_policy; }
int self_prop_get_radar()            { return prop_radar; }
int self_prop_get_screen()           { return prop_screen; }
int self_prop_get_obstacle_cleaner() { return prop_obstacle_cleaner; }
int self_prop_get_object_loader()    { return prop_object_loader; }
int self_prop_get_object_unloader()  { return prop_object_unloader; }
int self_prop_get_object_compacter() { return prop_object_compacter; }

// ---- property setters (to be implemented) ----

int self_prop_set_speed(int new_value)             { return 0; }
int self_prop_set_feedback(int new_value)          { return 0; }
int self_prop_set_debug(int new_value)             { return 0; }
int self_prop_set_navigation_type(int new_value)   { return 0; }
int self_prop_set_route_policy(int new_value)      { return 0; }
int self_prop_set_radar(int new_value)             { return 0; }
int self_prop_set_screen(int new_value)            { return 0; }
int self_prop_set_obstacle_cleaner(int new_value)  { return 0; }
int self_prop_set_object_loader(int new_value)     { return 0; }
int self_prop_set_object_unloader(int new_value)   { return 0; }
int self_prop_set_object_compacter(int new_value)  { return 0; }

// ---- state commands (to be implemented) ----

int self_hard_reset()     { return 0; }
int self_soft_reset()     { return 0; }
int self_emergency_stop() { return 0; }

// ---- low-level motor helpers ----

static LocalMotors_t motorIdToEnum(Motors_t motorId) {
    switch (motorId) {
        case Motors_RES:     return MOTORID_RES;
        case Motors_MOT1:    return MOTORID_WHEEL_RIGHT;
        case Motors_MOT2:    return MOTORID_WHEEL_LEFT;
        case Motors_MOT3:    return MOTORID_RADAR_SERVO;
        case Motors_MOT4:    return MOTORID_TRUNK;
        case Motors_END_MOT:
        default:             return MOTORID_ERR;
    }
}

static void dc_motor_stop(uint8_t in1, uint8_t in2) {
    ledcWrite(in1, 0);
    ledcWrite(in2, 0);
}

static void motorStopCallback(TimerHandle_t xTimer) {
    uint32_t pins = (uint32_t)pvTimerGetTimerID(xTimer);
    dc_motor_stop((pins >> 8) & 0xFF, pins & 0xFF);
}

static void dc_motor_start(uint8_t in1, uint8_t in2, int dir, uint8_t duty_pct, int duration_ms) {
    uint32_t duty = (uint32_t)((duty_pct * 255) / 100);
    if (dir > 0) {
        ledcWrite(in1, duty);
        ledcWrite(in2, 0);
    } else {
        ledcWrite(in1, 0);
        ledcWrite(in2, duty);
    }
    if (duration_ms > 0) {
        uint32_t pins = ((uint32_t)in1 << 8) | in2;
        if (motorStopTimer != NULL) {
            xTimerStop(motorStopTimer, 0);
            xTimerDelete(motorStopTimer, 0);
        }
        motorStopTimer = xTimerCreate("mStop", pdMS_TO_TICKS(duration_ms), pdFALSE, (void*)pins, motorStopCallback);
        if (motorStopTimer != NULL) xTimerStart(motorStopTimer, 0);
    }
}

static void dc_motor_wake(uint8_t sleep_pin) {
    digitalWrite(sleep_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1));
}

static void dc_motor_sleep(uint8_t sleep_pin, uint8_t in1, uint8_t in2) {
    dc_motor_stop(in1, in2);
    digitalWrite(sleep_pin, LOW);
}

// ---- motion API ----

int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration) {
    LocalMotors_t motor = motorIdToEnum(motor_id);
    switch (motor) {
        case MOTORID_TRUNK:
            dc_motor_wake(PIN_TRUNK_DRIVER_SLEEP);
            switch (direction) {
                case Direction_FORWARD:
                    if (!TRUNK_IS_OPEN()) {
                        int capped = ((duration * speed) < TRUNK_OPEN_TIME) ? duration : (TRUNK_OPEN_TIME / speed);
                        dc_motor_start(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2, 1, speed, capped);
                    } else {
                        dc_motor_stop(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2);
                    }
                    break;
                case Direction_BACKWARD:
                    if (TRUNK_IS_OPEN()) {
                        dc_motor_start(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2, -1, speed, duration);
                        do { vTaskDelay(pdMS_TO_TICKS(1)); } while (TRUNK_IS_OPEN());
                        dc_motor_stop(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2);
                    }
                    break;
                default:
                    break;
            }
            dc_motor_sleep(PIN_TRUNK_DRIVER_SLEEP, PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2);
            break;
        case MOTORID_WHEEL_RIGHT:
            switch (direction) {
                case Direction_FORWARD:  dc_motor_start(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2,  1, speed, duration); break;
                case Direction_BACKWARD: dc_motor_start(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2, -1, speed, duration); break;
                case Direction_STOP:     dc_motor_stop(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2);  break;
                default: break;
            }
            break;
        case MOTORID_WHEEL_LEFT:
            switch (direction) {
                case Direction_FORWARD:  dc_motor_start(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4,  1, speed, duration); break;
                case Direction_BACKWARD: dc_motor_start(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4, -1, speed, duration); break;
                case Direction_STOP:     dc_motor_stop(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4);  break;
                default: break;
            }
            break;
        default:
            break;
    }
    return 0;
}

// ---- motion stubs (to be implemented) ----

int self_motion_stop_motor(Motors_t motor_id)                               { return 0; }
int self_motion_steer_servo(Motors_t motor_id, int angle)                   { return 0; }
int self_motion_car_rotate(Direction_t direction)                           { return 0; }
int self_motion_car_proceed(Direction_t direction)                          { return 0; }
int self_motion_car_stop()                                                  { return 0; }

// ---- Arduino entry points (excluded from motor_test build) ----

#ifndef MOTOR_TEST

static void robot_server_task(void *arg) {
    (void)arg;
    RobotStartServer();
    vTaskDelete(NULL);
}

void setup() {
    self_robot_state_sem = xSemaphoreCreateMutex();
    Serial.begin(SERIAL_BAUD);

    ledcAttach(PIN_RADAR_SERVO,       5000, 8);
    ledcAttach(PIN_TRUNK_DRIVER_IN1,  1000, 8);
    ledcAttach(PIN_TRUNK_DRIVER_IN2,  1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN1, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN2, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN3, 1000, 8);
    ledcAttach(PIN_WHEELS_DRIVER_IN4, 1000, 8);

    pinMode(PIN_WHEELS_DRIVER_SLEEP, OUTPUT);
    digitalWrite(PIN_WHEELS_DRIVER_SLEEP, HIGH);
    pinMode(PIN_TRUNK_DRIVER_SLEEP, OUTPUT);
    pinMode(PIN_TRUNK_SWITCH, INPUT);
}

void loop() {
    if (udp_server_task_handle == NULL) {
        BaseType_t result = xTaskCreatePinnedToCore(
            robot_server_task, "udp_server", 8192, NULL, 1, &udp_server_task_handle, 0);
        if (result != pdPASS) {
            Serial.println("[UDP] Failed to create task");
        } else {
            Serial.println("[UDP] Task created successfully");
        }
    }
}

#endif // MOTOR_TEST
