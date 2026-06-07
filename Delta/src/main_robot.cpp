#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "main_robot.h"

static VL53L0X radar_sensor;

#define MAIN_FSM_SETUP       10
#define MAIN_FSM_IDLE        20
#define MAIN_FSM_CONNECTING  30
#define MAIN_FSM_ERROR       1000
#define MAIN_FSM_FATAL_ERROR 1010

#define SERVO_MIN_US  1000
#define SERVO_MAX_US  2000
#define PROP_SET(VAR, TYPE, VAL) \
    if (xSemaphoreTake(self_robot_prop_setter, pdMS_TO_TICKS(40)) == pdTRUE) { \
        VAR = (TYPE)VAL; \
        xSemaphoreGive(self_robot_prop_setter); \
        return 0; \
    } else { \
        return -1; \
    }

typedef enum {
    MOTORID_ERR         = -1,
    MOTORID_RES         = 0,
    MOTORID_WHEEL_RIGHT = 1,
    MOTORID_WHEEL_LEFT  = 2,
    MOTORID_RADAR_SERVO = 3,
    MOTORID_CLAW       = 4,
    MOTORID_LEVER      = 5,
} LocalMotors_t;

static TimerHandle_t motorStopTimer = NULL;
static TaskHandle_t  udp_server_task_handle = NULL;

RobotState_t      self_robot_state;
SemaphoreHandle_t self_robot_state_sem;
SemaphoreHandle_t self_robot_prop_setter;

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

static int last_claw_angle  = CLAW_ANGLE_MAX;
static int last_lever_angle = LEVER_ANGLE_MAX;

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

// ---- property setters----
int self_prop_set_robot_state(int new_value) {
    if (xSemaphoreTake(self_robot_state_sem, pdMS_TO_TICKS(20)) == pdTRUE) {
        self_robot_state = (RobotState_t)new_value;
        xSemaphoreGive(self_robot_state_sem);
        return 0;
    } else {
        return -1; /* failed to acquire state semaphore */
    }
}
int self_prop_set_speed(int new_value)             { PROP_SET(prop_speed, SpeedLevel_t, new_value); }
int self_prop_set_feedback(int new_value)          { PROP_SET(prop_feedback, FeedbackLevel_t, new_value); }
int self_prop_set_debug(int new_value)             { PROP_SET(prop_debug, DebugLevel_t, new_value); }
int self_prop_set_navigation_type(int new_value)   { PROP_SET(prop_navigation_type, NavigationType_t, new_value); }
int self_prop_set_route_policy(int new_value)      { PROP_SET(prop_route_policy, RoutePolicy_t, new_value); }
int self_prop_set_radar(int new_value)             { PROP_SET(prop_radar, int, new_value); }
int self_prop_set_screen(int new_value)            { PROP_SET(prop_screen, int, new_value); }
int self_prop_set_obstacle_cleaner(int new_value)  { PROP_SET(prop_obstacle_cleaner, int, new_value); }
int self_prop_set_object_loader(int new_value)     { PROP_SET(prop_object_loader, int, new_value); }
int self_prop_set_object_unloader(int new_value)   { PROP_SET(prop_object_unloader, int, new_value); }
int self_prop_set_object_compacter(int new_value)  { PROP_SET(prop_object_compacter, int, new_value); }

static void dc_motor_stop(uint8_t in1, uint8_t in2);

// ---- state commands ----

int self_hard_reset()     { 
    esp_restart();
    return 0; 
}
int self_soft_reset()     {
    self_prop_set_robot_state(RobotState_IDLE);
    return 0;
 }
int self_emergency_stop() {
    dc_motor_stop(CH_WHEEL_R_IN1, CH_WHEEL_R_IN2);
    dc_motor_stop(CH_WHEEL_L_IN3, CH_WHEEL_L_IN4);
    self_prop_set_robot_state(RobotState_ERR);
    return 0;
 }

// ---- low-level motor helpers ----

static LocalMotors_t motorIdToEnum(Motors_t motorId) {
    switch (motorId) {
        case Motors_RES:     return MOTORID_RES;
        case Motors_MOT1:    return MOTORID_WHEEL_RIGHT;
        case Motors_MOT2:    return MOTORID_WHEEL_LEFT;
        case Motors_MOT3:    return MOTORID_RADAR_SERVO;
        case Motors_MOT4:    return MOTORID_CLAW;
        case Motors_MOT5:    return MOTORID_LEVER;
        case Motors_END_MOT:
        default:             return MOTORID_ERR;
    }
}

