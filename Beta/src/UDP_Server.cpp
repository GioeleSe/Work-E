#include "UDP_Server.h"
#include "Beta.h"

AsyncUDP serverUDP;

void startUDPServer()
{
    Serial.println("Starting UDP server");
    Serial.println("Listening on port ");
    Serial.println(LOCAL_SERVER_PORT);
    if (serverUDP.listen(LOCAL_SERVER_PORT))
    {
        serverUDP.onPacket([](AsyncUDPPacket packet){
            handleIncomingPacket(packet);
        });
    }
    else
    {
        Serial.println("UDP server setup failed :-(");
        Serial.println("Error code ");
        Serial.println(serverUDP.lastErr());
        
        Serial.println("Retrying in 2 seconds");
        delay(2000);
        startUDPServer();
    }
}

void handleIncomingPacket(AsyncUDPPacket packet)
{
    //todo tuttecose
}