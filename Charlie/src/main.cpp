// Charlie ^^


#include<Arduino.h> // millis(), Serial and other core functions
#include<WiFi.h>
#include<ArduinoJson.h> // JSON parsing and construction
#include<AsyncUDP.h> // asynchronous UDP communication



#include"configs.h" // credentials and settings
#include"CharlieUDP.h" // UDP communication and message handling
#include"tasks.h" // task scheduling
#include"fsm.h"


void setup(){
  setupSerial();
  setupWiFi();
  syncTimeNTP();
  setupUDP();
  setupFSM();
  setupTasks();
}




void loop(){

  unsigned long now = millis();
  
  if (taskReady(&heartbeatTask, now)) {
    sendHeartbeat(state);
    Serial.println("Heartbeat: " + String(state));
  }
  
  parseUDPPacket();
  updateFSM(now);


    
}