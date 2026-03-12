#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <netinet/in.h>

#include "udp_server.h"
udp_server_data_t udp_server_data;                                  // global server data

int server_socket_init(){
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);                        // SOCK_DGRAM = connectionless, fixed max_length
    if (sockfd < 0) {
        perror("server - socket creation failed");
        exit(EXIT_FAILURE);
    }else{
        printf("server - socket created\n");
    }
    return sockfd;
}

void server_socket_set_server_info(struct sockaddr_in* server_info, const int server_port){
    server_info->sin_family = AF_INET;
    server_info->sin_port = htons(server_port);
    server_info->sin_addr.s_addr = INADDR_ANY;
}

void server_socket_bind(const int sockfd, struct sockaddr_in* server_info, socklen_t* size_of_server_info){
    
    printf("binding on port %d, fd %d, size %d\n", 
    ntohs(server_info->sin_port), 
    sockfd, 
    *size_of_server_info);

    if (bind(sockfd, (struct sockaddr *)server_info, *size_of_server_info) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }else{
        printf("server - bind successfully \n");
    }
}

ssize_t server_socket_wait_packets(const int sockfd, char* buffer, const int buffer_size, struct sockaddr* client_info, socklen_t* size_of_client_info){
    ssize_t n = recvfrom(sockfd, buffer, buffer_size, 0, client_info, size_of_client_info);
    if(n){
        buffer[n] = '\0';
    }
    return n;
}

void server_socket_send_message(const int sockfd, const char* msg, const size_t msg_size, const struct sockaddr* client_info, socklen_t* size_of_client_info){
    ssize_t n = sendto(sockfd, msg, msg_size, 0, client_info, *size_of_client_info);
    if(!n){
        printf("server - sendto function error, exit code %ld\n", n);
        exit((int)n);
    }else{
        printf("server - message sent\n");
    }
}


int server_udp_channel_test()
{
    struct sockaddr_in server_info, client_info;
    
    int sockfd = server_socket_init();
    server_socket_set_server_info(&server_info, SERVER_PORT);

    socklen_t size_of_server_info = (socklen_t)sizeof(server_info);
    server_socket_bind(sockfd, &server_info, &size_of_server_info);
    
    char buffer[BUFFER_SIZE];
    socklen_t size_of_client_info = (socklen_t)sizeof(client_info);
    ssize_t recv_bytes = server_socket_wait_packets(sockfd, buffer, BUFFER_SIZE, (struct sockaddr *)&client_info, &size_of_client_info);
    printf("server - Received message(%ld): '%s' from client\n", recv_bytes, buffer);
    
    const char *msg = "Hello from server";
    server_socket_send_message(sockfd, msg,  (size_t)strlen(msg), (const struct sockaddr *)&client_info, &size_of_client_info);
    printf("server - replied to client correctly\n");

    return 0;
}


// initialize the server with basic info (socket type, port to listen) and bind it. Initialize the buffer semaphore as well.
// return (udp_server_data_t*) reference to server data structure that contains the msg buffer too
udp_server_data_t* server_init(){
    udp_server_data.socket_fd = server_socket_init();
    memset(&(udp_server_data.server_info), 0, sizeof(udp_server_data.server_info));
    memset(&(udp_server_data.client_info), 0, sizeof(udp_server_data.client_info));

    server_socket_set_server_info((struct sockaddr_in*)&udp_server_data.server_info, SERVER_PORT);

    udp_server_data.size_of_server_info = (socklen_t)sizeof(udp_server_data.server_info);
    server_socket_bind(udp_server_data.socket_fd, (struct sockaddr_in*)&udp_server_data.server_info, &udp_server_data.size_of_server_info);

    sem_init(&udp_server_data.udp_server_buffer.buffer_messages_mutex, 0, 1);
    sem_init(&udp_server_data.udp_server_buffer.buffer_messages_counting_sem, 0, 0);
    udp_server_data.udp_server_buffer.buffer_max_size = BUFFER_SIZE;
    udp_server_data.udp_server_buffer.buffer_max_line_size = BUFFER_SIZE;
    return &udp_server_data;
}

