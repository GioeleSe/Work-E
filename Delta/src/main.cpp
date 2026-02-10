#include <Arduino.h>
#include "config.h"
#include "communication.h"
#include <cstring>
#include <WiFi.h>
#include <AsyncUDP.h>

AsyncUDP udp;
IPAddress serverIP; 

void setup() {
    Serial.begin(115200);
  Serial.println("Starting ESP32 AsyncUDP Server with listen() method...");
  initializeWiFi();
  startUDPServer();
}

void loop() {
  // put your main code here, to run repeatedly:
  commsLoop();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}