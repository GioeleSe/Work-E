#include "Beta.h"
#include "robot_types.h"
#include "main_robot.h"
#include "common_platform_abstr.h"
#include "car_motion.h"
#include "trunk_motion.h"
#include "radar_motion.h"
#include "dcmotor_motion.h"

#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h> // pololu lib (lighter than adafruit one)

Beta_t robot_beta;

// TODO: Low to High abstraction functions
// + define robot public properties with getter and setter
// + choose the reset and stop callbacks
// + move motor with timeout, direction and speed
// + move servo motor in the specific radar angle
// + read the distance to recognize obstacles in front of the car
//
// + define a radar function to populate the matrix with a single command
// + move the trunk until the closed-button is active
// + move the robot as a car    
//
// - use LEDs for low-level debugging (1 for power and 1 for state, according to frequency can be good, bad or error)
// - use OLED for high level debugging
// - activate the buzzer with a nice tone when using trunk

// properties defined in Beta.h, here only getter and setter


const char* logSeverityLabels[] = { "INFO", "WARN", "ERROR" };
const DC_Motor_Config_t motor_pins[] = {
    {0, 0},
    {PIN_DRIVER_1_MOTOR_1A, PIN_DRIVER_1_MOTOR_1B},
    {PIN_DRIVER_1_MOTOR_2A, PIN_DRIVER_1_MOTOR_2B},
    {PIN_DRIVER_2_MOTOR_1A, PIN_DRIVER_2_MOTOR_1B},
    {0, 0}
};
const int NUM_MOTORS = sizeof(motor_pins) / sizeof(DC_Motor_Config_t);
Motor_Timeout_t motorTimeouts[NUM_MOTORS] = {0};


int self_prop_get_robot_id() { return robot_beta.robotID; }
int self_prop_get_robot_state() { return (int)(robot_beta.robot_state); }
int self_prop_get_speed() { return robot_beta.speed; }
int self_prop_get_feedback() { return robot_beta.feedback; }
int self_prop_get_debug() { return robot_beta.debug; }
int self_prop_get_navigation_type() { return (int)(robot_beta.navigation_type); }
int self_prop_get_route_policy() { return (int)(robot_beta.route_policy); }
int self_prop_get_radar() { return robot_beta.radar_prop; }
int self_prop_get_screen() { return robot_beta.screen; }
int self_prop_get_obstacle_cleaner() { return robot_beta.obstacle_cleaner; }
int self_prop_get_object_loader() { return robot_beta.object_loader; }
int self_prop_get_object_unloader() { return robot_beta.object_unloader; }
int self_prop_get_object_compacter() { return robot_beta.object_compacter; }

int self_prop_get_lights() { return robot_beta.lights; }
int self_prop_set_lights(int new_value) { robot_beta.lights = new_value; return 0;}

int self_prop_set_robot_state(int new_value) { robot_beta.robot_state = (RobotState_t)new_value; return 0;}
int self_prop_set_speed(int new_value) { robot_beta.speed = new_value; return 0;}
int self_prop_set_feedback(int new_value) { robot_beta.feedback = new_value; return 0;}
int self_prop_set_debug(int new_value) { robot_beta.debug = new_value; return 0;}
int self_prop_set_navigation_type(int new_value) { robot_beta.buzzer = new_value; return 0;} // setter reused for buzzer functionality
int self_prop_set_route_policy(int new_value) { robot_beta.route_policy = (RoutePolicy_t)new_value; return 0;}
int self_prop_set_radar(int new_value) { robot_beta.radar_prop = new_value; return 0;}
int self_prop_set_screen(int new_value) { robot_beta.screen = new_value; return 0;}
int self_prop_set_obstacle_cleaner(int new_value) { robot_beta.obstacle_cleaner = new_value; return 0;}
int self_prop_set_object_loader(int new_value) { robot_beta.object_loader = new_value; return 0;}
int self_prop_set_object_unloader(int new_value) { robot_beta.moveTrunkCmd = new_value; return 0;}
int self_prop_set_object_compacter(int new_value) { robot_beta.object_compacter = new_value; return 0;}


