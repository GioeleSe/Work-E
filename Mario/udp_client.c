#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "udp_client.h"

int socket_init(){
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);                        // SOCK_DGRAM = connectionless, fixed max_length
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }else{
        printf("client - socket created\n");
    }
    return sockfd;
}

int socket_send_message(const int sockfd, const char* msg, const size_t msg_size, const struct sockaddr* server_info, socklen_t* size_of_server_info){
    int send_result = sendto(sockfd, msg, msg_size, MSG_CONFIRM, server_info, *size_of_server_info);
    printf("client - message sent.\n");
    return send_result;
}

ssize_t socket_get_message(const int sockfd, char* buffer, const int buffer_size, struct sockaddr* server_info, socklen_t* size_of_server_info){
    ssize_t n = recvfrom(sockfd, buffer, (size_t)buffer_size, 0, server_info, size_of_server_info);
    if(!n){
        printf("client - Receive function error, exit code %ld\n", n);
        exit(EXIT_FAILURE);
    }else{
        buffer[n] = '\0';
        printf("client - Received %ld bytes\n",n);
    }
    return n;
}

void client_socket_set_server_info(struct sockaddr_in* server_info, const int server_port, const char* server_address){
    server_info->sin_family = AF_INET;
    server_info->sin_port = htons(server_port);
    server_info->sin_addr.s_addr = inet_addr(server_address);
}

// mainly function used from other files.
// initialize the socket and send a string message
// return -1 if the send failed
// Note: use function serializeJson(json_doc, char_msg) to pass the string message
int client_send_packet(char* msg, ssize_t msg_size){
    struct sockaddr_in server_info;
    int sockfd = socket_init();
    client_socket_set_server_info(&server_info, SERVER_PORT, SERVER_ADDRESS);  
    socklen_t size_of_server_info = (socklen_t)sizeof(server_info);
    int send_result = socket_send_message(sockfd, msg, msg_size, (struct sockaddr *)&server_info, &size_of_server_info);
    if((RETRY_SEND_MESSAGE) && (send_result < 0)){
        int attempt_counter = 0;
        do{
            send_result = socket_send_message(sockfd, msg, msg_size, (struct sockaddr *)&server_info, &size_of_server_info);
            attempt_counter++;
        }while(send_result < 0 && (attempt_counter < RETRY_SEND_MESSAGE_MAX_ATTEMPTS));
    }
    return send_result;
}

int client_main_test() {
    // send an UDP message to the server
    const char *msg = "Hello from client";
    struct sockaddr_in server_info;
    
    int sockfd = socket_init();
    client_socket_set_server_info(&server_info, SERVER_PORT, SERVER_ADDRESS);
    
    socklen_t size_of_server_info = (socklen_t)sizeof(server_info);
    socket_send_message(sockfd, msg, (size_t)strlen(msg), (struct sockaddr *)&server_info, &size_of_server_info);
    
    
    
    // Receive a message from the same server
    char buffer[BUFFER_SIZE];
    ssize_t recv_bytes = 0;

    recv_bytes = socket_get_message(sockfd, buffer, BUFFER_SIZE, (struct sockaddr *)&server_info, &size_of_server_info);
    printf("client - Received message(%ld): '%s' from server\n", recv_bytes, buffer);
    


    close(sockfd);
    return 0;
}