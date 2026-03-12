#ifndef MAIN_ROBOT_H
#define MAIN_ROBOT_H

// should be defined as a global constant in the robot main code
int self_prop_get_robot_id();

// return an integer index of the enum RobotState_t
int self_prop_get_robot_status();


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

#endif