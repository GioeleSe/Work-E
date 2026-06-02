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
        // Serial.print("Error code ");
        // Serial.println(serverUDP.lastErr());

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
    printDataAsString(packetData, dataLength);

    // Deserialize the received JSON data and store it in a variable
    JsonDocument doc;

    // Catch any deserialization errors
    DeserializationError err = deserializeJson(doc, stringData);
    if (err)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(err.c_str());
        return;
    }

    //?? from here you can reconstruct the message extracting the values directly with doc["payload"]
    //?? how can I recognize the type of message received?

    // We only need to know the payload, which differs based on the type of message received
    CommandType incomingCommandType = doc["message_type"];

    //!! the order of statements in a switch case DOES NOT count!
    //!! you will have to deal with command priorities in a different way... (queue?)
    // Execute those commands!
    switch (incomingCommandType)
    {
    case CommandType_RESET:
    {
        ResetPayload rstPayload;

        rstPayload.reset = doc["payload"]["reset"];
        //?? if the receiverd string matches the "reset" keyword reset the board!
        break;
    }
    case CommandType_EMERGENCY_STOP:
    {
        EmergencyStopPayload stpPayload;

        stpPayload.stop = doc["payload"]["stop"];
        //?? if the received string matches the "stop" keyword reset the board!
        break;
    }
    case CommandType_MOTOR_CONTROL:
    {
        MotorControlPayload ctlPayload;

        //?? motor_id is an array
        ctlPayload.direction = doc["direction"];
        ctlPayload.speed = doc["speed"];
        ctlPayload.angle = doc["angle"];
        ctlPayload.duration = doc["duration_ms"];
        //??
        break;
    }
    case CommandType_MOVE:
    {
        // womp womp
        break;
    }
    case CommandType_SET_PROPERTY:
    {
        //todo
        break;
    }
    case CommandType_GET_PROPERTY:
    {
        //todo
        break;
    }
    default:
    {
        Serial.println("ERROR: couldn't recognize command");
        return;
    }
    }
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

//?? #ifdef DEBUG_UDP_SERVER
void printDataAsString(uint8_t *packetData, size_t dataLength)
{
    Serial.print("Received packet from server with ");
    Serial.print(dataLength);
    Serial.println(" bytes");

    // Print received data on the serial monitor (for debug purposes)
    Serial.println("Printing received data as string...");
    for (size_t i = 0; i < dataLength; i++)
    {
        // Print only if it contains printable characters
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
//?? #endif

//?? #ifndef DEBUG_UDP_SERVER
// void printDataAsString(uint8_t *packetData, size_t dataLength)
// {
// }
//?? #endif