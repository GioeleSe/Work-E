//#include "config.h"
#include "communication.h"
#include "navigation.h"
#include "motor_control.h"
#include <ArduinoJson.h>
#include <cstring>
#include <WiFi.h>
#include <WiFiUdp.h>

const int LEFT_OUTPUT_PIN = 7; // connected to in 1
const int RIGHT_OUTPUT_PIN = 2; // connected to in 2
const int PWM_FREQ = 500;     
const int PWM_RESOLUTION = 8; 
// requires board clock to be able to run at 500Hz * 2^8 = 128kHz
const int DELAY_MS = 10;

const int MAX_DUTY_CYCLE = (int)(pow(2, PWM_RESOLUTION) - 1);

void initMotors() {
  // initialize motor control pins, set to default state
  ledcAttachChannel(RIGHT_OUTPUT_PIN, PWM_FREQ, PWM_RESOLUTION, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // right motor start off
  ledcAttachChannel(LEFT_OUTPUT_PIN, PWM_FREQ, PWM_RESOLUTION, LEFT_CHANNEL);
  ledcWrite(LEFT_CHANNEL, 0); // left motor start off
}
void executeMotorControl(const MotorControlPayload& mcp){
printf("Executing motor control: speed=%d, angle=%d, duration=%d, ", mcp.speed, mcp.angle, mcp.duration);
  if(mcp.motor_ids == nullptr){
    printf("motor ids: none, direction: %d\n", mcp.direction);
    if
  }   
return;
}