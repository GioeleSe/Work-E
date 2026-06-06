#include "Beta.h"
#include "main_robot.h"
#include "common_platform_abstr.h"
#include "Adafruit_SSD1306"
// #include "UDP_Client.h"
// #include "UDP_Server.h"

// TODO: Low to High abstraction functions
// + define robot public properties with getter and setter
// - choose the reset and stop callbacks
// - move motor with timeout, direction and speed
// - move servo motor in the specific radar angle
// - read the distance in a matrix with tuples (angle, distance)
//
// - define a radar function to populate the matrix with a single command
// - move the trunk until the closed-button is active (openTrunk, closeTrunk and isOpenTrunk functions)
// -
//
// - use LEDs for low-level debugging (1 for power and 1 for state, according to frequency can be good, bad or error)
// - use OLED for high level debugging
// - activate the buzzer with a nice tone when using trunk

// properties defined in Beta.h, here only getter and setter
extern Beta_t robot_beta;
// extern int SELF_ROBOT_ID  // defined with its directive
extern RobotState_t     robot_state;
extern int              speed;
extern int              feedback;
extern int              debug;
extern NavigationType_t navigation_type;
extern RoutePolicy_t    route_policy;
extern int              radar;
extern int              screen;
extern int              obstacle_cleaner;
extern int              object_loader;
extern int              object_unloader;
extern int              object_compacter;

extern const char* logSeverityLabels;
extern Adafruit_SSD1306 display;

int self_prop_get_robot_id() { return SELF_ROBOT_ID; }
int self_prop_get_robot_state() { return (int)robot_state; }
int self_prop_get_speed() { return speed; }
int self_prop_get_feedback() { return feedback; }
int self_prop_get_debug() { return debug; }
int self_prop_get_navigation_type() { return (int)navigation_type; }
int self_prop_get_route_policy() { return (int)route_policy; }
int self_prop_get_radar() { return radar; }
int self_prop_get_screen() { return screen; }
int self_prop_get_obstacle_cleaner() { return obstacle_cleaner; }
int self_prop_get_object_loader() { return object_loader; }
int self_prop_get_object_unloader() { return object_unloader; }
int self_prop_get_object_compacter() { return object_compacter; }

int self_prop_set_robot_state(RobotState_t val) { robot_state = val; }
int self_prop_set_speed(int val) { speed = val; }
int self_prop_set_feedback(int val) { feedback = val; }
int self_prop_set_debug(int val) { debug = val; }
int self_prop_set_navigation_type(NavigationType_t val) { navigation_type = val; }
int self_prop_set_route_policy(RoutePolicy_t val) { route_policy = val; }
int self_prop_set_radar(int val) { radar = val; }
int self_prop_set_screen(int val) { screen = val; }
int self_prop_set_obstacle_cleaner(int val) { obstacle_cleaner = val; }
int self_prop_set_object_loader(int val) { object_loader = val; }
int self_prop_set_object_unloader(int val) { object_unloader = val; }
int self_prop_set_object_compacter(int val) { object_compacter = val; }

// messages printing functions
void logMessage(ErrorSeverity_t severity, const char* message){
#ifdef SERIAL_PRINT
    Serial.print("[");
    Serial.print(logSeverityLabels[severity]);
    Serial.print("] ");
    Serial.println(message);
#endif
}

void oledInit() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setClock(400000);  // 400kHz refresh for Oled responsiveness
   if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        robot_beta.screen = 0;  // screen state update (failed -> force status to 0)
        return;
    }
    
    display.clearDisplay();
    display.display();
    robot_beta.screen = 1;
}

void oledPrint(const char* message){
    if (robot_beta.screen == 0) return; 

    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.print(message);
    display.display();
}

// direct reset with log attempt only, not the gentle one
int self_hard_reset(){
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Hard Reset...");
    ESP.restart();
    return 0;
}

void rebootTimerCallback(TimerHandle_t xTimer) {
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Timer expired. Executing Hard Reset...");
    ESP.restart();
}

// warn the tasks that the reboot is going to happen
int self_soft_reset() {
    logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Soft reset initiated");
    if(robot_beta.isRequiredStop){
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "Soft reset - Emergency Stop deactivated");
        oledPrint("Emergency Stop cleared");
        return 0;
    }

    robot_beta.isRequiredReset = 1;
    oledPrint("Soft Reset in 2s");

    TimerHandle_t rebootTimer = xTimerCreate(
        "RebootTimer", 
        pdMS_TO_TICKS(2000), 
        pdFALSE,    // run one time only
        (void*)0,   // timer id set to 0
        rebootTimerCallback
    );

    if (rebootTimer != NULL) {
        xTimerStart(rebootTimer, 0);
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "Reboot scheduled in 2 seconds.");
        return 0;
    } else {
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "Timer creation failed! Forcing hard reset."); // fallback for freertos timer failure
        ESP.restart(); 
        return -1;  // not really needed as it's restarting
    }
}

// set the stop flag to warn the motor-controlling functions
int self_emergency_stop(){
    robot_beta.isRequiredStop = 1;      // will be cleared with soft reset, the motor movements will be prevented by this flag 
    logMessage(ErrorSeverity_t::ErrorSeverity_t_MID, "Emergency Stop required");
    oledPrint("Emergency Stop");
}


// dc and servo motors control
/*
 * DC MOTOR LOGIC:
 *   IN_1 IN_2 -> Direction
 *   Both LOW -> stop
 *   Both HIGH -> stop
 *   LOW HIGH -> clockwise
 *   HIGH LOW -> counterclockwise
 */

// int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration);
// int self_motion_stop_motor(Motors_t motor_id);
// int self_motion_steer_servo(Motors_t motor_id, int angle);
// int self_motion_car_rotate(Direction_t direction);
// int self_motion_car_proceed(Direction_t direction);
// int self_motion_car_stop();