void setupWiFi()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "starting wifi setup");
    IPAddress staticIP(192, 168, 137, 3); // static config valid for my hotspot, dynamic WifiManager setup if used in another network
    IPAddress gateway(192, 168, 137, 1);
    IPAddress netMask(255, 255, 255, 0);
    IPAddress dns(192, 168, 137, 1);

    WiFiManager wm;
    wm.setConfigPortalTimeout(120);
    // wm.setSTAStaticIPConfig(staticIP, gateway, netMask, dns);
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "connecting WiFiManager");

    if (wm.autoConnect("ESP32_WIFI_Config"))
    { // try last saved credentials, generate AP ESP32_WIFI_Config if invalid
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "WiFiManager setup done successfully. IP Address:");
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, WiFi.localIP().toString().c_str());
        delay(500);
    }
    else
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "no valid WiFi connection found. Resetting in 5s");
        delay(5000);
        ESP.restart();
    }
}

// start the Wire object with OLED I2c pins.
// if the device is not found the variable screen is forced to 0 to prevent screen usage attempts
void setupOled()
{
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setClock(400000); // 400kHz refresh for Oled responsiveness
    if (!robot_beta.display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println("SSD1306 allocation failed");
        robot_beta.screen = 0; // screen state update (failed -> force status to 0)
        return;
    }

    robot_beta.display.clearDisplay();
    robot_beta.display.display();
    robot_beta.screen = 1;
}
void oledPrint(const char *message)
{
    if (!robot_beta.screen)
    return;

    robot_beta.display.clearDisplay();
    robot_beta.display.setCursor(0, 0);
    robot_beta.display.setTextSize(1);
    robot_beta.display.setTextColor(SSD1306_WHITE);
    robot_beta.display.print(message);
    robot_beta.display.display();
}

// void setupHeartbeatTimer()
// {
//     heartbeatTimer = timerBegin(0, 80, true); // Timer 0, Clock divider 80
//     timerAttachInterrupt(heartbeatTimer, &onHeartbeatTimer, true);
//     timerAlarmWrite(heartbeatTimer, 2000000, true); // Trigger every 2 seconds
//     timerAlarmEnable(heartbeatTimer);
// }

// void IRAM_ATTR onHeartbeatTimer() /// Heartbeat timer ISR
// {
//     sendMessage(heartbeatMessage());
//     return;
// }

void logMessage(ErrorSeverity_t severity, const char *message)
{
    Serial.print("[");
    Serial.print(logSeverityLabels[severity]);
    Serial.print("] ");
    Serial.println(message);
}

void rebootTimerCallback(TimerHandle_t xTimer)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Timer expired. Executing Hard Reset...");
    ESP.restart();
}

void setupBuzzer(){
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "buzzer setup done");
}

// plays a short triple-beep sequence when robot_beta.buzzer is set to 1
// blocking during the beep sequence, then resets the flag to 0
void updateBuzzer()
{
    if (!robot_beta.buzzer) return;

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "buzzer: playing beep sequence");

    for (int i = 0; i < BUZZER_BEEP_COUNT; i++)
    {
        digitalWrite(PIN_BUZZER, HIGH);
        delay(BUZZER_BEEP_ON_MS);
        digitalWrite(PIN_BUZZER, LOW);
        delay(BUZZER_BEEP_OFF_MS);
    }

    robot_beta.buzzer = 0; // reset flag after sequence
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "buzzer: sequence done");
}


// direct reset with log attempt only, not the gentle one
int self_hard_reset()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Hard Reset...");
    ESP.restart();
    return 0;
}
// warn the tasks that the reboot is going to happen
int self_soft_reset()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Soft reset initiated");
    if (robot_beta.isRequiredStop)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "Soft reset - Emergency Stop deactivated");
        oledPrint("Emergency Stop cleared");
        return 0;
    }

    robot_beta.isRequiredReset = 1;
    oledPrint("Soft Reset in 2s");

    TimerHandle_t rebootTimer = xTimerCreate(
        "RebootTimer",
        pdMS_TO_TICKS(2000),
        pdFALSE,   // run one time only
        (void *)0, // timer id set to 0
        rebootTimerCallback);

    if (rebootTimer != NULL)
    {
        xTimerStart(rebootTimer, 0);
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "Reboot scheduled in 2 seconds");
        return 0;
    }
    else
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Timer creation failed! Forcing hard reset"); // fallback for freertos timer failure
        ESP.restart();
        return -1; // not really needed as it's restarting
    }
}

