#ifndef MAIN_ROBOT_H
#define MAIN_ROBOT_H
#include "robot_server.h"

// communication baud for serial speed matching
#define SERIAL_BAUD 115200
// ID assigned in docs, ranging from 1 to 4
#define SELF_ROBOT_ID 1

// Hardware pin assignments
#define PIN_BUZZER              4
#define PIN_RADAR_SERVO         13
#define PIN_RADAR_SENSOR_SDA    26
#define PIN_RADAR_SENSOR_SCL    27
#define PIN_LED_GREEN           14
#define PIN_LED_RED             15
#define PIN_OLED_SDA            33
#define PIN_OLED_SCL            32
#define PIN_WHEELS_DRIVER_SLEEP 16
#define PIN_WHEELS_DRIVER_IN1   25
#define PIN_WHEELS_DRIVER_IN2   23
#define PIN_WHEELS_DRIVER_IN3   22
#define PIN_WHEELS_DRIVER_IN4   21
#define PIN_TRUNK_DRIVER_SLEEP  17
#define PIN_TRUNK_DRIVER_IN1    19
#define PIN_TRUNK_DRIVER_IN2    18
#define PIN_TRUNK_SWITCH        34



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

#endif