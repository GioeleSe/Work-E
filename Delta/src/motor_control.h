#include <Arduino.h>
#include "config.h"
#include <ArduinoJson.h>
#include <cstring>
#define MOT_A1_PIN 12
#define MOT_A2_PIN 13
#define MOT_B1_PIN 5
#define MOT_B2_PIN 4
#define ULT_PIN 4 // To enable sleep mode

void initMotors();
void stopMotors();
void executeMotorControl(const MotorControlPayload& mcp);