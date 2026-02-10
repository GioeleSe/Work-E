#include "UDP_Client.h"

AsyncUDP clientUDP;

//? add a ConnectionStats struct to keep track of relevant information?

void connectToServer()
{
    Serial.print("Connecting to server ");
    Serial.print(SERVER_IP);
    Serial.print(":");
    Serial.println(SERVER_PORT);

    if (clientUDP.connect(SERVER_IP, SERVER_PORT))
    {
        Serial.println("UDP connection established successfully!");
    }
    else
    {
        Serial.println("UDP connection failed :-(");
        Serial.print("Error code ");
        Serial.println(clientUDP.lastErr());

        Serial.println("Retrying connection in 2 seconds");
        delay(2000);
        connectToServer();
    }
}

/// Serialize and send message to server
void sendMessage(JsonDocument message)
{
    //! enclose message in envelope structure
    String jsonString;
    serializeJson(message, jsonString);

    // Send data to server
    clientUDP.write((uint8_t *)jsonString.c_str(), jsonString.length());
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
    // todo OPTIONAL: error_code, error_message

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
    // todo OPTIONAL: error_code, error_message

    return doc;
}

void checkConnection()
{
    // Check if connection is still active
    // Otherwise, try to reconnect
    if (!clientUDP.connected())
    {
        Serial.println("!! Connection lost !!");
        Serial.println("Attempting to reconnect...");
        connectToServer();
    }
    return;
}