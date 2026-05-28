#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "robot_server.h"

#define MAIN_FSM_SETUP 10
#define MAIN_FSM_IDLE 20
#define MAIN_FSM_CONNECTING 30
#define MAIN_FSM_SENDING_FEEDBACK 40
#define MAIN_FSM_SENDING_DEBUG 900
#define MAIN_FSM_ERROR 1000
#define MAIN_FSM_FATAL_ERROR 1010

#define PIN_BUZZER 4
#define PIN_RADAR_SERVO 13
#define PIN_RADAR_SENSOR_SDA 26
#define PIN_RADAR_SENSOR_SCL 27
#define PIN_LED_GREEN 14
#define PIN_LED_RED 15
#define PIN_OLED_SDA 33
#define PIN_OLED_SCL 32
#define PIN_WHEELS_DRIVER_SLEEP 16
#define PIN_WHEELS_DRIVER_IN1 25
#define PIN_WHEELS_DRIVER_IN2 23 // doublecheck these ones
#define PIN_WHEELS_DRIVER_IN3 22 // doublecheck these ones
#define PIN_WHEELS_DRIVER_IN4 21
#define PIN_TRUNK_DRIVER_SLEEP 17
#define PIN_TRUNK_DRIVER_IN1 19 // doublecheck these ones
#define PIN_TRUNK_DRIVER_IN2 18 // doublecheck these ones
#define PIN_TRUNK_SWITCH 34


typedef enum{
    MOTORID_ERR = -1,
    MOTORID_RES = 0,
    MOTORID_WHEEL_RIGHT = 1,
    MOTORID_WHEEL_LEFT = 2,
    MOTORID_RADAR_SERVO = 3,
    MOTORID_TRUNK = 4
} LocalMotors_t;
#define TRUNK_OPEN_TIME 2000*100                                    // open time is 2.0s at speed = 100
#define TRUNK_IS_OPEN() (digitalRead(PIN_TRUNK_SWITCH)==LOW)        // defined here cause it's easier to change the high/low logic
TimerHandle_t motorStopTimer = NULL;


TaskHandle_t udp_server_task_handle = NULL;                         // can check if the task is still running, pausing or stopping it and stuff.


RobotState_t self_robot_state; // robot state is protected as might be changed by crashing ISR or another thread
SemaphoreHandle_t self_robot_state_sem;

SpeedLevel_t prop_speed = SpeedLevel_t::SpeedLevel_NORMAL;
DebugLevel_t prop_debug = DebugLevel_t::DebugLevel_BASIC;
FeedbackLevel_t prop_feedback = FeedbackLevel_t::FeedbackLevel_DEBUG;
NavigationType_t prop_navigation_type = NavigationType_t::NavigationType_MANUAL;
RoutePolicy_t prop_route_policy = RoutePolicy_SHORTEST;
// activation properties used as bool. Still faster this way with no cast needed
int prop_radar = 1;
int prop_screen = 1;
int prop_obstacle_cleaner = 1;
int prop_object_loader = 1;
int prop_object_unloader = 1;
int prop_object_compacter = 1;

int self_prop_get_robot_id()
{
#ifdef SELF_ROBOT_ID
    return SELF_ROBOT_ID;
#else
    return 0; // no robot id case -> neutral value
#endif;
}

// The function returns the current robot state using the proper semaphore.
// Returns -1 if no platform is defined.
int self_prop_get_robot_state()
{
    int robot_state_value = -1;
    if (xSemaphoreTake(self_robot_state_sem, (TickType_t)20) == pdTRUE)
    { // wait 20 ticks if semaphore is not available
        robot_state_value = (int)self_robot_state;
    }
    xSemaphoreGive(self_robot_state_sem);
    return robot_state_value;
}

// Inline functions just for space, when other files are calling they get the same overhead
static inline int self_prop_get_speed() { return (int)prop_speed; }
static inline int self_prop_get_debug() { return (int)prop_debug; }
static inline int self_prop_get_feedback() { return (int)prop_feedback; }
static inline int self_prop_get_navigation_type() { return (int)prop_navigation_type; }
static inline int self_prop_get_route_policy() { return (int)prop_route_policy; }
static inline int self_prop_get_radar() { return prop_radar; }
static inline int self_prop_get_screen() { return prop_screen; }
static inline int self_prop_get_obstacle_cleaner() { return prop_obstacle_cleaner; }
static inline int self_prop_get_object_loader() { return prop_object_loader; }
static inline int self_prop_get_object_unloader() { return prop_object_unloader; }
static inline int self_prop_get_object_compacter() { return prop_object_compacter; }



