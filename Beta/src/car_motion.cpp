#include "car_motion.h"
#include "main_robot.h"
#include "Beta.h"

extern Beta_t robot_beta;

int self_motion_car_rotate(Direction_t direction)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "function self_motion_car_rotate called");
    int res_left = 0, res_right = 0;
    switch(direction){
        case Direction_t::Direction_LEFT:
            res_left = self_motion_activate_dc_motor(MOTOR_CAR_LEFT, Direction_t::Direction_BACKWARD, robot_beta.speed, CAR_ROTATE_TIME);
            res_right = self_motion_activate_dc_motor(MOTOR_CAR_RIGHT, Direction_t::Direction_FORWARD, robot_beta.speed, CAR_ROTATE_TIME);
        break;
        case Direction_t::Direction_RIGHT:
            res_left = self_motion_activate_dc_motor(MOTOR_CAR_LEFT, Direction_t::Direction_FORWARD, robot_beta.speed, CAR_ROTATE_TIME);
            res_right = self_motion_activate_dc_motor(MOTOR_CAR_RIGHT, Direction_t::Direction_BACKWARD, robot_beta.speed, CAR_ROTATE_TIME);
        break;
        default:
            logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "invalid direction read by car_rotate, ignoring movement");
            return -1;
        break;
    }
    if((res_left < 0) || (res_right <0)){
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "car was unable to rotate, activate_dc_motors returned an error");
        return -2;
    }
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "car motors activated successfully by car_rotate");
    return 0;

}

int self_motion_car_proceed(Direction_t direction)
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "function self_motion_car_proceed called");
    if((direction != Direction_t::Direction_BACKWARD) && (direction != Direction_t::Direction_FORWARD)){
        logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "invalid direction received by car_proceed");
        return -1;
    }

    int res_left = self_motion_activate_dc_motor(MOTOR_CAR_LEFT, direction, robot_beta.speed, 0);
    int res_right = self_motion_activate_dc_motor(MOTOR_CAR_RIGHT, direction, robot_beta.speed, 0);
    if((res_left < 0) || (res_right <0)){
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "car was unable to proceed, activate_dc_motors returned an error");
        return -1;
    }

    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "car motors activated successfully by car_proceed");
    return 0;
}

int self_motion_car_stop()
{
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "function self_motion_car_stop called");
    int res = self_motion_stop_motor(MOTOR_CAR_LEFT);
    res = self_motion_stop_motor(MOTOR_CAR_RIGHT);
    if(!res){
        logMessage(ErrorSeverity_t::ErrorSeverity_t_HIGH, "car was unable to stop. stop_motor returned an error");
        return -1;
    }
    
    logMessage(ErrorSeverity_t::ErrorSeverity_t_LOW, "car motors deactivated successfully by car_stop");
    return 0;
}