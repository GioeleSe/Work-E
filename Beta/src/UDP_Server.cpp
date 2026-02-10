#include "UDP_Server.h"

AsyncUDP serverUDP;

void startUDPServer()
{
    Serial.println("Starting UDP server");
    Serial.println("Listening on port ");
    Serial.println(LOCAL_SERVER_PORT);
    if (serverUDP.listen(LOCAL_SERVER_PORT))
    {
        serverUDP.onPacket([](AsyncUDPPacket packet)
                           { handleIncomingPacket(packet); });
    }
    else
    {
        Serial.println("UDP server setup failed :-(");
        Serial.print("Error code ");
        Serial.println(serverUDP.lastErr());

        Serial.println("Retrying in 2 seconds");
        delay(2000);
        startUDPServer();
    }
}

void handleIncomingPacket(AsyncUDPPacket packet)
{
    uint8_t *packetData = packet.data(); // Get a pointer to the packet's raw data
    size_t dataLength = packet.length(); // See how many bytes have been received

    string stringData = stringifyData(packetData, dataLength);
    // printDataAsString(packetData, dataLength);

    // Deserialize the received JSON data
    JsonDocument doc; // The deserialized document will be stored here
    deserializeJson(doc, stringData);

    //? from here you can reconstruct the message extracting the values directly with doc["..."]
    //? how can I recognize the type of message received?

    // then, execute command
}

string stringifyData(uint8_t *packetData, size_t dataLength)
{
    string stringData = "";
    for (size_t i = 0; i < dataLength; i++)
    {
        stringData += (char)packetData[i]; //?? I'm not sure this is correct...
    }
    return stringData;
}

void printDataAsString(uint8_t *packetData, size_t dataLength)
{
    Serial.print("Received packet from server with ");
    Serial.print(dataLength);
    Serial.println(" bytes");

    //? print received data on the serial monitor (for debug purposes)
    Serial.println("Prining received data as string...");
    for (size_t i = 0; i < dataLength; i++)
    {
        //? print only if it contains printable characters
        if (packetData[i] >= 32 && packetData[i] <= 126)
        {
            Serial.print((char)packetData[i]);
        }
        else
        {
            Serial.print(".");
        }
        Serial.println();
    }
}