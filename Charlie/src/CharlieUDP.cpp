

#include <WiFi.h>
#include <time.h>
#include <AsyncUDP.h>
#include "CharlieUDP.h"
#include "configs.h"
#include "fsm.h"
#include "tasks.h"


AsyncUDP udp;

// UDP receive buffer
char udpBuffer[UDP_BUFFER_SIZE];
// UDP receive buffer

unsigned long timeOffset = 0;


void setupUDP(){

  if(udp.listen(UDP_PORT)){

    Serial.println("UDP listening on port "+String(UDP_PORT));

    udp.onPacket([](AsyncUDPPacket packet){ // callback for incoming packets
      
      int len = packet.length();
      
      if(len >= UDP_BUFFER_SIZE) return;
      
      memcpy(udpBuffer, packet.data(), len);
      
      udpBuffer[len] = '\0';
      
      Serial.print("Received UDP: ");
      
      Serial.println(udpBuffer);}
    );
}

}

void syncTimeNTP() {
  Serial.print("[NTP] Syncing time... ");
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (attempts < 10) {
    Serial.println(" OK");
    Serial.print("[NTP] Current time: ");
    Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
  } else {
    Serial.println(" FAILED");
    Serial.println("[NTP] Using millis() as fallback");
  }
}

unsigned long getEpochTimeMs() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    time_t now;
    time(&now);
    return ((unsigned long)now * 1000);
  } else {
    return millis() + timeOffset;
  }
}


void buildEnvelope(JsonDocument& doc, const char* messageType, const char* mode) {
  doc["protocol"] = "robot-net/1.0";
  doc["robot_id"] = ROBOT_ID;  // Charlie = 2
  doc["message_type"] = messageType;
  doc["request_id"] = lastRequestId;
  doc["mode"] = mode;
  doc["timestamp"] = getEpochTimeMs(); 
}


void sendUDP(const JsonDocument& doc) {
   char buffer[512];
  size_t len = serializeJson(doc, buffer);

  udp.writeTo(
    (uint8_t*)buffer,
    len,
    SERVER_IP,
    UDP_PORT
  );
}


void sendHeartbeat(RobotState_t state) {
   JsonDocument doc;  
  
  buildEnvelope(doc, "heartbeat", "auto");


  doc["request_id"] = 0;
  
  // Payload
  JsonObject payload = doc["payload"].to<JsonObject>();
  payload["state"] = (int)state;
  
  int rssiVal = WiFi.RSSI();
  payload["rssi"] = (rssiVal != 0 && rssiVal != -127) ? rssiVal : -999;
  
  sendUDP(doc);
}


void sendFeedback(uint16_t reqId, int status, int errorCode, const char* errorMsg) {
   JsonDocument doc;  
  
  buildEnvelope(doc, "feedback", "auto");
  doc["request_id"] = reqId;
  
  JsonObject payload = doc["payload"].to<JsonObject>();
  payload["status"] = status;  // 0=SUCCESS, 1=FAILURE, 2=PENDING
  
  if (status == 1) {  // FAILURE
    payload["error_code"] = errorCode;
    payload["error_message"] = errorMsg ? errorMsg : "";
  }
  
  sendUDP(doc);
}



void sendEvent(int eventCode, const char* details) {
    JsonDocument doc;  
  
  buildEnvelope(doc, "event", "auto");
  
  JsonObject payload = doc["payload"].to<JsonObject>();

  payload["event"] = eventCode;  // 10=obstacle_detected, 11=obstacle_removed, etc
  
  if (details) {
    payload["details"] = details;
  }
  
  sendUDP(doc);
}



void sendProperty(const char* prop, const char* value) {
    JsonDocument doc;  

  buildEnvelope(doc, "property", "auto");
  
    JsonObject payload = doc["payload"].to<JsonObject>();
  payload["prop"] = prop;
  payload["value"] = value;
  
  sendUDP(doc);
}


void sendDebug(const char* message) {
  JsonDocument doc;  
  buildEnvelope(doc, "debug", "auto");
  
  JsonObject payload = doc["payload"].to<JsonObject>();
  payload["message"] = message;
  
  sendUDP(doc);
}


void sendError(int severity, int errorCode) {
  JsonDocument doc;
  
  buildEnvelope(doc, "error", "auto");
  
  JsonObject payload = doc["payload"].to<JsonObject>();
  payload["severity"] = severity;  // 0=LOW, 1=MID, 2=HIGH
  payload["error_code"] = errorCode;
  
  sendUDP(doc);
}


