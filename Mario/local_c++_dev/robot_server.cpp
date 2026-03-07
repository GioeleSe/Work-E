#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "udp_server.h"
#include "robot_server.h"


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

int PacketHandler(char* packet, ssize_t packet_size){
    // Guard against null or empty packets
    if (packet == nullptr || packet_size <= 0) {
        printf("PacketHandler: invalid packet (null or non-positive size)\n");
        return -1;
    }

    JsonDocument json_doc;
    DeserializationError json_doc_error = deserializeJson(json_doc, packet, packet_size);
    if (json_doc_error) {
        printf("deserializeJson() returned %s\n", json_doc_error.c_str());
        return -1;
    }

    // Check that message_type key exists and is the right type before casting
    if (!json_doc["message_type"].is<int>()) {
        printf("PacketHandler: missing or non-integer 'message_type'\n");
        return -1;
    }
    int message_type = json_doc["message_type"].as<int>();

    // Check that payload key exists
    if (json_doc["payload"].isNull()) {
        printf("PacketHandler: missing or null 'payload'\n");
        return -1;
    }

    // serializeJson returns 0 on failure (e.g. buffer too small)
    char payload_str[1024];
    size_t serialized_len = serializeJson(json_doc["payload"], payload_str, sizeof(payload_str));
    if (serialized_len == 0) {
        printf("PacketHandler: serializeJson() failed for payload\n");
        return -1;
    }

    // Warn if output was truncated (serialized_len >= buffer means truncation occurred)
    if (serialized_len >= sizeof(payload_str)) {
        printf("PacketHandler: warning - payload was truncated (serialized_len=%zu)\n", serialized_len);
    }

    printf("Message type: %d\n", message_type);
    printf("Payload: %s\n", payload_str);
    return 0;
}