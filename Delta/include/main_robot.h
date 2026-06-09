#ifndef MAIN_ROBOT_H
#define MAIN_ROBOT_H
#include "robot_types.h"

// communication baud for serial speed matching
#define SERIAL_BAUD 115200
// ID assigned in docs, ranging from 1 to 4
#define SELF_ROBOT_ID 2

// LEDC channel assignments for DC wheel motors (timers 1 and 2, 1000 Hz)
// Servos are driven by ESP32Servo on timers 0 and 3 — no overlap.
#define CH_WHEEL_R_IN1   2
#define CH_WHEEL_R_IN2   3
#define CH_WHEEL_L_IN3   4
#define CH_WHEEL_L_IN4   5

// Hardware pin assignments
#define PIN_BUZZER              4
#define PIN_RADAR_SERVO         19
#define PIN_CLAW_SERVO          13
#define PIN_LEVER_SERVO         18
#define PIN_RADAR_SENSOR_SDA    27
#define PIN_RADAR_SENSOR_SCL    26
#define PIN_LED_GREEN           14
#define PIN_LED_RED             15
#define PIN_OLED_SDA            33
#define PIN_OLED_SCL            32
#define PIN_WHEELS_DRIVER_SLEEP 16
#define PIN_WHEELS_DRIVER_IN1   25
#define PIN_WHEELS_DRIVER_IN2   23
#define PIN_WHEELS_DRIVER_IN3   22
#define PIN_WHEELS_DRIVER_IN4   21

// Radar servo sweep limits
#define RADAR_ANGLE_MIN  0
#define RADAR_ANGLE_MAX  180
#define RADAR_SCAN_STEP  5     // degrees between readings for blocking scan
#define RADAR_MAX_READINGS ((RADAR_ANGLE_MAX - RADAR_ANGLE_MIN) / RADAR_SCAN_STEP + 1)

// Non-blocking tick scan parameters
#define RADAR_CONE_DEG      60   // total cone width swept per tick cycle
#define RADAR_STEP_DEG      10   // degrees between readings in cone
#define RADAR_SAMPLES        7   // readings per cycle (CONE_DEG / STEP_DEG + 1)
#define RADAR_OBSTACLE_MM  400   // distance threshold for obstacle detection

typedef struct {
    int     angle;
    int     distance_mm;
} RadarReading_t;





// should be defined as a global constant in the robot main code
int self_prop_get_robot_id();

// return an integer index of the enum RobotState_t
int self_prop_get_robot_state();


// return integer representation of property speed (actual value, casted bool or enum index)
int self_prop_get_speed();
// return integer representation of property feedback (actual value, casted bool or enum index)
int self_prop_get_feedback();
// return integer representation of property debug (actual value, casted bool or enum index)
int self_prop_get_debug();
// return integer representation of property navigation_type (actual value, casted bool or enum index)
int self_prop_get_navigation_type();
// return integer representation of property route_policy (actual value, casted bool or enum index)
int self_prop_get_route_policy();
// return integer representation of property radar (actual value, casted bool or enum index)
int self_prop_get_radar();
// return integer representation of property screen (actual value, casted bool or enum index)
int self_prop_get_screen();
// return integer representation of property obstacle_cleaner (actual value, casted bool or enum index)
int self_prop_get_obstacle_cleaner();
// return integer representation of property object_loader (actual value, casted bool or enum index)
int self_prop_get_object_loader();
// return integer representation of property object_unloader (actual value, casted bool or enum index)
int self_prop_get_object_unloader();
// return integer representation of property object_compacter (actual value, casted bool or enum index)
int self_prop_get_object_compacter();

// Set a new value for property speed.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_speed(int new_value);
// Set a new value for property feedback.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_feedback(int new_value);
// Set a new value for property debug.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_debug(int new_value);
// Set a new value for property navigation_type.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_navigation_type(int new_value);
// Set a new value for property route_policy.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_route_policy(int new_value);
// Set a new value for property radar.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_radar(int new_value);
// Set a new value for property screen.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_screen(int new_value);
// Set a new value for property obstacle_cleaner.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_obstacle_cleaner(int new_value);
// Set a new value for property object_loader.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_object_loader(int new_value);
// Set a new value for property object_unloader.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_object_unloader(int new_value);
// Set a new value for property object_compacter.
// The new value is an integer representation (enum index, actual value or, i.e. casted bool to int)
int self_prop_set_object_compacter(int new_value);