// command handling 
void handleCommand(CommandType cmd, uint16_t reqId, const JsonDocument& doc) {
  JsonVariantConst payloadVariant = doc["payload"];
  
  switch (cmd){
    case CMD_MOVE:
    if (state == ROBOT_IDLE) {
      // extract parameters from payload with defaults if missing
      int dest_x = payloadVariant["destination_x"] | 0;
      int dest_y = payloadVariant["destination_y"] | 0;
      int dest_checkpoint = payloadVariant["destination_checkpoint"] | 0;
      int nav_type = payloadVariant["navigation_type"] | 0;
      int route_policy = payloadVariant["route_policy"] | 0;
Serial.println("[CMD] MOVE received");
      Serial.printf("  destination: (%d, %d)\n", dest_x, dest_y);
      Serial.printf("  checkpoint: %d, nav_type: %d, policy: %d\n",
        dest_checkpoint, nav_type, route_policy);

        // change state to BUSY and start task
        state = ROBOT_BUSY;
        busyStarted = false;

        // send pending feedback immediately
        sendFeedback(reqId, 2, 0, "");  // status=PENDING(2)
        
      } else{
        Serial.println("[CMD] MOVE rejected: not IDLE");
        sendFeedback(reqId, 1, 1, "Robot not idle");  // status=FAILURE(1), error_code=1
        }
      
        break;

    case CMD_MOTOR_CONTROL:
      Serial.println("[CMD] MOTOR_CONTROL (placeholder)");
      
      sendFeedback(reqId, 0, 0, "");  // status=SUCCESS(0)
      break;

    case CMD_SET_PROPERTY:
      {
        const char* prop = payloadVariant["prop"];
        const char* new_value = payloadVariant["new_value"];
        
        Serial.printf("[CMD] SET_PROPERTY: %s = %s\n", prop, new_value);
 
        sendFeedback(reqId, 0, 0, "");  // status=SUCCESS(0)
      }
      break;

    case CMD_GET_PROPERTY:
      {
        const char* prop = payloadVariant["prop"];
        
        Serial.printf("[CMD] GET_PROPERTY: %s\n", prop);
     
        sendProperty(prop, "default_value");
        
        sendFeedback(reqId, 0, 0, "");  // status=SUCCESS(0)
      }
      break;

  
    case CMD_STOP:
      Serial.println("[CMD] STOP (emergency)");
      
      state = ROBOT_IDLE;
      busyStarted = false;

      sendFeedback(reqId, 0, 0, "");  // status=SUCCESS(0)
      break;

    default:
      Serial.println("[CMD] Unknown command");
      sendFeedback(reqId, 1, 2, "Unknown command");  // status=FAILURE(1), error_code=2
      break;
  }
}


void parseUDPPacket(){
  StaticJsonDocument<JSON_DOC_SIZE> doc;
  DeserializationError error = deserializeJson(doc, udpBuffer);
  
  if (error){
    Serial.print("[ERROR] JSON parse failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // envelope validation
  if (!doc.containsKey("message_type") ||
  !doc.containsKey("request_id") ||
  !doc.containsKey("payload")){
    Serial.println("[ERROR] Invalid envelope");
    return;
  }
  
  // extract message_type and request_id
  const char* messageType = doc["message_type"];
  uint16_t reqId = doc["request_id"];
  lastRequestId = reqId;

  // Only handle "command" messages here, others can be ignored or handled differently
  if(strcmp(messageType, "command") != 0) {
    Serial.printf("[WARN] Not a command: %s\n", messageType);
    return;
  }

  // command validation
  if(!doc["payload"].containsKey("command")){
    Serial.println("[ERROR] Missing command field");
    sendFeedback(reqId, 1, 3, "Missing command field");
    return;
  }
  
  // extract command
  const char* cmdStr = doc["payload"]["command"];
  
  CommandType cmd = CMD_NONE;


  if      (strcmp(cmdStr, "move") == 0)           cmd = CMD_MOVE;
  else if (strcmp(cmdStr, "motor_control") == 0)  cmd = CMD_MOTOR_CONTROL;
  else if (strcmp(cmdStr, "set_property") == 0)   cmd = CMD_SET_PROPERTY;
  else if (strcmp(cmdStr, "get_property") == 0)   cmd = CMD_GET_PROPERTY;
  else if (strcmp(cmdStr, "stop") == 0)           cmd = CMD_STOP;

  handleCommand(cmd, reqId, doc);
}