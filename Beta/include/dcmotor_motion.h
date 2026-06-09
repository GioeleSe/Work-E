#ifndef DCMOTOR_MOTION_H
#define DCMOTOR_MOTION_H

#include "Beta.h"
#include "main_robot.h"
#include "common_platform_abstr.h"

void setupMotors();
void checkMotorTimeouts();
int  parseMotorID(Motors_t motor_id);

#endif