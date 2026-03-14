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

// parse the payload to the specific get_config expected structure
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
    return 0;
}

// parse the payload to the specific set_config expected structure
// NOTE: the expected new value is int type!
// return -1 for null or invalid type fields
int get_set_config_payload( const JsonObject& json_payload, SetConfigPayload_t* set_config_payload){
    JsonVariant prop_name = json_payload["prop"];
    JsonVariant prop_value = json_payload["new_value"];
    if((prop_name.isNull()) || !(prop_name.is<int>())){
        printf("invalid property field inside payload (expected non-null integer)\n");
        return -1;
    }
    if((prop_value.isNull()) || !(prop_value.is<int>())){
        printf("invalid property value field inside payload (expected non-null integer)\n");
        return -1;
    }
    int prop_name_int = prop_name.as<int>();
    int prop_value_int = prop_value.as<int>();
    if(prop_name_int < 0){                                          // prop value is allowed to be negative (hence not checked here)
        printf("unrecognized value for prop (configfields) value: %d\n", prop_name_int);
        return -1;
    }
    set_config_payload->prop = (ConfigFields_t)prop_name_int;
    set_config_payload->new_value = (int)(prop_value);
    return 0;
}

// parse the payload to the specific reset expected structure
// return -1 for null or invalid type field
int get_reset_payload(const JsonObject& json_payload, ResetPayload_t* reset_payload){
    JsonVariant reset_keyword = json_payload["reset"];
    if((reset_keyword.isNull()) || !(reset_keyword.is<const char*>())){
        printf("invalid reset field inside payload (expected non-null string)\n");
        return -1;
    }
    const char* reset_keyword_str = reset_keyword.as<const char*>();
    strncpy(reset_payload->reset, reset_keyword_str, strlen(reset_keyword_str));                    // no others length check measures for now, C trust @.@ 
    return 0;
}

// parse the payload to the specific emergency_stop expected structure
// return -1 for null or invalid type field
int get_emergency_stop_payload(const JsonObject& json_payload, EmergencyStopPayload_t* emergency_stop_payload) {
    JsonVariant emergency_stop_keyword = json_payload["stop"];
    if((emergency_stop_keyword.isNull()) || !(emergency_stop_keyword.is<const char*>())){
        printf("invalid emergency_stop field inside payload (expected non-null string)\n");
        return -1;
    }
    const char* emergency_stop_keyword_str = emergency_stop_keyword.as<const char*>();
    strncpy(emergency_stop_payload->stop, emergency_stop_keyword_str, strlen(emergency_stop_keyword_str));
    return 0;
}

