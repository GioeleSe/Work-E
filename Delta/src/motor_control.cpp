#include "config.h"
#include "communication.h"
#include "navigation.h"
#include "motor_control.h"
#include <ArduinoJson.h>
#include <cstring>
#include <WiFi.h>
#include <WiFiUdp.h>

void initMotors() {
  // initialize motor control pins, set to default state
  
}
void executeMotorControl(const MotorControlPayload& mcp){
  // for now just print the params, in the future this will interface with the actual motor control code
//   Serial.print("Executing motor control: speed=");
//   Serial.print(mcp.speed);
//   Serial.print(", angle=");
//   Serial.print(mcp.angle);
//   Serial.print(", duration=");
//   Serial.print(mcp.duration);
//   Serial.print(", motor_ids=");
//   if (mcp.motor_ids != nullptr){
//     for (JsonVariant v : mcp.motor_ids) {
//       Serial.print(v.as<int>());
//       Serial.print(" ");
//     }
//   } else {
//     Serial.print("N/A");
//     Serial.print(", direction=");
//     Serial.print(mcp.direction);
//   }
//   Serial.println();
printf("Executing motor control: speed=%d, angle=%d, duration=%d, ", mcp.speed, mcp.angle, mcp.duration);
   return;
}