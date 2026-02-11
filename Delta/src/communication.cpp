/* Robot receives commands and puts them in a producer queue, then consumes them.
   1,2,3 are put in queue, 4,5 are handled immediately.
   Base station can send commands:
    1. move to destination coordinate
    2. motor control -> to control specific robot functions
    3. set_config (low level motor params)
    4. emergency stop
    5. reset

   Robot can send message:
   - heartbeat
   - event -> obstacle detected/cleared, load collected, destination reached
   - feedback -> delaying execution because of obstacle, or need help with load
   - error
*/

#include "config.h"
#include "communication.h"
#include "navigation.h"
#include "motor_control.h"
#include "delta.cpp"
#include <ArduinoJson.h>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncUDP.h>


IPAddress serverIP; 
static AsyncUDP udp;
static IPAddress backendIp;
static uint16_t backendPort = 0;
extern RobotState current_state;
// simple dedupe: store last seen request ids (circular buffer)
static const int MAX_RECENT = 16;
static uint16_t recent_ids[MAX_RECENT];
static int recent_pos = 0;

static bool seenRequest(uint16_t id){
  for(int i=0;i<MAX_RECENT;i++) if(recent_ids[i]==id) return true;
  recent_ids[recent_pos]=id; recent_pos=(recent_pos+1)%MAX_RECENT; return false;
}