// Little helper function to map the motor IDs to the locally-assigned ones
LocalMotors_t motorIdToEnum(Motors_t motorId) {
    switch (motorId) {
        case Motors_RES:  
            return LocalMotors_t::MOTORID_RES;
        case Motors_MOT1:  
            return LocalMotors_t::MOTORID_WHEEL_RIGHT;
        case Motors_MOT2:   
            return LocalMotors_t::MOTORID_WHEEL_LEFT;
        case Motors_MOT3:
            return LocalMotors_t::MOTORID_RADAR_SERVO;
        case Motors_MOT4:
            return LocalMotors_t::MOTORID_TRUNK;
        case Motors_END_MOT:
        default:
            return LocalMotors_t::MOTORID_ERR;
    }
}

void dc_motor_stop(uint8_t in1, uint8_t in2) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
}
void motorStopCallback(TimerHandle_t xTimer) {
    // retrieve the pins from the timer ID
    uint32_t pins = (uint32_t)pvTimerGetTimerID(xTimer);
    uint8_t in1 = (pins >> 8) & 0xFF;
    uint8_t in2 = (pins) & 0xFF;
    dc_motor_stop(in1, in2);
}
void dc_motor_start(uint8_t in1, uint8_t in2, int dir, uint8_t duty, int duration) {
    uint32_t pwm = (uint32_t)((duty * 255) / 100.0);
    if (dir) {
        ledcWrite(in1, pwm);
        digitalWrite(in2, LOW);
    } else {
        digitalWrite(in1, LOW);
        ledcWrite(in2, pwm);
    }
    if (duration > 0) {
        uint32_t pins = ((uint32_t)in1 << 8) | in2;                 // pack both pins into the timer ID to pass them to the callback
        if (motorStopTimer != NULL) {
            xTimerStop(motorStopTimer, 0);
            xTimerDelete(motorStopTimer, 0);
        }
        motorStopTimer = xTimerCreate("motorStop", pdMS_TO_TICKS(duration), pdFALSE, (void*)pins, motorStopCallback);
        if (motorStopTimer != NULL){
            xTimerStart(motorStopTimer, 0);
        }
    }
}
void dc_motor_wake(uint8_t sleep_pin) {
    digitalWrite(sleep_pin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(1));                                   // 1ms given to the driver to activate
}

void dc_motor_sleep(uint8_t sleep_pin, uint8_t in1, uint8_t in2) {
    dc_motor_stop(in1, in2);
    digitalWrite(sleep_pin, LOW);
}
// Start the specified motor in the wanted direction.
// For DC motors the activation is given by the driver activation (with a stop callback if duration is set != 0)
// The DC rotating direction can be Direction_t::Direction_BACKWARD or Direction_t::Direction_FORWARD (using pseudocode this is: (pin A, pin B)=(1,0) or (pin A, pin B)=(0,1))
// The DC speed can be tuned with a pwm signal on the HIGH ping. This looks something like (pin A, pin B)=(analogWrite((speed*(255))/100),0). That's the mapping of percentage to a pwm value 0-256 (expected parameter for analog write)
int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration){
    LocalMotors_t motor = motorIdToEnum(motor_id);
    switch(motor){
        case LocalMotors_t::MOTORID_TRUNK:
            dc_motor_wake(PIN_TRUNK_DRIVER_SLEEP);
            switch(direction){
                case Direction_t::Direction_FORWARD:
                    if(!TRUNK_IS_OPEN()){
                        // open the trunk with the specified speed (for a specific MAX time as there's no switch on the opening side)
                        dc_motor_start(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2, 1, speed, (((duration*speed)<TRUNK_OPEN_TIME)? duration : (TRUNK_OPEN_TIME/speed)));
                    }else{
                        dc_motor_stop(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2); // avoid opening the trunk if it's already open. No assurance that the position to reach is safe
                    }
                break;
                case Direction_t::Direction_BACKWARD:
                    // close the trunk (for the specified time or until the switch triggers, whoever comes first)
                    if(TRUNK_IS_OPEN()){
                        dc_motor_start(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2, -1, speed, duration);
                    }
                    do{
                        vTaskDelay(pdMS_TO_TICKS(1));
                    }while(TRUNK_IS_OPEN());
                    dc_motor_stop(PIN_TRUNK_DRIVER_IN1, PIN_TRUNK_DRIVER_IN2);
                break;
            }
            dc_motor_wake(PIN_TRUNK_DRIVER_SLEEP);
        break;
        case LocalMotors_t::MOTORID_WHEEL_RIGHT:
            switch(direction){
                case Direction_t::Direction_FORWARD:
                    dc_motor_start(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2, 1, speed, duration);
                break;
                case Direction_t::Direction_BACKWARD:
                    dc_motor_start(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2, -1, speed, duration);
                break;
                case Direction_t::Direction_STOP:
                    dc_motor_stop(PIN_WHEELS_DRIVER_IN1, PIN_WHEELS_DRIVER_IN2);
                break;
            }
        break;
        case LocalMotors_t::MOTORID_WHEEL_LEFT:
            switch(direction){
                case Direction_t::Direction_FORWARD:
                    dc_motor_start(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4, 1, speed, duration);
                break;
                case Direction_t::Direction_BACKWARD:
                    dc_motor_start(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4, -1, speed, duration);
                break;
                case Direction_t::Direction_STOP:
                    dc_motor_stop(PIN_WHEELS_DRIVER_IN3, PIN_WHEELS_DRIVER_IN4);
                break;
            }
        break;
        default:
        // Activations of other motors non implemented.
        // Send back an error and debug messages
        break;
    }
}


