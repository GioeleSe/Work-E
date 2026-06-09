#include "common_platform_abstr.h"
#include "udp_client.h"

platform_socket_fd_t socket_init(){
    platform_socket_fd_t sockfd;
    sockfd = platform_socket_create_udp();
    if (sockfd < 0) {
        platform_print("client - socket creation failed");
        // exit(EXIT_FAILURE);
        platform_panic(-1);
    }else{
        platform_print("client - socket created\n");
    }
    return sockfd;
}

int socket_send_message(const platform_socket_fd_t sockfd, const char* msg, const platform_ssize_t msg_size, const platform_sockaddr_t* server_info, platform_socklen_t* size_of_server_info){
    int send_result = platform_socket_sendto(sockfd, msg, msg_size, server_info, *size_of_server_info);
    platform_print("client - message sent.\n");
    return send_result;
}

platform_ssize_t socket_get_message(const platform_socket_fd_t sockfd, char* buffer, const int buffer_size, platform_sockaddr_t* server_info, platform_socklen_t* size_of_server_info){
    platform_ssize_t n = platform_socket_recvfrom(sockfd, buffer, (platform_ssize_t)buffer_size, server_info, size_of_server_info);
    if(n<0){
        platform_print("client - Receive function error, exit code %ld\n", n);
        platform_panic(-1);
    }else{
        if (n > 0 && n < buffer_size) {
            buffer[n] = '\0';
        }
        platform_print("client - Received %ld bytes\n",n);
    }
    return n;
}

void client_socket_set_server_info(platform_sockaddr_in_t* server_info, const int server_port, const char* server_address){
    server_info->sin_family = AF_INET;
    server_info->sin_port = htons(server_port);
    server_info->sin_addr.s_addr = inet_addr(server_address);       // provided by lwIP API too
}

// persistent socket — created once, reused for all sends
static platform_socket_fd_t _client_sockfd = -1;
static platform_sockaddr_in_t _client_server_info;
static platform_socklen_t _client_server_info_size;

static void client_ensure_socket(){
    if(_client_sockfd >= 0) return;
    _client_sockfd = platform_socket_create_udp();
    client_socket_set_server_info(&_client_server_info, SERVER_PORT, SERVER_ADDRESS);
    _client_server_info_size = (platform_socklen_t)sizeof(_client_server_info);
    platform_print("client - persistent socket created\n");
}

// mainly function used from other files.
// initialize the socket (once) and send a string message
// return -1 if the send failed
// Note: use function serializeJson(json_doc, char_msg) to pass the string message
int client_send_packet(char* msg, platform_ssize_t msg_size){
    client_ensure_socket();
    int send_result = socket_send_message(_client_sockfd, msg, msg_size, (platform_sockaddr_t *)&_client_server_info, &_client_server_info_size);
    if((RETRY_SEND_MESSAGE) && (send_result < 0)){
        // socket may be broken — recreate it and retry
        platform_socket_close(_client_sockfd);
        _client_sockfd = -1;
        client_ensure_socket();
        int attempt_counter = 0;
        do{
            send_result = socket_send_message(_client_sockfd, msg, msg_size, (platform_sockaddr_t *)&_client_server_info, &_client_server_info_size);
            attempt_counter++;
        }while(send_result < 0 && (attempt_counter < RETRY_SEND_MESSAGE_MAX_ATTEMPTS));
    }
    return send_result;
}

int client_main_test() {
    // send an UDP message to the server
    const char *msg = "Hello from client";
    platform_sockaddr_in_t server_info;
    
    platform_socket_fd_t sockfd = socket_init();
    client_socket_set_server_info(&server_info, SERVER_PORT, SERVER_ADDRESS);
    
    platform_socklen_t size_of_server_info = (platform_socklen_t)sizeof(server_info);
    socket_send_message(sockfd, msg, (platform_ssize_t)strlen(msg), (platform_sockaddr_t *)&server_info, &size_of_server_info);

    // Receive a message from the same server
    char buffer[BUFFER_SIZE];
    platform_ssize_t recv_bytes = 0;
    recv_bytes = socket_get_message(sockfd, buffer, BUFFER_SIZE, (platform_sockaddr_t *)&server_info, &size_of_server_info);
    platform_print("client - Received message(%ld): '%s' from server\n", recv_bytes, buffer);
    platform_socket_close(sockfd);
    return 0;
}