// parse the payload to the specific motor_control expected structure
// return -1 for null or invalid type fields
int get_motor_control_payload(const JsonObject& json_payload, MotorControlPayload_t* motor_control_payload){
    /*
        "payload": {
            "command": 2,
            "motor_id": [
                -1                          // no explicit motor to drive (-1 is end_mot) -> drive as a car!
            ],
            "direction": 0,
            "speed": 100,
            "angle": 0,
            "duration_ms": 0
        }
    */
    JsonVariant motor_id_list = json_payload["motor_id"];           // -1 as terminal value, list of indexes of enum Motors_t
    JsonVariant direction = json_payload["direction"];              // index of enum Direction_t
    JsonVariant speed = json_payload["speed"];
    JsonVariant angle = json_payload["angle"];
    JsonVariant duration_ms = json_payload["duration_ms"];          // 0 as continuous movement
    
    // check existence and type
    JsonVariant int_fields_list[] = {direction, speed, angle, duration_ms}; // motor_id_list is array of int
    for(JsonVariant field : int_fields_list){
        if((field.isNull()) || (!field.is<int>())){
            printf("invalid motor_control field inside payload (expected non-null int)\n");
            return -1; 
        }
    }
    if((motor_id_list.isNull()) || !(motor_id_list.is<JsonArray>())){
        printf("invalid motor_control field inside payload (expected non-null int array)\n");
        return -1; 
    }
    for (JsonVariant motor_id : motor_id_list.as<JsonArray>()) {
        if (!motor_id.is<int>()) {
            printf("invalid motor_control field inside payload (expected array of int)\n");
            return -1;
        }
    }

    // payload struct building
    JsonArray motor_id_list_array = motor_id_list.as<JsonArray>();
    int array_size = motor_id_list_array.size();
    array_size = (array_size < MAX_MOTORS_COUNT)?array_size:MAX_MOTORS_COUNT; // max size clip (some kind of basic protection)

    for(int i = 0; i < (array_size); i++){
        motor_control_payload->motor_ids[i] = (Motors_t)motor_id_list_array[i].as<int>();           // the terminal value must be inserted to understand the actual size of the list
    }
    motor_control_payload->direction = direction;
    motor_control_payload->speed = speed;
    motor_control_payload->angle = angle;
    motor_control_payload->duration = duration_ms;
    return 0;
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
    
    int get_value = -1;                                             // default to -1 to signal a potential error
    switch(get_config_payload->prop){                               // amazing job of vscode shortcuts for these lines
        case ConfigFields_t::ConfigFields_SPEED:
            get_value = self_prop_get_speed();
        case ConfigFields_t::ConfigFields_FEEDBACK:
            get_value = self_prop_get_feedback();
        case ConfigFields_t::ConfigFields_DEBUG:
            get_value = self_prop_get_debug();
        case ConfigFields_t::ConfigFields_NAVIGATION_TYPE:
            get_value = self_prop_get_navigation_type();
        case ConfigFields_t::ConfigFields_ROUTE_POLICY:
            get_value = self_prop_get_route_policy();
        case ConfigFields_t::ConfigFields_RADAR:
            get_value = self_prop_get_radar();
        case ConfigFields_t::ConfigFields_SCREEN:
            get_value = self_prop_get_screen();
        case ConfigFields_t::ConfigFields_OBSTACLE_CLEANER:
            get_value = self_prop_get_obstacle_cleaner();
        case ConfigFields_t::ConfigFields_OBJECT_LOADER:
            get_value = self_prop_get_object_loader();
            case ConfigFields_t::ConfigFields_OBJECT_UNLOADER:
            get_value = self_prop_get_object_unloader();
        case ConfigFields_t::ConfigFields_OBJECT_COMPACTER:
            get_value = self_prop_get_object_compacter();
        default:
            printf("unrecognized value for prop (configfields) value: %d\n", get_config_payload->prop);
            reply_packet["payload"]["status"] = ActionResult_t::ActionResult_FAILURE;               // consume here the error by setting a "failure warning" message back ("value" will be -1)
        break;
    }
    reply_packet["payload"]["value"] = get_value;

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

// call the set function for the correct property to change. The new value is assigned using main_robot functions 
// (any error is consumed with failure feedback)
int set_config_handler(uint16_t command_uuid, SetConfigPayload_t* set_config_payload){
    JsonObject reply_packet;
    set_message_common_headers(reply_packet);
    reply_packet["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
    reply_packet["request_uuid"] = command_uuid;                    // use the same as the command/request so that the server can track the source event for this reply
    reply_packet["payload"]["status"] = ActionResult_t::ActionResult_SUCCESS;                       // default to success, changed if any later step fails
    
    int int_new_value = (set_config_payload->new_value);   // common copy here as int, for now the only property type (used as enum index)
    
    switch(set_config_payload->prop){
        case ConfigFields_t::ConfigFields_SPEED:
            self_prop_set_speed(int_new_value);
        case ConfigFields_t::ConfigFields_FEEDBACK:
            self_prop_set_feedback(int_new_value);
        case ConfigFields_t::ConfigFields_DEBUG:
            self_prop_set_debug(int_new_value);
        case ConfigFields_t::ConfigFields_NAVIGATION_TYPE:
            self_prop_set_navigation_type(int_new_value);
        case ConfigFields_t::ConfigFields_ROUTE_POLICY:
            self_prop_set_route_policy(int_new_value);
        case ConfigFields_t::ConfigFields_RADAR:
            self_prop_set_radar(int_new_value);
        case ConfigFields_t::ConfigFields_SCREEN:
            self_prop_set_screen(int_new_value);
        case ConfigFields_t::ConfigFields_OBSTACLE_CLEANER:
            self_prop_set_obstacle_cleaner(int_new_value);
        case ConfigFields_t::ConfigFields_OBJECT_LOADER:
            self_prop_set_object_loader(int_new_value);
            case ConfigFields_t::ConfigFields_OBJECT_UNLOADER:
            self_prop_set_object_unloader(int_new_value);
        case ConfigFields_t::ConfigFields_OBJECT_COMPACTER:
            self_prop_set_object_compacter(int_new_value);
        default:
            printf("unrecognized value for prop (configfields) value: %d. Wanted (new) int value: %d\n", set_config_payload->prop, int_new_value);
            reply_packet["payload"]["status"] = ActionResult_t::ActionResult_FAILURE;               // consume here the error by setting a "failure warning" message back
        break;
    }

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);                      // error of the sending procedure ignored (set RETRY_SEND_MESSAGE to 1 in udp_client.h to perform multiple attempts in the (socket) sending procedure)
    return 0;
}

// checking the current robot state and calling soft or hard reset function
// (any error is consumed with failure feedback)
int reset_handler(uint16_t command_uuid, ResetPayload_t* reset_payload){
    JsonObject reply_packet;
    set_message_common_headers(reply_packet);
    reply_packet["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
    reply_packet["request_uuid"] = command_uuid;
    reply_packet["payload"]["status"] = ActionResult_t::ActionResult_SUCCESS;
    int error_feedback = 0;

    if(strncmp(reset_payload->reset, "reset", strlen("reset")) != 0){
        // send back a failure feedback (expected "reset" keyword as unique field with same key and val)
        printf("unrecognized value for reset command: '%s'. Expected string: 'reset'\n",reset_payload->reset);
        error_feedback = 1;
    }else{
        // call the main_robot function self_get_robot_status() to get an index of the enum RobotState_t
        // check for which type of activations to call (idle/busy context -> reset the entire board; emergency stop/error -> force a clear of the error state)
        // finally call the main_robot functions self_hard_reset() or self_soft_reset() 
        RobotState_t current_robot_state = (RobotState_t)self_prop_get_robot_status();
        switch(current_robot_state){
            RobotState_ERR:
                printf("calling soft_reset function ... \n");
                self_soft_reset();
                break;
            RobotState_BUSY:
            RobotState_IDLE:
            default:                                                // unrecognized robot state triggers an hard reset
                printf("calling hard_reset function ... \n");
                self_hard_reset();
                break;
        }
    }
    reply_packet["payload"]["status"] = (error_feedback)?ActionResult_t::ActionResult_FAILURE:ActionResult_t::ActionResult_SUCCESS;
    
    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

int emergency_stop_handler(uint16_t command_uuid, EmergencyStopPayload_t* emergency_stop_payload){
    JsonObject reply_packet;
    set_message_common_headers(reply_packet);
    reply_packet["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
    reply_packet["request_uuid"] = command_uuid;
    reply_packet["payload"]["status"] = ActionResult_t::ActionResult_SUCCESS;
    
    printf("calling emergency_stop function ... \n");
    self_emergency_stop();

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

int motor_control_handler(uint16_t command_uuid, MotorControlPayload_t* motor_control_payload){
    // TODO: GOON here @.@
    // check if the robot is busy too!
    // define which functions the main code should expose 
    // decide which type of activations is better to use (high level "move_forward" or low level "start_motor")
    return 0;
}

// Deserialize the Json from string argument "packet" and check for presence and type of common fields
// According to the incoming command the proper handler is called (with the parsed specific payload)
// Note: handler errors are ignored for now
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
            SetConfigPayload_t set_config_payload;
            if(get_set_config_payload(payload, &set_config_payload) < 0){
                printf("Invalid payload for set_config command\n");
                return -1;
            }    
            set_config_handler(request_id, &set_config_payload);
        break;
        case CommandType_t::CommandType_EMERGENCY_STOP:
            EmergencyStopPayload_t emergency_stop_payload;
            if(get_emergency_stop_payload(payload, &emergency_stop_payload) < 0){
                printf("Invalid payload for emergency_stop command\n");
                return -1;
            }    
            emergency_stop_handler(request_id, &emergency_stop_payload);
        break;
        case CommandType_t::CommandType_RESET:
            ResetPayload_t reset_payload;
            if(get_reset_payload(payload, &reset_payload) < 0){
                printf("Invalid payload for reset command\n");
                return -1;
            }    
            reset_handler(request_id, &reset_payload);
        break;
        case CommandType_t::CommandType_MOTOR_CONTROL:
        MotorControlPayload_t motor_control_payload;
            if(get_motor_control_payload(payload, &motor_control_payload) < 0){
                printf("Invalid payload for motor control command\n");
                return -1;
            }
            reset_handler(request_id, &motor_control_payload);
        break;
        case CommandType_t::CommandType_MOVE:
            // TODO: GOON here ç.ç
        break;
        default:
            printf("unrecognized command value: %d\n", command);
            return -1;
        break;
    }

    return 0;
}