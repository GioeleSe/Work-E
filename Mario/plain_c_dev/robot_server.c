#include <stdio.h>
#include <pthread.h>
#include<unistd.h>
#include "udp_server.h"

void* message_reader_thread(void* arg){
    char message[BUFFER_SIZE];
    int msg_count = 0;
    
    while(1){
        int ret = server_buffer_pop(message);
        if(ret == 0){
            printf("[msg #%d] received: '%s'\n", ++msg_count, message);
        } else {
            // buffer empty, avoid busy-waiting
            sleep(1);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){
    udp_server_data_t* udp_server_data;
    udp_server_data = server_init();
    printf("Server initialized.\n");

    pthread_t reader_tid;
    if(pthread_create(&reader_tid, NULL, message_reader_thread, NULL) != 0){
        perror("failed to create reader thread");
        return 1;
    }
    printf("Starting port listening.\n");
    server_listen_port();

    pthread_join(reader_tid, NULL);
    return 0;
}