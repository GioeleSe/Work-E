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
#include <ArduinoJson.h>
#include <cstring>
#include <WiFi.h>
#include <AsyncUDP.h>


static AsyncUDP udp;
static IPAddress backendIp;
static uint16_t backendPort = 0;


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
      handlePacket(packet.length());
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

static void sendFeedbackSuccess(uint16_t req_id, uint16_t mode_bit){
        StaticJsonDocument<256> out;
      out["protocol"]=PROTOCOL_ID;
      out["message_type"]="feedback";
      out["request_id"]=req_id;
      out["mode"]=mode_bit;
      out["payload"]["status"]="success";
      out["timestamp"]=millis()/1000;
      sendJson(backendIp, backendPort, out);
}

void handlePacket(int len){ 
  if (len<=0) return;
  static uint8_t buf[1024];
  int r = udp.read(buf, sizeof(buf)-1);
  if(r<=0) return;
  buf[r]=0;
  StaticJsonDocument<1024> doc;
  //
  auto err = deserializeJson(doc, (char*)buf);
  if(err) return;
  const char* proto = doc["protocol"];
  const char* mtype = doc["message_type"];
  const char* payload = doc["payload"];
  uint16_t req_id = doc["request_id"] | 0;
  if(!proto || strcmp(proto, PROTOCOL_ID)!=0) return;
  IPAddress remoteIP = udp.remoteIP();
  uint16_t remotePort = udp.remotePort();
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
              mcp.speed = doc["payload"]["speed"] | 0;
              mcp.angle = doc["payload"]["angle"] | 0;
              mcp.duration = doc["payload"]["duration"] | 0;
              mcp.motor_ids = doc["payload"]["motor_ids"];
              if (mcp.motor_ids == nullptr){
                mcp.direction = doc["payload"]["direction"];
              }
              executeMotorControl(mcp);
              sendFeedbackSuccess(req_id, 0x00); 
            } else if(strcmp(cmd,"set_config")==0){
              // parse config params, update configs
              SetConfigPayload scp;
              scp.prop = doc["payload"]["prop"];
              scp.new_value = doc["payload"]["new_value"];
              executeSetConfig(scp);\
              sendFeedbackSuccess(req_id, 0x00);
            } else if(strcmp(cmd, "get_config")==0){
              // parse which config to get, retrieve value, and send feedback with value
              GetConfigPayload gcp;
              gcp.prop = doc["payload"]["prop"];
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
  p["state"]="idle"; // or actual state
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
