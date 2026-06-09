#include "robot_server.h"
#include "main_robot.h"
#include "udp_client.h"
#include "ArduinoJson.h"
#include <WiFi.h>

const char* ssid = "local_hotspot";
const char* password = "esp32_mcu";

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== CHARLIE STARTUP ===");

    // Connect WiFi
    Serial.println("Connecting to WiFi...");
    IPAddress local_IP(192, 168, 137, 101); // Static IP for the robot
    IPAddress gateway(192, 168, 137, 1); // Gateway (router) IP
    IPAddress subnet(255, 255, 255, 0); // Subnet mask
    // WiFi.config(local_IP, gateway, subnet);
    WiFi.begin(ssid, password);
    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if(WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWiFi failed");
    }

    // Initialize hardware
    Serial.println("Initializing hardware...");
    initMotors();

    // Start radar task, which will handle radar servo control and distance sensing in the background
    startRadarTask();

    // Send online notification to server
    if(WiFi.status() == WL_CONNECTED) {
        JsonDocument doc;
        doc["protocol"]     = "robot-net/1.0";
        doc["robot_id"]     = self_prop_get_robot_id();
        doc["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
        doc["request_id"]   = 0;
        doc["mode"]         = (int)MessageMode_t::MessageMode_MANUAL;
        doc["timestamp"]    = (long)time(NULL);
        doc["payload"]["event"] = "online";
        char buf[256];
        ssize_t len = serializeJson(doc, buf);
        client_send_packet(buf, len);
        Serial.println("Online notification sent.");
    }

    // Start server
    Serial.println("Starting server...");
    Serial.print("WiFi status before starting server: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.println(WiFi.localIP());
    RobotStartServer();
}

void loop() {
    // Empty - everything runs in tasks
    delay(1000);
}