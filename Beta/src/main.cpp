#include <Arduino.h>
#include "Beta.h"
#include "UDP_Client.h"
#include "UDP_Server.h"

hw_timer_t *heartbeatTimer = NULL;

void setupMotors();
void setupHeartbeatTimer();
void setupWiFi();
void IRAM_ATTR onHeartbeatTimer();

void setup()
{    
    // Begin serial communication
    Serial.begin(115200);
    while (!Serial)
    {
        delay(100);
    }
    
    // Setup hardware related functions
    setupMotors();
    setupHeartbeatTimer();
    
    // Setup WiFi connection and communication with the server
    setupWiFi();
    connectToServer();
    // startUDPServer();

    //todo initialize self
}

void loop()
{
    // construct message types based on needs and events, then, sendMessage()
    //? -> switch case?
}

void setupMotors()
{
    // Set all motor control pins to OUTPUT
    pinMode(MOTOR_A1_PIN, OUTPUT);
    pinMode(MOTOR_A2_PIN, OUTPUT);
    pinMode(MOTOR_B1_PIN, OUTPUT);
    pinMode(MOTOR_B2_PIN, OUTPUT);

    // Initial state: motors are turned off
    digitalWrite(MOTOR_A1_PIN, LOW);
    digitalWrite(MOTOR_A2_PIN, LOW);
    digitalWrite(MOTOR_B1_PIN, LOW);
    digitalWrite(MOTOR_B2_PIN, LOW);
}

void setupHeartbeatTimer()
{
    heartbeatTimer = timerBegin(0, 80, true); // Timer 0, Clock divider 80
    timerAttachInterrupt(heartbeatTimer, &onHeartbeatTimer, true);
    timerAlarmWrite(heartbeatTimer, 2000000, true); // Trigger every 2 seconds
    timerAlarmEnable(heartbeatTimer);
}

void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Connecting to WiFi");
    Serial.print("SSID: ");
    Serial.print(WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected! IP Address: ");
    Serial.println(WiFi.localIP());
}

/// Heartbeat timer ISR
void IRAM_ATTR onHeartbeatTimer()
{
    checkConnection();

    //? identify robot state, send heartbeat
    sendMessage(heartbeatMessage());
    return;
}