static void dc_motor_stop(uint8_t ch1, uint8_t ch2) {
    ledcWrite(ch1, 0);
    ledcWrite(ch2, 0);
}

static void motorStopCallback(TimerHandle_t xTimer) {
    uint32_t chs = (uint32_t)pvTimerGetTimerID(xTimer);
    dc_motor_stop((chs >> 8) & 0xFF, chs & 0xFF);
}

static void dc_motor_start(uint8_t ch1, uint8_t ch2, int dir, uint8_t duty_pct, int duration_ms) {
    uint32_t duty = (uint32_t)((duty_pct * 255) / 100);
    if (dir > 0) {
        ledcWrite(ch1, duty);
        ledcWrite(ch2, 0);
    } else {
        ledcWrite(ch1, 0);
        ledcWrite(ch2, duty);
    }
    if (duration_ms > 0) {
        uint32_t chs = ((uint32_t)ch1 << 8) | ch2;
        if (motorStopTimer != NULL) {
            xTimerStop(motorStopTimer, 0);
            xTimerDelete(motorStopTimer, 0);
        }
        motorStopTimer = xTimerCreate("mStop", pdMS_TO_TICKS(duration_ms), pdFALSE, (void*)chs, motorStopCallback);
        if (motorStopTimer != NULL) xTimerStart(motorStopTimer, 0);
    }
}

// ---- motion API ----

int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration) {
    LocalMotors_t motor = motorIdToEnum(motor_id);
    switch (motor) {
        case MOTORID_WHEEL_RIGHT:
            switch (direction) {
                case Direction_FORWARD:  dc_motor_start(CH_WHEEL_R_IN1, CH_WHEEL_R_IN2,  1, speed, duration); break;
                case Direction_BACKWARD: dc_motor_start(CH_WHEEL_R_IN1, CH_WHEEL_R_IN2, -1, speed, duration); break;
                case Direction_STOP:     dc_motor_stop(CH_WHEEL_R_IN1, CH_WHEEL_R_IN2);  break;
                default: break;
            }
            break;
        case MOTORID_WHEEL_LEFT:
            switch (direction) {
                case Direction_FORWARD:  dc_motor_start(CH_WHEEL_L_IN3, CH_WHEEL_L_IN4,  1, speed, duration); break;
                case Direction_BACKWARD: dc_motor_start(CH_WHEEL_L_IN3, CH_WHEEL_L_IN4, -1, speed, duration); break;
                case Direction_STOP:     dc_motor_stop(CH_WHEEL_L_IN3, CH_WHEEL_L_IN4);  break;
                default: break;
            }
            break;
        default:
            break;
    }
    return 0;
}

// ---- servo API ----
uint32_t angle_to_duty(int angle) {
    uint32_t us = SERVO_MIN_US + ((uint32_t)angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180;
    return (us * 65536) / 20000;
}
int self_motion_open_claw(int angle) {
    if (angle < CLAW_ANGLE_MIN || angle > CLAW_ANGLE_MAX) return -1;
    ledcWrite(CH_CLAW_SERVO, angle_to_duty(angle));
    delay(500);
    ledcWrite(CH_CLAW_SERVO, 0);
    last_claw_angle = angle;
    return 0;
}
int self_motion_move_lever(int angle) {
    if (angle < LEVER_ANGLE_MIN || angle > LEVER_ANGLE_MAX) return -1;
    int current = last_lever_angle;
    int step = (angle > current) ? 1 : -1;
    while (current != angle) {
        current += step;
        ledcWrite(CH_LEVER_SERVO, angle_to_duty(current));
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    delay(500);
    ledcWrite(CH_LEVER_SERVO, 0);
    last_lever_angle = angle;
    return 0;
}
// ---- motion stubs (to be implemented) ----

int self_motion_stop_motor(Motors_t motor_id)                               { return 0; }
int self_motion_steer_servo(Motors_t motor_id, int angle) {
    if (motorIdToEnum(motor_id) != MOTORID_RADAR_SERVO) return -1;
    if (angle < RADAR_ANGLE_MIN || angle > RADAR_ANGLE_MAX) return -1;
    ledcWrite(CH_RADAR_SERVO, angle_to_duty(angle));
    return 0;
}
int self_motion_car_rotate(Direction_t direction)                           { return 0; }
int self_motion_car_proceed(Direction_t direction)                          { return 0; }
int self_motion_car_stop()                                                  { return 0; }

// ---- radar API ----

static bool radar_ready = false;

int self_radar_init() {
    Wire.begin(PIN_RADAR_SENSOR_SDA, PIN_RADAR_SENSOR_SCL);
    radar_sensor.setTimeout(500);
    if (!radar_sensor.init()) {
        radar_ready = false;
        return -1;
    }
    radar_sensor.startContinuous();
    radar_ready = true;
    return 0;
}

int self_radar_read_distance() {
    if (!radar_ready) return -1;
    uint16_t mm = radar_sensor.readRangeContinuousMillimeters();
    if (radar_sensor.timeoutOccurred() || mm == 65535) return -1;
    return (int)mm;
}

int self_radar_scan(RadarReading_t *out_readings, int max_readings) {
    if (!radar_ready || out_readings == NULL || max_readings <= 0) return -1;
    int count = 0;
    for (int angle = RADAR_ANGLE_MIN; angle <= RADAR_ANGLE_MAX && count < max_readings; angle += RADAR_SCAN_STEP) {
        ledcWrite(CH_RADAR_SERVO, angle_to_duty(angle));
        vTaskDelay(pdMS_TO_TICKS(80)); // settle time for servo + sensor
        int dist = self_radar_read_distance();
        out_readings[count].angle       = angle;
        out_readings[count].distance_mm = dist;
        count++;
    }
    // return servo to center after scan
    ledcWrite(CH_RADAR_SERVO, angle_to_duty((RADAR_ANGLE_MIN + RADAR_ANGLE_MAX) / 2));
    vTaskDelay(pdMS_TO_TICKS(300));
    ledcWrite(CH_RADAR_SERVO, 0);
    return count;
}

// ---- display API ----

#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 oled(128, 32, &Wire1, -1);

int self_display_init() {
    Wire1.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) return -1;
    oled.clearDisplay();
    oled.display();
    return 0;
}

void self_display_show(const char* line1, const char* line2) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);  oled.println(line1);
    oled.setCursor(0, 16); oled.println(line2);
    oled.display();
}