int setup()
{
    assert(defined(UDP_SERVER_H));
    assert(defined(ROBOT_SERVER_H));
    
    assert(self_robot_state_sem != NULL);
    self_robot_state_sem = xSemaphoreCreateMutex();

    Serial.begin(SERIAL_BAUD);
}
int loop()
{
    int main_state = MAIN_FSM_SETUP;

    switch(main_state){
        case MAIN_FSM_SETUP:
        // set up of functionalities:
        // - pin assignment
        // - general variables init
        // - oled (separate task)
        // - radar (separate task)
            ledcAttach(PIN_RADAR_SERVO, 5000, 8);                       // pwm for servo set at 5kHz
            ledcAttach(PIN_TRUNK_DRIVER_IN1, 1000, 8);                  // chill pwm for the drv8833
            ledcAttach(PIN_TRUNK_DRIVER_IN2, 1000, 8);
            pinMode(PIN_TRUNK_DRIVER_SLEEP, OUTPUT);
            pinMode(PIN_TRUNK_SWITCH, INPUT);                           // no pulldown, already external 10k resistor
            
            main_state = MAIN_FSM_CONNECTING;
        break;
        case MAIN_FSM_CONNECTING:
        // Connection to the server with local upd listener. Lower-level details are handled on separate modules.
        // The server is created and managed using robot_server module. To start it, calling its main entry is sufficient.
            BaseType_t result = xTaskCreatePinnedToCore(RobotStartServer, "udp_server", 8192, NULL, 1, &udp_server_task_handle, 0);
            if (result != pdPASS || udpTaskHandle == NULL) {
                Serial.println("[UDP] Failed to create task");
                main_state = MAIN_FSM_FATAL_ERROR;
            } else {
                Serial.println("[UDP] Task created successfully");
                main_state = MAIN_FSM_IDLE;
            }
        break;
        case MAIN_FSM_IDLE:
        // Main loop waiting for a new command (radar and oled might be still working)
        break;
        // case MAIN_FSM_SENDING_FEEDBACK:
        // // Feedback can be sent using functionalities from udp_client modules.
        // // In its module the main entry to use is client_send_packet(msg, msg_size).
        // // Note: at least 1 feedback packet is already sent back by robot_server module at the end of each packet handler.
        // break;
        // case MAIN_FSM_SENDING_DEBUG:
        // // Debug can be sent using the same method as for the feedback, the one from udp_client modules.
        // // Use client_send_packet(msg, msg_size) with an encoded debug structured packet.
        // break;
        case MAIN_FSM_ERROR:
        // A NON-fatal error caused by the sensors or the internal board (probably by the wires, my bad) can be signaled here.
        // An example can be the OLED screen not recognized or some anomalies from the distance sensor.
        break;
        case MAIN_FSM_FATAL_ERROR:
        // You guessed it, this one is for serious errors.
        // An example can be the server connection not working even after the specified attempts or some thread-related issue (unrecoverable ones).
        // Simply send a fatal error message with the description of the issue and reboot the board.
        break;
        default:
        // Unrecognized state, might be caused by corruption or concurrent access to the same register.
        break;
    }
}
