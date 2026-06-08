#ifndef CAR_MOTION_H
#define CAR_MOTION_H

#include "Beta.h"
#include "main_robot.h"
#include "common_platform_abstr.h"

int self_motion_car_rotate(Direction_t direction);
int self_motion_car_proceed(Direction_t direction);
int self_motion_car_stop();

#endif