// ---- Arduino entry points (excluded from motor_test build) ----

#ifndef MOTOR_TEST
#include <WiFi.h>

static const char* state_to_str(RobotState_t s) {
    switch (s) {
        case RobotState_IDLE: return "IDLE";
        case RobotState_BUSY: return "BUSY";
        case RobotState_ERR:  return "ERROR";
        default:              return "?";
    }
}

static void display_update() {
    self_display_show(
        state_to_str((RobotState_t)self_prop_get_robot_state()),
        WiFi.localIP().toString().c_str()
    );
}

static void robot_server_task(void *arg) {
    (void)arg;
    RobotStartServer();
    vTaskDelete(NULL);
}

void setup() {
    self_robot_state_sem  = xSemaphoreCreateMutex();
    self_robot_prop_setter = xSemaphoreCreateMutex();
    Serial.begin(SERIAL_BAUD);

    if (self_display_init() != 0) {
        Serial.println("[OLED] init failed");
    } else {
        display_update();
    }

    ledcSetup(CH_RADAR_SERVO, 50,   16); ledcAttachPin(PIN_RADAR_SERVO,        CH_RADAR_SERVO);
    ledcSetup(CH_CLAW_SERVO,  50,   16); ledcAttachPin(PIN_CLAW_SERVO,         CH_CLAW_SERVO);
    ledcSetup(CH_LEVER_SERVO, 50,   16); ledcAttachPin(PIN_LEVER_SERVO,        CH_LEVER_SERVO);
    ledcSetup(CH_WHEEL_R_IN1, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN1, CH_WHEEL_R_IN1);
    ledcSetup(CH_WHEEL_R_IN2, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN2, CH_WHEEL_R_IN2);
    ledcSetup(CH_WHEEL_L_IN3, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN3, CH_WHEEL_L_IN3);
    ledcSetup(CH_WHEEL_L_IN4, 1000,  8); ledcAttachPin(PIN_WHEELS_DRIVER_IN4, CH_WHEEL_L_IN4);

    //set initial position of claw and lever
    ledcWrite(CH_CLAW_SERVO,  angle_to_duty(CLAW_ANGLE_MAX));   // open position at boot
    ledcWrite(CH_LEVER_SERVO, angle_to_duty(LEVER_ANGLE_MAX));  // raised position at boot  

    pinMode(PIN_WHEELS_DRIVER_SLEEP, OUTPUT);
    digitalWrite(PIN_WHEELS_DRIVER_SLEEP, HIGH);

    if (self_radar_init() != 0) {
        Serial.println("[RADAR] VL53L0X init failed");
    }
}

void loop() {
    static uint32_t last_display_ms = 0;
    uint32_t now = millis();
    if (now - last_display_ms >= 2000) {
        last_display_ms = now;
        display_update();
    }

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