void initializeWiFi(){
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected successfully!");
  
  serverIP = WiFi.localIP();
  Serial.print("Server IP address: ");
  Serial.println(serverIP);
  Serial.print("Signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}
void startUDPServer(){
  bool success = udp.listen(LOCAL_PORT);
  if(success) {
    Serial.print("UDP server listening on port ");
    Serial.println(LOCAL_PORT);
    udp.onPacket([](AsyncUDPPacket packet){
      handlePacket(packet);
    });
  } else {
    Serial.println("Failed to start UDP server");
  }
}

static void sendJson(const IPAddress& ip, uint16_t port, const JsonDocument& doc){
  char buf[512];
  size_t n = serializeJson(doc, buf, sizeof(buf));
  //send
}

void sendFeedbackSuccess(uint16_t req_id, uint16_t mode_bit){
        StaticJsonDocument<256> out;
      out["protocol"]=PROTOCOL_ID;
      out["message_type"]="feedback";
      out["request_id"]=req_id;
      out["mode"]=mode_bit;
      out["payload"]["status"]="success";
      out["timestamp"]=millis()/1000;
      sendJson(backendIp, backendPort, out);
}

void handlePacket(AsyncUDPPacket packet){ 
    // Extract packet information
  IPAddress clientIP = packet.remoteIP();
  uint16_t clientPort = packet.remotePort();
  String message = "";

  // Convert packet data to string
  for (size_t i = 0; i < packet.length(); i++) {
    message += (char)packet.data()[i];
  }
  message.trim();
  StaticJsonDocument<1024> doc;
  //
  auto err = deserializeJson(doc, (char*)message.c_str());
  if(err) return;
  const char* proto = doc["protocol"];
  const char* mtype = doc["message_type"];
  const char* payload = doc["payload"];
  uint16_t req_id = doc["request_id"] | 0;
  if(!proto || strcmp(proto, PROTOCOL_ID)!=0) return;
  IPAddress remoteIP = packet.remoteIP();
  uint16_t remotePort = packet.remotePort();
  backendIp = remoteIP; backendPort = remotePort;

    if(seenRequest(req_id)) {
        // already handled, optinally send feedback that it's duplicate 
        return;
      }
    else{
     if (strcmp(mtype, "emergency_stop")==0){
      // handle immediately
      // enqueue or handle other commands
      // send immediate ack/pending feedback
      sendFeedbackSuccess(req_id, 0x01); // fixed mode bit for mnual mode
      
    }else if (strcmp(mtype, "reset")==0){ // reset
      // handle immediately
      // call reset routines...
      // send feedback
       sendFeedbackSuccess(req_id, 0x01); // fixed mode bit for mnual mode
    } else if (strcmp(mtype, "command")==0){
       // validate payload.command
            const char* cmd = doc["payload"]["command"];
            if(cmd==nullptr) return;
            // Map to internal commands and call handlers:
            if(strcmp(cmd,"move")==0){
              //undefined, send success regardless
              sendFeedbackSuccess(req_id, 0x00); // fixed mode bit for auto mode
            } else if(strcmp(cmd,"motor_control")==0){
              // parse motor control params, call motor control module
              MotorControlPayload mcp;
              memset(&mcp, 0, sizeof(mcp));
              mcp.speed = (uint8_t)(doc["payload"]["speed"] | 0);
              mcp.angle = (int)(doc["payload"]["angle"] | 0);
              mcp.duration = (uint32_t)(doc["payload"]["duration"] | 0);
              // parse motor_ids array; allocate Motors[] terminated by END_MOT
              JsonArray ids = doc["payload"]["motor_ids"].as<JsonArray>();
              if(!ids.isNull()){
                size_t count = ids.size();
                Motors* arr = (Motors*)malloc(sizeof(Motors) * (count + 1));
                if(arr){
                  size_t i = 0;
                  for(JsonVariant v : ids){
                    if(i < count){
                      arr[i++] = (Motors)(int)v;
                    }
                  }
                  arr[count] = END_MOT; // sentinel
                  mcp.motor_ids = arr;
                } else {
                  mcp.motor_ids = nullptr;
                }
              } else {
                mcp.motor_ids = nullptr;
              }
              if (mcp.motor_ids == nullptr){
                mcp.direction = (Direction)(doc["payload"]["direction"] | 0);
              }
              executeMotorControl(mcp);
              if(mcp.motor_ids) free(mcp.motor_ids);
              sendFeedbackSuccess(req_id, 0x00);
            } else if(strcmp(cmd,"set_config")==0){
              // parse config params, update configs
              SetConfigPayload scp;
              memset(&scp,0,sizeof(scp));
              scp.prop = (ConfigFields)(doc["payload"]["prop"] | 0);
              // convert new_value to string for existing API (caller expects char* printable)
              const char* sval = doc["payload"]["new_value"] | nullptr;
              char* allocated = NULL;
              if(sval){
                allocated = (char*)malloc(strlen(sval)+1);
                if(allocated) strcpy(allocated, sval);
              } else if(doc["payload"]["new_value"].is<int>()){
                int v = doc["payload"]["new_value"] | 0;
                allocated = (char*)malloc(32);
                if(allocated) sprintf(allocated, "%d", v);
              } else if(doc["payload"]["new_value"].is<bool>()){
                bool b = doc["payload"]["new_value"];
                allocated = (char*)malloc(8);
                if(allocated) sprintf(allocated, "%s", b?"true":"false");
              }
              scp.new_value = allocated;
              executeSetConfig(scp);
              if(allocated) free(allocated);
              sendFeedbackSuccess(req_id, 0x00);
            } else if(strcmp(cmd, "get_config")==0){
              // parse which config to get, retrieve value, and send feedback with value
              GetConfigPayload gcp;
              gcp.prop = doc["payload"]["prop"];
              // sendFeedback(req_id, JsonObject(), "pending"); // send pending feedback while retrieving config
              executeGetConfig(gcp);
              sendFeedbackSuccess(req_id, 0x00);
            } else {
              // unknown command, optionally send error feedback
            }

      }
  }
}

void commsLoop(){
  // periodic heartbeat every 2 seconds
  static uint32_t lastHB = 0;
  if(millis() - lastHB > 2000){
    lastHB = millis();
    sendHeartbeat();
  }
}

void sendHeartbeat(){
  if(WiFi.status()!=WL_CONNECTED || backendPort==0) return;
  StaticJsonDocument<256> out;
  out["protocol"]=PROTOCOL_ID;
  out["message_type"]="heartbeat";
  out["request_id"]=0;
  out["mode"]="auto";
  JsonObject p = out.createNestedObject("payload");
  p["state"]=current_state; // or actual state
  p["rssi"]=WiFi.RSSI();
  out["timestamp"]=millis()/1000;
  sendJson(backendIp, backendPort, out);
}

void sendFeedback(uint16_t request_id, const JsonObject& payload, const char* status){
  if(WiFi.status()!=WL_CONNECTED || backendPort==0) return;
  StaticJsonDocument<256> out;
  out["protocol"] = PROTOCOL_ID;
  out["message_type"] = "feedback";
  out["request_id"] = request_id;
  out["mode"] = "auto";
  JsonObject p = out.createNestedObject("payload");
  p["status"] = status;
  // copy additional fields from payload if provided
  if (!payload.isNull()) {
    for (JsonPair kv : payload) {
      p[kv.key()] = kv.value();
    }
  }
  out["timestamp"] = millis()/1000;
  sendJson(backendIp, backendPort, out);
}
