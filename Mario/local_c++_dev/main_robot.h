#ifndef MAIN_ROBOT_H
#define MAIN_ROBOT_H

int self_prop_get_robot_id();

int self_prop_get_speed();
int self_prop_get_feedback();
int self_prop_get_debug();
int self_prop_get_navigation_type();
int self_prop_get_route_policy();
int self_prop_get_radar();
int self_prop_get_screen();
int self_prop_get_obstacle_cleaner();
int self_prop_get_object_loader();
int self_prop_get_object_unloader();
int self_prop_get_object_compacter();

#endif