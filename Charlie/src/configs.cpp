#include"configs.h"
#include<WiFi.h>



const char* SSID = "charlie";
const char* PASSWORD = "contrasenacharlie";

IPAddress local_IP(192, 168, 137, 69);
IPAddress gateway(192, 168, 137, 1);
IPAddress subnet(255, 255, 255, 0);

const int UDP_PORT = 8000;

IPAddress SERVER_IP(192, 168, 137, 1);

const unsigned long HEARTBEAT_INTERVAL = 2000;

void setupSerial(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("Charlie is alive ^^");
}

void setupWiFi(){
  Serial.println("Charlie is connecting to WiFi ^^");
  
  if(!WiFi.config(local_IP, gateway, subnet)){
    Serial.println("Charlie failed to configure his static IP :(");
  }
  
  WiFi.begin(SSID,PASSWORD);
  
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nCharlie is connected! ^^");
  Serial.print("Charlie IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Charlie MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Charlie RSSI (dBm): ");
  Serial.println(WiFi.RSSI());

}