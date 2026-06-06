#include "udp_client.h"
#include "udp_server.h"
#include "robot_server.h"
#include "main_robot.h"

#define ROBOT_NET_PROTOCOL "robot-net/1.0"

// -------------------------------- main code --------------------------------

void* message_server_thread(void* arg){
    platform_print("robot server - udp_server_thread - Server initialized. Calling listening function.\n");
    server_listen_port();                                           // will return only on error or on request
    platform_thread_exit();
    return NULL;
}

void* message_reader_thread(void* arg){
    platform_print("robot server - message_reader_thread - Reader started.\n");

    char message[BUFFER_SIZE];
    int msg_count = 0;
    int pop_result = 0;
    ssize_t message_length = 0;

    while(1){
        pop_result = server_buffer_pop(message);
        message_length = strlen(message);
        message[message_length] = '\0';
        if((pop_result == 0) && (message_length > 1)){
            platform_print("robot server - message_reader_thread - [msg #%d] received: '%s'\n", ++msg_count, message);
            PacketHandler(message, strlen(message));
        } else {
            // buffer empty, avoid busy-waiting
            platform_sleep_ms(1000);
        }
    }
    platform_thread_exit();
    return NULL;
}

#ifdef PLATFORM_LINUX
void handle_shutdown(int sig){
    platform_print("\nrobot server - shutting down\n");
    if(sig == SIGINT){                                              // "gracefully" (more or less)
        platform_sem_wait(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
        udp_server_data.stop_server = 1;
        platform_sem_post(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
    }else{
        udp_server_data.stop_server = 1;
    }
    exit(0);
}
#endif

int RobotStartServer(){
    platform_init_time();

#ifdef PLATFORM_LINUX
    setvbuf(stdout, NULL, _IOLBF, 0);                               // prevent platform_print buffering
    signal(SIGINT, handle_shutdown);
    signal(SIGTERM, handle_shutdown);
#endif

    platform_thread_t reader_tid, server_tid;
    server_init();

    if(platform_thread_create(&reader_tid, message_reader_thread, NULL, "msg_reader") != 0){
        platform_print("robot server - RobotStartServer - Failed to create reader thread.\n");
        return -1;
    }
    platform_print("robot server - RobotStartServer - Reader thread created.\n");

    if(platform_thread_create(&server_tid, message_server_thread, NULL, "msg_server") != 0){
        platform_print("robot server - RobotStartServer - Failed to create server thread.\n");
        return -1;
    }
    platform_print("robot server - RobotStartServer - Server thread created.\n");

    platform_thread_join(server_tid);
    platform_thread_join(reader_tid);
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
            platform_print("missing field %s in json packet\n", field_data.name);
            return -1;
        }else{
            if(!field_data.check(field_val)){
                platform_print("wrong type of field %s in json packet\n", field_data.name);
                return -1;
            }
        }
    }
    return 0;
}