// set the stop flag to warn the motor-controlling functions
int self_emergency_stop()
{
    robot_beta.isRequiredStop = 1; // will be cleared with soft reset, the motor movements will be prevented by this flag
    logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "Emergency Stop required");
    oledPrint("Emergency Stop");
    return 0;
}

int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW,
               "self_motion_activate_dc_motor: ENTER");

    // -------------------------
    // Motor ID debug
    // -------------------------
    char buf[128];

    snprintf(buf, sizeof(buf),
             "motor_id=%d direction=%d speed=%d duration=%d",
             motor_id, direction, speed, duration);

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);

    if (parseMotorID(motor_id) < 0)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH,
                   "parseMotorID FAILED -> aborting motor command");
        return -1;
    }

    uint8_t pinA = motor_pins[motor_id].pinA;
    uint8_t pinB = motor_pins[motor_id].pinB;

    snprintf(buf, sizeof(buf),
             "motor pins: A=%u B=%u",
             pinA, pinB);

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);

    if (pinA == 0 && pinB == 0)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH,
                   "motor has no pin assignment -> abort");
        return -2;
    }

    // -------------------------
    // PWM calculation debug
    // -------------------------
    speed = constrain(speed, 0, 100);
    int pwm_value = (speed * 255) / 100;

    snprintf(buf, sizeof(buf),
             "clamped speed=%d pwm_value=%d",
             speed, pwm_value);

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);

    // -------------------------
    // STOP condition
    // -------------------------
    if (pwm_value == 0 || direction == Direction_t::Direction_STOP)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_MID,
                   "STOP condition triggered -> writing 0 to both pins");

        ledcWrite(pinA, 0);
        ledcWrite(pinB, 0);

        return 0;
    }

    // -------------------------
    // Direction logic debug
    // -------------------------
    if (direction == Direction_t::Direction_FORWARD)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW,
                   "direction = FORWARD");

        ledcWrite(pinA, pwm_value);
        ledcWrite(pinB, 0);
    }
    else if (direction == Direction_t::Direction_BACKWARD)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW,
                   "direction = BACKWARD");

        ledcWrite(pinA, 0);
        ledcWrite(pinB, pwm_value);
    }
    else
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH,
                   "INVALID direction -> abort");
        return -3;
    }

    // -------------------------
    // Timeout debug
    // -------------------------
    if (duration > 0)
    {
        motorTimeouts[motor_id].isActive = true;
        motorTimeouts[motor_id].stopTime = millis() + duration;

        snprintf(buf, sizeof(buf),
                 "timeout ENABLED stopTime=%lu",
                 (unsigned long)motorTimeouts[motor_id].stopTime);

        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, buf);
    }
    else
    {
        motorTimeouts[motor_id].isActive = false;

        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW,
                   "timeout DISABLED");
    }

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW,
               "self_motion_activate_dc_motor: EXIT OK");

    return 0;
}
int self_motion_stop_motor(Motors_t motor_id)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "function self_motion_stop_motor called");
    if (parseMotorID(motor_id) < 0)
    {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "invalid motor id, returning error from stop_motor");
        return -1;
    }else if(motor_id >= 1 && motor_id <= 3){
        ledcWrite(motor_pins[motor_id].pinA, 0);
        ledcWrite(motor_pins[motor_id].pinB, 0);
    }else if(motor_id == 4){
        motorTimeouts[MOTOR_RADAR].isActive = false; // can only force the flag to 0 since servo is moving on angle-request only
        motorTimeouts[MOTOR_RADAR].stopTime = 0;
    }
    return 0;
}
int self_motion_steer_servo(Motors_t motor_id, int angle)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "function self_motion_steer_servo called");
    if(motor_id != MOTOR_RADAR){
        return -1;
    }

    if(robot_beta.radar.motorServo.attached()){
        moveRadarToAngle(angle);
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "servo moved to the specified angle");
        return 0;
    }else{
        return -2;
    }
}
