#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

#include <Arduino.h>

// Motor config pair
typedef struct {
    uint8_t pinA;
    uint8_t pinB;
} DC_Motor_Config_t;

// Motor timeout tracking
typedef struct {
    bool isActive;
    unsigned long stopTime;
} Motor_Timeout_t;

#endif