// check for command-type message (== assert "message_type" = 0)
// and payload command field existence and value
int check_command(const JsonObject& json_payload){
    JsonVariant payload_command = json_payload["command"];
    if((payload_command.isNull()) || !(payload_command.is<int>())){
        platform_print("invalid command field inside payload (expected non-null integer)\n");
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
    reply_doc["request_uuid"] = (uint16_t)(rand()%65535);
    reply_doc["mode"] = (int)MessageMode_t::MessageMode_MANUAL;
    reply_doc["timestamp"] = time(NULL);
}

// parse the payload to the specific get_config expected structure
int get_get_config_payload( const JsonObject& json_payload, GetConfigPayload_t* get_config_payload){
    JsonVariant prop_name = json_payload["prop"];
    if((prop_name.isNull()) || !(prop_name.is<int>())){
        platform_print("invalid property field inside payload (expected non-null integer)\n");
        return -1;
    }
    int prop_name_int = prop_name.as<int>();
    if(prop_name_int < 0){
        platform_print("unrecognized value for prop (configfields) value: %d\n", prop_name_int);
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
        platform_print("invalid property field inside payload (expected non-null integer)\n");
        return -1;
    }
    if((prop_value.isNull()) || !(prop_value.is<int>())){
        platform_print("invalid property value field inside payload (expected non-null integer)\n");
        return -1;
    }
    int prop_name_int = prop_name.as<int>();
    int prop_value_int = prop_value.as<int>();
    if(prop_name_int < 0){                                          // prop value is allowed to be negative (hence not checked here)
        platform_print("unrecognized value for prop (configfields) value: %d\n", prop_name_int);
        return -1;
    }
    set_config_payload->prop = (ConfigFields_t)prop_name_int;
    set_config_payload->new_value = prop_value_int;
    return 0;
}

// parse the payload to the specific reset expected structure
// return -1 for null or invalid type field
int get_reset_payload(const JsonObject& json_payload, ResetPayload_t* reset_payload){
    JsonVariant reset_keyword = json_payload["reset"];
    if((reset_keyword.isNull()) || !(reset_keyword.is<const char*>())){
        platform_print("invalid reset field inside payload (expected non-null string)\n");
        return -1;
    }
    const char* reset_keyword_str = reset_keyword.as<const char*>();
    strncpy(reset_payload->reset, reset_keyword_str, strlen(reset_keyword_str));                    // no others length check measures for now, C trust @.@
    reset_payload->reset[sizeof(reset_payload->reset) - 1] = '\0';
    return 0;
}

// parse the payload to the specific emergency_stop expected structure
// return -1 for null or invalid type field
int get_emergency_stop_payload(const JsonObject& json_payload, EmergencyStopPayload_t* emergency_stop_payload) {
    JsonVariant emergency_stop_keyword = json_payload["stop"];
    if((emergency_stop_keyword.isNull()) || !(emergency_stop_keyword.is<const char*>())){
        platform_print("invalid emergency_stop field inside payload (expected non-null string)\n");
        return -1;
    }
    const char* emergency_stop_keyword_str = emergency_stop_keyword.as<const char*>();
    strncpy(emergency_stop_payload->stop, emergency_stop_keyword_str, strlen(emergency_stop_keyword_str));
    esp->stop[sizeof(esp->stop) - 1] = '\0';
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
            platform_print("invalid motor_control field inside payload (expected non-null int)\n");
            return -1;
        }
    }
    if((motor_id_list.isNull()) || !(motor_id_list.is<JsonArray>())){
        platform_print("invalid motor_control field inside payload (expected non-null int array)\n");
        return -1;
    }
    for (JsonVariant motor_id : motor_id_list.as<JsonArray>()) {
        if (!motor_id.is<int>()) {
            platform_print("invalid motor_control field inside payload (expected array of int)\n");
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
    motor_control_payload->direction = (Direction_t)direction.as<int>();
    motor_control_payload->speed = speed.as<int>();
    motor_control_payload->angle = angle.as<int>();
    motor_control_payload->duration = duration_ms.as<int>();
    return 0;
}

// parse the payload to the specific move command expected structure
// return -1 for null or invalid type fields
int get_move_payload(const JsonObject& json_payload, MovePayload_t* move_payload){
    JsonVariant destination_x = json_payload["destination_x"];
    JsonVariant destination_y = json_payload["destination_y"];
    JsonVariant destination_checkpoint = json_payload["destination_checkpoint"];
    JsonVariant navigation_type = json_payload["navigation_type"];
    JsonVariant route_policy = json_payload["route_policy"];

    JsonVariant int_fields_list[] = {destination_x, destination_y, destination_checkpoint, navigation_type, route_policy};
    for(JsonVariant field : int_fields_list){
        if((field.isNull()) || (!field.is<int>())){
            platform_print("invalid move field inside payload (expected non-null int)\n");
            return -1;
        }
    }
    move_payload->destination_x = destination_x.as<int>();
    move_payload->destination_y = destination_y.as<int>();
    move_payload->destination_checkpoint = destination_checkpoint.as<int>();
    move_payload->navigation_type = navigation_type.as<int>();
    move_payload->route_policy = route_policy.as<int>();
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
        break;
        case ConfigFields_t::ConfigFields_FEEDBACK:
            get_value = self_prop_get_feedback();
        break;
        case ConfigFields_t::ConfigFields_DEBUG:
            get_value = self_prop_get_debug();
        break;
        case ConfigFields_t::ConfigFields_NAVIGATION_TYPE:
            get_value = self_prop_get_navigation_type();
        break;
        case ConfigFields_t::ConfigFields_ROUTE_POLICY:
            get_value = self_prop_get_route_policy();
        break;
        case ConfigFields_t::ConfigFields_RADAR:
            get_value = self_prop_get_radar();
        break;
        case ConfigFields_t::ConfigFields_SCREEN:
            get_value = self_prop_get_screen();
        break;
        case ConfigFields_t::ConfigFields_OBSTACLE_CLEANER:
            get_value = self_prop_get_obstacle_cleaner();
        break;
        case ConfigFields_t::ConfigFields_OBJECT_LOADER:
            get_value = self_prop_get_object_loader();
        break;
        case ConfigFields_t::ConfigFields_OBJECT_UNLOADER:
            get_value = self_prop_get_object_unloader();
        break;
        case
            ConfigFields_t::ConfigFields_OBJECT_COMPACTER:
            get_value = self_prop_get_object_compacter();
        break;
        default:
            platform_print("unrecognized value for prop (configfields) value: %d\n", get_config_payload->prop);
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
        break;
        case ConfigFields_t::ConfigFields_FEEDBACK:
            self_prop_set_feedback(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_DEBUG:
            self_prop_set_debug(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_NAVIGATION_TYPE:
            self_prop_set_navigation_type(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_ROUTE_POLICY:
            self_prop_set_route_policy(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_RADAR:
            self_prop_set_radar(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_SCREEN:
            self_prop_set_screen(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_OBSTACLE_CLEANER:
            self_prop_set_obstacle_cleaner(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_OBJECT_LOADER:
            self_prop_set_object_loader(int_new_value);
            break;
        case ConfigFields_t::ConfigFields_OBJECT_UNLOADER:
            self_prop_set_object_unloader(int_new_value);
        break;
        case ConfigFields_t::ConfigFields_OBJECT_COMPACTER:
            self_prop_set_object_compacter(int_new_value);
        break;
        default:
            platform_print("unrecognized value for prop (configfields) value: %d. Wanted (new) int value: %d\n", set_config_payload->prop, int_new_value);
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
        platform_print("unrecognized value for reset command: '%s'. Expected string: 'reset'\n",reset_payload->reset);
        error_feedback = 1;
    }else{
        // call the main_robot function self_get_robot_status() to get an index of the enum RobotState_t
        // check for which type of activations to call (idle/busy context -> reset the entire board; emergency stop/error -> force a clear of the error state)
        // finally call the main_robot functions self_hard_reset() or self_soft_reset()
        RobotState_t current_robot_state = (RobotState_t)self_prop_get_robot_state();
        switch(current_robot_state){
            case RobotState_t::RobotState_ERR:
                platform_print("calling soft_reset ...\n");
                self_soft_reset();
                break;
            case RobotState_t::RobotState_BUSY:
            case RobotState_t::RobotState_IDLE:
            default:                                                // unrecognized robot state triggers an hard reset
                platform_print("calling hard_reset function ... \n");
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

    platform_print("calling emergency_stop function ... \n");
    self_emergency_stop();

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

// Perform de-multipling from motor_control functionality of the UI server to the almost high level functions like // Perform de-multipling from motor_control functionality of the UI server to the almost high level functions like moveself_motion_car_proceed
// From this function the robot will send back error, pending or success feedback according to the command shape.
// Note: for now no check on the actual activation functions is implemented (even if they're defined as int functions)
int motor_control_handler(uint16_t command_uuid, MotorControlPayload_t* motor_control_payload){
    JsonObject reply_packet;
    set_message_common_headers(reply_packet);
    reply_packet["request_uuid"] = command_uuid;
    reply_packet["message_type"] = (int)MessageType_t::MessageType_FEEDBACK;
    FeedbackMsg_t feedback_msg;

    int route_command = 0;
    int pending_command = 0;

    RobotState_t robot_state = (RobotState_t)self_prop_get_robot_state();
    int speed = motor_control_payload->speed;
    int duration = motor_control_payload->duration;
    int angle = motor_control_payload->angle;
    Direction_t direction = motor_control_payload->direction;
    switch(robot_state){
        case RobotState_t::RobotState_IDLE:
            route_command = 1;
            break;
        case RobotState_t::RobotState_BUSY:
            if(REFUSE_BUSY_TASK){
                feedback_msg.error_code = ErrorCode_t::ErrorCode_ROBOT_BUSY_STATE;
                strncpy(feedback_msg.error_message, REFUSE_BUSY_TASK_MSG, strlen(REFUSE_BUSY_TASK_MSG));
                feedback_msg.status = ActionResult_t::ActionResult_FAILURE;
            }else{
                // send a PENDING state for this task but call directly the needed function. *No function buffer* is used (too complicated for this scope)
                feedback_msg.error_code = ErrorCode_t::ErrorCode_ROBOT_BUSY_STATE;
                feedback_msg.status = ActionResult_t::ActionResult_PENDING;
                route_command = 1;
                pending_command = 1;
            }
            break;
        case RobotState_t::RobotState_ERR:                          // internal error state -> send back an error
        default:
            // send an ERROR feedback message
            feedback_msg.error_code = ErrorCode_t::ErrorCode_ROBOT_ERROR_STATE;
            feedback_msg.status = ActionResult_t::ActionResult_FAILURE;
            strncpy(feedback_msg.error_message, REFUSE_INTERNAL_ERROR_MSG, strlen(REFUSE_INTERNAL_ERROR_MSG));
            break;
    }

    if(route_command){
        if(motor_control_payload->motor_ids[0] == Motors_t::Motors_END_MOT){  // no motors -> drive as a car
            // NOTE: the speed parameter is ignored in car driving, the robot is expected to use the **internal** speed property
            // if(speed == 0) self_motion_car_stop();
            switch(direction){
                case Direction_t::Direction_LEFT:
                case Direction_t::Direction_RIGHT:
                    self_motion_car_rotate(direction);
                break;
                case Direction_t::Direction_BACKWARD:
                case Direction_t::Direction_FORWARD:
                    self_motion_car_proceed(direction);
                break;
                case Direction_t::Direction_STOP:
                default:                                            // stop the robot even if the direction has an invalid value (safety and common sense)
                    self_motion_car_stop();
                break;
            }
        }else{
            int index = 0;
            Motors_t motor_id = motor_control_payload->motor_ids[index];

            while((motor_id != Motors_t::Motors_END_MOT) && (index < MAX_MOTORS_COUNT)){ // start or stop each single motor
                if(angle != 0){
                    self_motion_steer_servo(motor_id, angle);
                }else{
                    if(speed == 0){
                        self_motion_stop_motor(motor_id);           // allow to stop motors even with speed=0 (below it's the Direction_STOP alternative)
                    }else{
                        switch(direction){
                            case Direction_t::Direction_BACKWARD:
                            case Direction_t::Direction_FORWARD:
                                self_motion_activate_dc_motor(motor_id, direction, speed, motor_control_payload->duration);
                            break;
                            case Direction_t::Direction_STOP:
                            default:
                                self_motion_stop_motor(motor_id);
                            break;
                        }
                    }
                }
                motor_id = motor_control_payload->motor_ids[++index];
            }
        }
    }
    // for now no checks are implemented on the actual activations (defined as int functions for possible extensions)
    if(!pending_command){
        feedback_msg.error_code = ErrorCode_t::ErrorCode_NO_ERROR;
        feedback_msg.status = ActionResult_t::ActionResult_SUCCESS;
        strncpy(feedback_msg.error_message, TASK_DONE_MSG, strlen(TASK_DONE_MSG));
    }

    reply_packet["payload"]["status"] = feedback_msg.status;
    reply_packet["payload"]["error_code"] = feedback_msg.error_code;
    reply_packet["payload"]["error_message"] = feedback_msg.error_message;

    char reply_packet_serialized[BUFFER_SIZE];
    ssize_t reply_packet_serialized_size = serializeJson(reply_packet, reply_packet_serialized);
    client_send_packet(reply_packet_serialized, reply_packet_serialized_size);
    return 0;
}

// Dummy stub. Move command is not implemented as the required hardware was too complex (to obtain a valid movement sensors framework)
int move_handler(uint16_t command_uuid, MovePayload_t* move_payload){
    // Needed segments to handle this command:
    //  check the type of command according to the parameters
    //  fetch of the given checkpoint (checkpoint based only)
    //  route generation to the given destination according to the navigation type (x-y coordinates)
    platform_print("Received move command with destination (x,y): (%d,%d), (checkpoint): (%d) and parameters:\n\tnavigation type:%d,\t route policy:%d\n", move_payload->destination_x, move_payload->destination_y, move_payload->destination_checkpoint, move_payload->navigation_type, move_payload->route_policy);
    return 0;
}

// Deserialize the Json from string argument "packet" and check for presence and type of common fields
// According to the incoming command the proper handler is called (with the parsed specific payload)
// Note: handler errors are ignored for now
int PacketHandler(char* packet, ssize_t packet_size){
    if (packet == nullptr || packet_size <= 0) {
        platform_print("PacketHandler: invalid packet pointer\n");
        return -1;
    }

    JsonDocument json_doc;
    DeserializationError json_doc_error = deserializeJson(json_doc, packet, packet_size);
    if (json_doc_error) {
        platform_print("deserializeJson() returned %s\n", json_doc_error.c_str());
        return -1;
    }
    if(check_fields(json_doc)<0){
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
        platform_print("invalid command value (expected value: 0)\n");
        return -1;

    }

    int command = check_command(payload);
    switch(command){
        case CommandType_t::CommandType_GET_PROPERTY:
            GetConfigPayload_t get_config_payload;
            if(get_get_config_payload(payload, &get_config_payload) < 0){
                platform_print("Invalid payload for get_config command\n");
                return -1;
            }
            get_config_handler(request_id, &get_config_payload);
        break;
        case CommandType_t::CommandType_SET_PROPERTY:
            SetConfigPayload_t set_config_payload;
            if(get_set_config_payload(payload, &set_config_payload) < 0){
                platform_print("Invalid payload for set_config command\n");
                return -1;
            }
            set_config_handler(request_id, &set_config_payload);
        break;
        case CommandType_t::CommandType_EMERGENCY_STOP:
            EmergencyStopPayload_t emergency_stop_payload;
            if(get_emergency_stop_payload(payload, &emergency_stop_payload) < 0){
                platform_print("Invalid payload for emergency_stop command\n");
                return -1;
            }
            emergency_stop_handler(request_id, &emergency_stop_payload);
        break;
        case CommandType_t::CommandType_RESET:
            ResetPayload_t reset_payload;
            if(get_reset_payload(payload, &reset_payload) < 0){
                platform_print("Invalid payload for reset command\n");
                return -1;
            }
            reset_handler(request_id, &reset_payload);
        break;
        case CommandType_t::CommandType_MOTOR_CONTROL:
            MotorControlPayload_t motor_control_payload;
            if(get_motor_control_payload(payload, &motor_control_payload) < 0){
                platform_print("Invalid payload for motor control command\n");
                return -1;
            }
            motor_control_handler(request_id, &motor_control_payload);
        break;
        case CommandType_t::CommandType_MOVE:
            MovePayload_t move_payload;
            if(get_move_payload(payload, &move_payload) < 0){
                platform_print("Invalid payload for move command\n");
                return -1;
            }
            move_handler(request_id, &move_payload);
        break;
        default:
            platform_print("unrecognized command value: %d\n", command);
            return -1;
        break;
    }
    return 0;
}