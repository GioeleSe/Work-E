/* test.cpp
   Single-file test harness that sends UDP JSON messages to the local
   `communication.cpp` listener (port 8000) and prints replies to Serial.

   Usage: fill `WIFI_SSID` and `WIFI_PASS` with your network credentials,
   build & upload to the ESP32, open Serial at 115200.

   This does not modify `communication.cpp`. It uses a local UDP socket
   bound to `TEST_LOCAL_PORT` so replies from the listener are received
   by this sketch.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// --- Configure these before running tests ---
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASS";

const IPAddress DEST_IP = INADDR_NONE; // will be replaced with local IP
const uint16_t DEST_PORT = 8000; // listener port used by communication.cpp
const uint16_t TEST_LOCAL_PORT = 9000; // source port for replies

WiFiUDP udp;

bool waitForReply(uint32_t timeoutMs = 2000){
  uint32_t start = millis();
  while(millis() - start < timeoutMs){
    int size = udp.parsePacket();
    if(size>0){
      static uint8_t buf[1024];
      int r = udp.read(buf, sizeof(buf)-1);
      if(r>0){
        buf[r]=0;
        Serial.print("REPLY: ");
        Serial.println((char*)buf);
        return true;
      }
    }
    delay(10);
  }
  Serial.println("No reply (timeout)");
  return false;
}

void sendJsonString(const char* json){
  // send to the local listener (device IP)
  IPAddress target = WiFi.localIP();
  udp.beginPacket(target, DEST_PORT);
  udp.write((const uint8_t*)json, strlen(json));
  udp.endPacket();
  Serial.print("SENT: "); Serial.println(json);
}

void setup(){
  Serial.begin(115200);
  while(!Serial) delay(10);
  Serial.println("test.cpp starting");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-t0 < 20000) {
    Serial.print('.'); delay(500);
  }
  Serial.println();
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WiFi failed to connect - update WIFI_SSID / WIFI_PASS in test.cpp");
    return;
  }

  Serial.print("WiFi connected, IP= "); Serial.println(WiFi.localIP());

  // bind local UDP socket to known port so communication.cpp will reply to it
  if(!udp.begin(TEST_LOCAL_PORT)){
    Serial.println("udp.begin failed");
    return;
  }
  Serial.print("UDP test socket bound to port "); Serial.println(TEST_LOCAL_PORT);

  // Example 1: emergency_stop (expect immediate feedback success)
  const char* emergency = "{\"protocol\":\"robot-net/1.0\",\"message_type\":\"command\",\"request_id\":1,\"mode\":\"manual\",\"payload\":{\"command\":\"emergency_stop\"},\"timestamp\":1700000000}";
  sendJsonString(emergency);
  waitForReply(2000);

  // Example 2: move (expect pending)
  const char* move = "{\"protocol\":\"robot-net/1.0\",\"message_type\":\"command\",\"request_id\":2,\"mode\":\"auto\",\"payload\":{\"command\":\"move\",\"destination\":{\"x\":10,\"y\":5},\"navigation_type\":\"free_move\"},\"timestamp\":1700000000}";
  sendJsonString(move);
  waitForReply(2000);

  // Example 3: duplicate request_id 2 -> current implementation may drop duplicates
  const char* move_dup = "{\"protocol\":\"robot-net/1.0\",\"message_type\":\"command\",\"request_id\":2,\"mode\":\"auto\",\"payload\":{\"command\":\"move\",\"destination\":{\"x\":11,\"y\":6}},\"timestamp\":1700000000}";
  sendJsonString(move_dup);
  waitForReply(2000);

  Serial.println("Test sequence complete.");
}

void loop(){
  // no-op; tests run from setup
}
