#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <typeindex>
#include <functional>
#include <map>
#include <time.h>

#include "udp_client.h"
#include "udp_server.h"
#include "robot_server.h"
#include "main_robot.h"

#define ROBOT_NET_PROTOCOL "robot-net/1.0"

// -------------------------------- main code --------------------------------

void* message_server_thread(void* arg){
    printf("robot server - udp_server_thread - Server initialized. Calling listening function.\n");
    server_listen_port();                                           // will return only on error or on request

    pthread_exit(NULL);
}

void* message_reader_thread(void* arg){
    printf("robot server - message_reader_thread - Reader started.\n");

    char message[BUFFER_SIZE];
    int msg_count = 0;
    int pop_result = 0;
    ssize_t message_length = 0;

    while(1){
        pop_result = server_buffer_pop(message);
        message_length = strlen(message);
        message[message_length] = '\0';
        if((pop_result == 0) && (message_length > 1)){
            printf("robot server - message_reader_thread - [msg #%d] received: '%s'\n", ++msg_count, message);
            PacketHandler(message, strlen(message));
        } else {
            // buffer empty, avoid busy-waiting
            sleep(1);
        }
    }
    pthread_exit(NULL);
}

void handle_shutdown(int sig){
    printf("\nrobot server - shutting down\n");
    if(sig == SIGINT){                                              // "gracefully" (more or less)
        sem_wait(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
        udp_server_data.stop_server = 1;
        sem_post(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
    }else{
        udp_server_data.stop_server = 1;
    }
    exit(0);
}

int main(int argc, char* argv[]){
    setvbuf(stdout, NULL, _IOLBF, 0);                               // prevent printf buffering
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);

    pthread_t reader_tid, server_tid;
    pthread_attr_t reader_pt_attr, server_pt_attr;
    server_init();

    pthread_attr_init(&reader_pt_attr);
    if(pthread_create(&reader_tid, NULL, message_reader_thread, NULL) != 0){
        printf("robot server - main - Failed to create reader thread.\n");
        pthread_attr_destroy(&reader_pt_attr);
        return -1;
    }else{
        printf("robot server - main - Reader thread created.\n");
    }

    pthread_attr_init(&server_pt_attr);
    if(pthread_create(&server_tid, NULL, message_server_thread, NULL) != 0){
        printf("robot server - main - Failed to create server thread.\n");
        pthread_attr_destroy(&server_pt_attr);
        return -1;
    }else{
        printf("robot server - main - Server thread created.\n");
    }

  
    pthread_join(server_tid, NULL);
    pthread_join(reader_tid, NULL);
    pthread_attr_destroy(&server_pt_attr);
    pthread_attr_destroy(&reader_pt_attr);
    return 0;
}

// ------- robot_server packet handlers implementation here -------
struct FieldDef {                                                   // fields parser structures (runtime type check)
    const char* name;
    std::function<bool(const JsonVariant&)> check;
};
const FieldDef expected_fields[] = {
    {"protocol",     [](const JsonVariant& v){ return v.is<const char*>(); }},
    {"robot_id",     [](const JsonVariant& v){ return v.is<int>(); }},
    {"message_type", [](const JsonVariant& v){ return v.is<int>(); }},
    {"request_id",   [](const JsonVariant& v){ return v.is<int>(); }},
    {"mode",         [](const JsonVariant& v){ return v.is<int>(); }},
    {"payload",      [](const JsonVariant& v){ return v.is<JsonObject>(); }},
    {"timestamp",    [](const JsonVariant& v){ return v.is<long>(); }}
};

// check for common non-null needed (header) fields:
// header fields = "protocol", "robot_id", "message_type", "request_id", "mode", "payload", "timestamp"
// (check for both presence and type)
// obv return -1 for errors
int check_fields(const JsonDocument& json_doc){
    for (auto const& field_data : expected_fields){
        JsonVariant field_val = json_doc[field_data.name];
        if(field_val.isNull()){
            printf("missing field %s in json packet\n", field_data.name);
            return -1;
        }else{
            if(!field_data.check(field_val)){
                printf("wrong type of field %s in json packet\n", field_data.name);
                return -1;
            }
        }
    }
}

// check for command-type message (== assert "message_type" = 0)
// and payload command field existence and value
int check_command(const JsonObject& json_payload){
    JsonVariant payload_command = json_payload["command"];
    if((payload_command.isNull()) || !(payload_command.is<int>())){
        printf("invalid command field inside payload (expected non-null integer)\n");
        return -1;
    }
    int command_val = payload_command.as<int>();
    return (command_val < 0)?-1:command_val;                        // any negative values are set to -1 (used here as generic error value)
}

// set common headers of the message envelope.
// The mode is set to default manual.
// Note that:
//  - message_type is *not* set here 
//  - the timestamp is set here
//  - the uuid is random, overwrite it to match the command uuid
void set_message_common_headers(const JsonObject& reply_doc){
    reply_doc["protocol"] = ROBOT_NET_PROTOCOL;
    reply_doc["robot_id"] = self_prop_get_robot_id();
    reply_doc["request_uui"] = (uint16_t)(rand()%65535);
    reply_doc["mode"] = (int)MessageMode_t::MessageMode_MANUAL;
    reply_doc["timestamp"] = time(NULL);
}

// define the payload of
int get_get_config_payload( const JsonObject& json_payload, GetConfigPayload_t* get_config_payload){
    JsonVariant prop_name = json_payload["prop"];
    if((prop_name.isNull()) || !(prop_name.is<int>())){
        printf("invalid property field inside payload (expected non-null integer)\n");
        return -1;
    }
    int prop_name_int = prop_name.as<int>();
    if(prop_name_int < 0){
        printf("unrecognized value for prop (configfields) value: %d\n", prop_name_int);
        return -1;
    }
    get_config_payload->prop = (ConfigFields_t)prop_name_int;
}

// get the current property value of the robot (functions defined in main_robot)
// send it back as response packet
// return -1 if the prop is unrecognized
int get_config_handler(uint16_t command_uuid, GetConfigPayload_t* get_config_payload){
    JsonObject reply_packet;
    set_message_common_headers(reply_packet);
    reply_packet["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
    reply_packet["request_uuid"] = command_uuid;                    // use the same as the command/request so that the server can track the source event for this reply
    reply_packet["payload"]["prop"] = get_config_payload->prop;

    switch(get_config_payload->prop){                               // amazing job of vscode shortcuts for these lines
        case ConfigFields_t::ConfigFields_SPEED:
            reply_packet["payload"]["value"] = self_prop_get_speed();
        case ConfigFields_t::ConfigFields_FEEDBACK:
            reply_packet["payload"]["value"] = self_prop_get_feedback();
        case ConfigFields_t::ConfigFields_DEBUG:
            reply_packet["payload"]["value"] = self_prop_get_debug();
        case ConfigFields_t::ConfigFields_NAVIGATION_TYPE:
            reply_packet["payload"]["value"] = self_prop_get_navigation_type();
        case ConfigFields_t::ConfigFields_ROUTE_POLICY:
            reply_packet["payload"]["value"] = self_prop_get_route_policy();
        case ConfigFields_t::ConfigFields_RADAR:
            reply_packet["payload"]["value"] = self_prop_get_radar();
        case ConfigFields_t::ConfigFields_SCREEN:
            reply_packet["payload"]["value"] = self_prop_get_screen();
        case ConfigFields_t::ConfigFields_OBSTACLE_CLEANER:
            reply_packet["payload"]["value"] = self_prop_get_obstacle_cleaner();
        case ConfigFields_t::ConfigFields_OBJECT_LOADER:
            reply_packet["payload"]["value"] = self_prop_get_object_loader();
            case ConfigFields_t::ConfigFields_OBJECT_UNLOADER:
            reply_packet["payload"]["value"] = self_prop_get_object_unloader();
        case ConfigFields_t::ConfigFields_OBJECT_COMPACTER:
            reply_packet["payload"]["value"] = self_prop_get_object_compacter();
        default:
            printf("unrecognized value for prop (configfields) value: %d\n", get_config_payload->prop);
            return -1;
        break;
    }

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

int PacketHandler(char* packet, ssize_t packet_size){
    if (packet == nullptr || packet_size <= 0) {
        printf("PacketHandler: invalid packet pointer\n");
        return -1;
    }

    JsonDocument json_doc;
    DeserializationError json_doc_error = deserializeJson(json_doc, packet, packet_size);
    if (json_doc_error) {
        printf("deserializeJson() returned %s\n", json_doc_error.c_str());
        return -1;
    }
    if(!check_fields(json_doc)){
        return -1;
    }
    
    char* protocol = json_doc["protocol"].as<char*>();
    int robot_id = json_doc["robot_id"].as<int>();
    int message_type = json_doc["message_type"].as<int>();
    int request_id = json_doc["request_id"].as<int>();
    int mode = json_doc["mode"].as<int>();
    JsonObject payload = json_doc["payload"].as<JsonObject>();
    long timestamp = json_doc["timestamp"].as<long>();
    
    if(message_type != 0){
        printf("invalid command value (expected value: 0)\n");
        return -1;

    }
    
    int command = check_command(payload);                       
    switch(command){                                                // TODO: complete all handlers routing
        case CommandType_t::CommandType_GET_PROPERTY:
        GetConfigPayload_t get_config_payload;
        if(get_get_config_payload(payload, &get_config_payload) < 0){
            printf("Invalid payload for get_config command\n");
            return -1;
        }
        get_config_handler(request_id, &get_config_payload);
        break;
        case CommandType_t::CommandType_SET_PROPERTY:
        break;
        case CommandType_t::CommandType_MOTOR_CONTROL:
        break;
        case CommandType_t::CommandType_MOVE:
        break;
        case CommandType_t::CommandType_EMERGENCY_STOP:
        break;
        case CommandType_t::CommandType_RESET:
        break;
        default:
            printf("unrecognized command value: %d\n", command);
            return -1;
        break;
    }

    return 0;
}