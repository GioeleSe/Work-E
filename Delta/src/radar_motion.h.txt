#ifndef RADAR_MOTION_H
#define RADAR_MOTION_H

#include "Beta.h"
#include "main_robot.h"
#include "common_platform_abstr.h"
#include <ESP32Servo.h>
#include <VL53L0X.h>

void setupRadar();
void internalSetupServo();
void internalSetupDistanceSensor();
void moveRadarToAngle(int target_angle);
void tickRadarScan();

#endif