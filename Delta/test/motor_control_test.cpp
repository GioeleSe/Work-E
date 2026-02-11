// On-device test that parses motor-control JSONs and drives motors.
// Build with PlatformIO and open Serial to observe steps.

#include <Arduino.h>
#include <ArduinoJson.h>
#include "communication.h"
#include "motor_control.h"
static void applyMotorCommand(Direction dir, uint8_t speed, uint32_t durationMs){
  MotorControlPayload mcp;
  mcp.motor_ids = nullptr; // no explicit motor ids -> car mode
  mcp.direction = dir;
  mcp.speed = speed;
  mcp.angle = 0;
  mcp.duration = durationMs;

  Serial.printf("Driving motors: dir=%d speed=%u duration=%lu\n",
                (int)dir, speed, (unsigned long)durationMs);
  executeMotorControl(mcp);
  delay(durationMs);

  // stop after each command for safety
  MotorControlPayload stop = mcp;
  stop.speed = 0;
  stop.direction = STOP;
  stop.duration = 0;
  executeMotorControl(stop);
  delay(200);
}

void setup(){
  Serial.begin(115200);
  while(!Serial) delay(10);
  Serial.println("Starting motor control forward/backward loop test");

  initMotors();
}

void loop(){
  // Drive forward 1s
  MotorControlPayload mcp;
  mcp.motor_ids = nullptr;
  mcp.direction = FORWARD;
  mcp.speed = 80;
  mcp.angle = 0;
  mcp.duration = 0;
  Serial.println("Forward 1s");
  executeMotorControl(mcp);
  delay(1000);

  // Stop / pause 1s
  Serial.println("Pause 1s");
  MotorControlPayload stop = mcp;
  stop.speed = 0;
  stop.direction = STOP;
  executeMotorControl(stop);
  delay(1000);

  // Drive backward 1s
  Serial.println("Backward 1s");
  mcp.direction = BACKWARD;
  mcp.speed = 80;
  executeMotorControl(mcp);
  delay(1000);

  // Stop / pause 1s
  Serial.println("Pause 1s");
  executeMotorControl(stop);
  delay(1000);
}
