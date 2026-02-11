//#include "config.h"
#include "communication.h"
#include "navigation.h"
#include "motor_control.h"
#include <ArduinoJson.h>
#include <cstring>
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>


const int PWM_FREQ = 500;     
const int PWM_RESOLUTION = 8; 
// requires board clock to be able to run at 500Hz * 2^8 = 128kHz
const int DELAY_MS = 10;

const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1);

void initMotors() {
  // initialize motor control pins, set to default state
  pinMode(MOT_A1_PIN, OUTPUT);
  pinMode(MOT_A2_PIN, OUTPUT);
  pinMode(MOT_B1_PIN, OUTPUT);
  pinMode(MOT_B2_PIN, OUTPUT);
    pinMode(ULT_PIN, OUTPUT);
    // Ensure PWM range matches our duty computation on ESP8266
#ifdef ESP8266
    analogWriteRange(MAX_DUTY_CYCLE);
#endif
  // Turn off motors - Initial state
  digitalWrite(MOT_A1_PIN, LOW);
  digitalWrite(MOT_A2_PIN, LOW);
  digitalWrite(MOT_B1_PIN, LOW);
  digitalWrite(MOT_B2_PIN, LOW);
}

void spinClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed)
{
    digitalWrite(ULT_PIN, HIGH);
    analogWrite(MOTOR_PIN1, speed);
    digitalWrite(MOTOR_PIN2, LOW);
}

void spinAntiClockwise(int MOTOR_PIN1, int MOTOR_PIN2, int speed)
{
    digitalWrite(ULT_PIN, HIGH);
    digitalWrite(MOTOR_PIN1, LOW);
    analogWrite(MOTOR_PIN2, speed);
}
void stopMotors()
{
    digitalWrite(ULT_PIN, LOW);
    digitalWrite(MOT_A1_PIN, LOW);
    digitalWrite(MOT_A2_PIN, LOW);
    digitalWrite(MOT_B1_PIN, LOW);
    digitalWrite(MOT_B2_PIN, LOW);
}
void executeMotorControl(const MotorControlPayload& mcp){
printf("Executing motor control: speed=%d, angle=%d, duration=%d, ", mcp.speed, mcp.angle, mcp.duration);
      int duty = (mcp.speed * MAX_DUTY_CYCLE) / 100;

    if(mcp.motor_ids == nullptr){
        printf("motor ids: none, direction: %d\n", mcp.direction);
        switch (mcp.direction) {
            case FORWARD:
                spinClockwise(MOT_A1_PIN, MOT_A2_PIN, duty);
                spinClockwise(MOT_B1_PIN, MOT_B2_PIN, duty);
                break;
            case BACKWARD:
                spinAntiClockwise(MOT_A1_PIN, MOT_A2_PIN, duty);
                spinAntiClockwise(MOT_B1_PIN, MOT_B2_PIN, duty);
                break;
            case LEFT:
                spinClockwise(MOT_A1_PIN, MOT_A2_PIN, duty);
                spinAntiClockwise(MOT_B1_PIN, MOT_B2_PIN, duty);
                break;
            case RIGHT:
                spinAntiClockwise(MOT_A1_PIN, MOT_A2_PIN, duty);
                spinClockwise(MOT_B1_PIN, MOT_B2_PIN, duty);
                break;
            case STOP:
                stopMotors();
                break;
            default:
                // unknown
                break;
        }
  } else {
  }   
return;
}