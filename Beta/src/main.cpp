#include <Arduino.h>
#include "UDP_RoboControl.h"
#include "Beta.h"

hw_timer_t *heartbeatTimer = NULL;

Beta self;

void IRAM_ATTR onHeartbeatTimer();

void setup()
{
    // todo: setup motor control
    //* -> set all motor control inputs to OUTPUT
    //* -> set initial state as LOW (motors are turned off)
    
    // Setup heartbeat timer
    heartbeatTimer = timerBegin(0, 80, true); // Timer 0, Clock divider 80
    timerAttachInterrupt(heartbeatTimer, &onHeartbeatTimer, true);
    timerAlarmWrite(heartbeatTimer, 5000000, true); // Trigger every 5 seconds
    timerAlarmEnable(heartbeatTimer);
    
    // Begin serial communication
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }
    
    // Initialize Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    Serial.print("SSID: ");
    Serial.print(ssid);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected! IP Address: ");
    Serial.println(WiFi.localIP());
    
    connectToServer();
    
    // Start UDP server
    // startUDPServer();
}

void loop()
{
    // construct message types based on needs and events, then, sendMessage()
    //? -> switch case?
}

// Heartbeat timer ISR
void IRAM_ATTR onHeartbeatTimer()
{
    checkConnection();

    //? identify robot state, send heartbeat
    sendMessage(heartbeatMessage());
    return;
}