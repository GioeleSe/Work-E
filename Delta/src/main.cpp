#include <Arduino.h>
#include "config.h"
#include "communication.h"
#include <cstring>
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>

#ifndef EXCLUDE_FROM_MOTOR_TEST
void setup() {
    Serial.begin(115200);
  Serial.println("Starting ESP32 AsyncUDP Server with listen() method...");
  initializeWiFi();
  startUDPServer();
}

void loop() {
  commsLoop();
}
#endif

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}