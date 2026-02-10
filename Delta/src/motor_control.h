
#include <Arduino.h>
#include <ArduinoJson.h>
#include "communication.h"
#include "navigation.h"
#include "motor_control.cpp"


void initmotors();
void executeMotorControl(const MotorControlPayload& mcp);