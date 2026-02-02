#include "UDP_RoboControl.h"
#include "Beta.h"

enum MessageType
{
    HEARTBEAT,
    FEEDBACK,
    EVENT,
    ERROR
};

AsyncUDP udp;

//? add a ConnectionStats struct to keep track of relevant information?

void connectToServer()
{
    Serial.println("Connecting to server ");
    Serial.println(serverIP);
    Serial.println(":");
    Serial.println(serverPort);

    if (udp.connect(serverIP, serverPort))
    {
        Serial.println("UDP connection established successfully!");
    }
    else
    {
        Serial.println("UDP connection failed :-(");
        Serial.println("Error code ");
        Serial.println(udp.lastErr());
        
        Serial.println("Retrying connection in 5 seconds");
        delay(5000);
        connectToServer();
    }
}

void startUDPServer()
{
    Serial.println("STARTING UDP SERVER");
    udp.listen(serverIP, serverPort);
    // todo packet handler here
    // udp.onPacket(...);
}

// Serialize and send message to server
void sendMessage(JsonDocument message)
{
    String jsonString;
    serializeJson(message, jsonString);
    
    // Send data to server
    udp.write((uint8_t *)jsonString.c_str(), jsonString.length());
}

//? states and other relevant info will be passed as arguments to these functions
//? almost everything here is a placeholder
JsonDocument heartbeatMessage()
{
    JsonDocument doc;
    
    doc["state"] = "IDLE"; // IDLE | DISCONNECTED | BUSY | ERROR
    doc["rssi"] = WiFi.RSSI();
    return doc;
}

JsonDocument feedbackMessage()
{
    JsonDocument doc;
    
    doc["status"] = "PENDING"; // SUCCESS | FAILURE | PENDING
    // todo OPTIONAL: error.code, error.message
    
    return doc;
}

JsonDocument eventMessage()
{
    JsonDocument doc;
    
    doc["obstacle_detected"] = 0;
    doc["poi_recahed"] = 0;
    doc["load_collected"] = 0;
    doc["load_disposed"] = 0;
    doc["reroute_required"] = 0;
    
    return doc;
}

JsonDocument errorMessage()
{
    JsonDocument doc;
    
    doc["severity"] = "LOW"; // LOW | MID | HIGH
    // todo OPTIONAL: error.code, error.message
    
    return doc;
}
   
void checkConnection()
{
    // Check if connection is still active
    // Otherwise, try to reconnect
    if (!udp.connected())
    {
        Serial.println("!! Connection lost !!");
        Serial.println("Attempting to reconnect...");
        connectToServer();
    }
    return;
}