// Perform a reboot of the board
// (gracefully stop the active process/task and reboot)
int self_hard_reset();

// Clear error flags or continue after an emergency stop
int self_soft_reset();                                              

// stop any active process/task and trigger the error state
int self_emergency_stop();

// Start the specified motor in the wanted direction.
// For DC motors the activation is given by the driver activation (with a stop callback if duration is set != 0)
// The DC rotating direction can be Direction_t::Direction_BACKWARD or Direction_t::Direction_FORWARD (using pseudocode this is: (pin A, pin B)=(1,0) or (pin A, pin B)=(0,1))
// The DC speed can be tuned with a pwm signal on the HIGH ping. This looks something like (pin A, pin B)=(analogWrite((speed*(255))/100),0). That's the mapping of percentage to a pwm value 0-256 (expected parameter for analog write)
int self_motion_activate_dc_motor(Motors_t motor_id, Direction_t direction, int speed, int duration);

// Stop the specified motor.
// Just stop it by setting (pin A, pin B)=[(1,1) | (0,0)])
int self_motion_stop_motor(Motors_t motor_id);

// Turn the specified motor (expected to be a Servo) of the wanted angle parameter.
// It should be easly mapped to the Servo library function servo.write(angle)
// (speed is ignored, just turn it as you want)
int self_motion_steer_servo(Motors_t motor_id, int angle);
// robot Delta specific functions to control claw and lever
int self_motion_open_claw(int angle);
int self_motion_move_lever(int angle);
uint32_t angle_to_duty(int angle);

// Turn the robot to the specified direction value.
// No timing, no motor ids, just turn it as you want.
// Note: in the body of this the function self_motion_activate_dc_motor should be reused with an empirical measured duration
int self_motion_car_rotate(Direction_t direction);

// Make the robot go forward or backward according to the specified direction value.
// This command will activate the wheel motors in the correct direction with the **internal** speed.
// Here the speed parameter is ignored in car driving, the robot is expected to use the **internal** speed property.
// Note: the robot is expected to stop only when self_motion_car_stop() is called.
int self_motion_car_proceed(Direction_t direction);

// Simply stop the wheel motors
// Note: in the body of this the function self_motion_stop_motor should be reused.
int self_motion_car_stop();

#define CLAW_SERVO_MIN_US    500   // pulse width at 0°
#define CLAW_SERVO_MAX_US   2400   // pulse width at 180°
#define CLAW_SERVO_STEP_US    53   // ~5° per step
#define LEVER_SERVO_MIN_US   500   // pulse width at 0°
#define LEVER_SERVO_MAX_US  2400   // pulse width at 180°
#define LEVER_SERVO_STEP_US   53   // ~5° per step

#define CLAW_ANGLE_MIN   25   // open position
#define CLAW_ANGLE_MAX   90   // closed position
#define LEVER_ANGLE_MIN  5   // raised position  — tune upward if still oscillating
#define LEVER_ANGLE_MAX  74  // lowered position — tune downward if not low enough

// Initialize the SSD1306 OLED — call once in setup()
int self_display_init();

// Write two lines of text to the OLED
void self_display_show(const char* line1, const char* line2);

// Initialize radar servo (ESP32Servo) and VL53L0X sensor — call once in setup()
int self_radar_init();

// Read a single distance measurement in mm. Returns -1 on failure.
int self_radar_read_distance();

// Non-blocking scan tick — call every loop(). Sweeps a RADAR_CONE_DEG cone
// around the current servo position in RADAR_SAMPLES steps, then updates
// self_radar_obstacle_detected().
void self_radar_tick();

// Returns true if the last completed tick scan found an obstacle within RADAR_OBSTACLE_MM.
bool self_radar_obstacle_detected();

// Blocking full sweep from RADAR_ANGLE_MIN to RADAR_ANGLE_MAX. For testing only.
int self_radar_scan(RadarReading_t *out_readings, int max_readings);

#endif