int server_buffer_push(char* data,  int size_of_data){
    udp_server_buffer_t* buffer_data = &udp_server_data.udp_server_buffer;
    sem_wait(&(buffer_data->buffer_messages_mutex));

    int head = buffer_data->buffer_messages_head;
    int tail = buffer_data->buffer_messages_tail;
    int max_size = buffer_data->buffer_max_size;
    int max_line_size = buffer_data->buffer_max_line_size;
    int next = (head + 1 == max_size) ? 0 : head + 1;
    
    if(buffer_data->buffer_is_full){
        sem_post(&buffer_data->buffer_messages_mutex);
        return -1; // drop the message, buffer full
    } 
    
    int saturation_size_of_data = (size_of_data > max_line_size)? max_line_size:size_of_data;
    // printf("push: data='%s' size=%d max_line=%d saturated=%d\n", data, size_of_data, max_line_size, saturation_size_of_data);
    memset(buffer_data->buffer_messages[head], 0, max_line_size);
    strncpy((buffer_data->buffer_messages[head]), data, saturation_size_of_data);
    buffer_data->buffer_messages[head][saturation_size_of_data] = '\0';

    if(next == tail){
        buffer_data->buffer_is_full = 1;
    }
    buffer_data->buffer_messages_head = next;

    sem_post(&buffer_data->buffer_messages_counting_sem);
    sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

int server_buffer_pop(char* dest_data){
    udp_server_buffer_t* buffer_data = &udp_server_data.udp_server_buffer;
    sem_wait(&buffer_data->buffer_messages_counting_sem);
    sem_wait(&buffer_data->buffer_messages_mutex);
    
    int head = buffer_data->buffer_messages_head;
    int tail = buffer_data->buffer_messages_tail;
    int max_size = buffer_data->buffer_max_size;
    int max_line_size = buffer_data->buffer_max_line_size;
    int next = ((tail + 1) == max_size) ? 0 : tail + 1;

    char* src_data = buffer_data->buffer_messages[tail];
    int size_of_src_data = strlen(src_data);
    strncpy(dest_data, src_data, size_of_src_data);
    dest_data[size_of_src_data] = '\0';  
    
    buffer_data->buffer_messages_tail = next;
    buffer_data->buffer_is_full = 0;

    sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

void server_listen_port(){
    int buffer_is_full = 0;
    char single_message_buffer[BUFFER_SIZE];
    ssize_t recv_bytes = 0;
    char should_exit = 0;

    udp_server_data.size_of_client_info = (socklen_t)sizeof(udp_server_data.client_info);           // shared but shouldn't be used (C trust)
    do{
        sem_wait(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
        buffer_is_full = udp_server_data.udp_server_buffer.buffer_is_full;
        sem_post(&udp_server_data.udp_server_buffer.buffer_messages_mutex);

        while((!buffer_is_full) && (!udp_server_data.stop_server)){        // get new messages up to max capacity (no message priority)
            recv_bytes = server_socket_wait_packets(udp_server_data.socket_fd, single_message_buffer, BUFFER_SIZE, (struct sockaddr *)&udp_server_data.client_info, &udp_server_data.size_of_client_info);
            if(recv_bytes){
                int push_ret = server_buffer_push(single_message_buffer, recv_bytes);
                if (push_ret == -1){
                    printf("Server buffer is full. New messages will not be received! \n");
                }
            }else{
                continue;
            }
        }
        if(udp_server_data.stop_server){                            // destroy the semaphore and the socket 
            sem_destroy(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
            sem_destroy(&udp_server_data.udp_server_buffer.buffer_messages_counting_sem);
            close(udp_server_data.socket_fd);
            should_exit = 1;
        }
        sleep(3);                                                   // wait to avoid keeping busy the semaphore 
    }while(!should_exit);
}