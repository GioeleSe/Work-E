#include "udp_server.h"

udp_server_data_t udp_server_data;                                  // global server data

platform_socket_fd_t server_socket_init(){
    platform_socket_fd_t sockfd;
    sockfd = platform_socket_create_udp();                          // SOCK_DGRAM = connectionless, fixed max_length
    if (sockfd < 0) {
        platform_print("server - socket creation failed");
        // exit(EXIT_FAILURE);
        platform_panic(-1);
    }else{
        platform_print("server - socket created\n");
    }
    return sockfd;
}

void server_socket_set_server_info(platform_sockaddr_in_t* server_info, const int server_port){
    server_info->sin_family = AF_INET;                              // available in lwIP as well
    server_info->sin_port = htons(server_port);                     // valid function in both platforms 
    server_info->sin_addr.s_addr = INADDR_ANY;                      // available in lwIP as well (value 0x00000000)
}

void server_socket_bind(const platform_socket_fd_t sockfd, platform_sockaddr_in_t* server_info, platform_socklen_t* size_of_server_info){
    
    platform_print("binding on port %d, fd %d, size %d\n", 
    ntohs(server_info->sin_port), sockfd, *size_of_server_info);    // valid function in both platforms

    if (platform_socket_bind(sockfd, (const platform_sockaddr_t *)server_info, *size_of_server_info) < 0)
    {
        platform_print("bind failed");
        // exit(EXIT_FAILURE);
        platform_panic(-1);
    }else{
        platform_print("server - bind successfully \n");
    }
}

platform_ssize_t server_socket_wait_packets(const platform_socket_fd_t sockfd, char* buffer, const int buffer_size, platform_sockaddr_t* client_info, platform_socklen_t* size_of_client_info){
    platform_ssize_t n = platform_socket_recvfrom(sockfd, buffer, buffer_size-1, client_info, size_of_client_info);
    if (n > 0 && n < buffer_size) {
        buffer[n] = '\0';
    }
    return n;
}

void server_socket_send_message(const platform_socket_fd_t sockfd, const char* msg, const size_t msg_size, const platform_sockaddr_t* client_info, platform_socklen_t* size_of_client_info){
    platform_ssize_t n = platform_socket_sendto(sockfd, msg, msg_size, client_info, *size_of_client_info);
    if(n<0){
        platform_print("server - sendto function error, exit code %ld\n", n);
        // exit((int)n);
        platform_panic(-1);
    }else{
        platform_print("server - message sent\n");
    }
}


int server_udp_channel_test()
{
    platform_sockaddr_in_t server_info, client_info;
    
    int sockfd = server_socket_init();
    server_socket_set_server_info(&server_info, SERVER_PORT);

    platform_socklen_t size_of_server_info = (platform_socklen_t)sizeof(server_info);
    server_socket_bind(sockfd, &server_info, &size_of_server_info);
    
    char buffer[BUFFER_SIZE];
    platform_socklen_t size_of_client_info = (platform_socklen_t)sizeof(client_info);
    platform_ssize_t recv_bytes = server_socket_wait_packets(sockfd, buffer, BUFFER_SIZE, (platform_sockaddr_t *)&client_info, &size_of_client_info);
    platform_print("server - Received message(%ld): '%s' from client\n", recv_bytes, buffer);
    
    const char *msg = "Hello from server";
    server_socket_send_message(sockfd, msg,  (size_t)strlen(msg), (const platform_sockaddr_t *)&client_info, &size_of_client_info);
    platform_print("server - replied to client correctly\n");

    return 0;
}


// initialize the server with basic info (socket type, port to listen) and bind it. Initialize the buffer semaphore as well.
// return (udp_server_data_t*) reference to server data structure that contains the msg buffer too
udp_server_data_t* server_init(){
    udp_server_data.socket_fd = server_socket_init();
    memset(&(udp_server_data.server_info), 0, sizeof(udp_server_data.server_info));                 // memset present in both platforms
    memset(&(udp_server_data.client_info), 0, sizeof(udp_server_data.client_info));

    server_socket_set_server_info((platform_sockaddr_in_t*)&udp_server_data.server_info, SERVER_PORT);

    udp_server_data.size_of_server_info = (platform_socklen_t)sizeof(udp_server_data.server_info);
    server_socket_bind(udp_server_data.socket_fd, (platform_sockaddr_in_t*)&udp_server_data.server_info, &udp_server_data.size_of_server_info);

    platform_sem_init_mutex(&udp_server_data.udp_server_buffer.buffer_messages_mutex, 1, 1);
    platform_sem_init_mutex(&udp_server_data.udp_server_buffer.buffer_messages_counting_sem, 0, 255);
    udp_server_data.udp_server_buffer.buffer_max_size = BUFFER_QUEUE_SIZE;
    udp_server_data.udp_server_buffer.buffer_max_line_size = BUFFER_SIZE;
    return &udp_server_data;
}

int server_buffer_push(char* data,  int size_of_data){
    udp_server_buffer_t* buffer_data = &udp_server_data.udp_server_buffer;
    platform_sem_wait(&(buffer_data->buffer_messages_mutex));

    int head = buffer_data->buffer_messages_head;
    int tail = buffer_data->buffer_messages_tail;
    int max_size = buffer_data->buffer_max_size;
    int max_line_size = buffer_data->buffer_max_line_size;
    int next = (head + 1 == max_size) ? 0 : head + 1;
    
    if(buffer_data->buffer_is_full){
        platform_sem_post(&buffer_data->buffer_messages_mutex);
        return -1; // drop the message, buffer full
    } 
    
    int saturation_size_of_data = (size_of_data > max_line_size)? max_line_size:size_of_data;
    // platform_print("push: data='%s' size=%d max_line=%d saturated=%d\n", data, size_of_data, max_line_size, saturation_size_of_data);
    memset(buffer_data->buffer_messages[head], 0, max_line_size);
    strncpy((buffer_data->buffer_messages[head]), data, saturation_size_of_data);                   // strncpy present in both as well
    buffer_data->buffer_messages[head][saturation_size_of_data] = '\0';

    if(next == tail){
        buffer_data->buffer_is_full = 1;
    }
    buffer_data->buffer_messages_head = next;

    platform_sem_post(&buffer_data->buffer_messages_counting_sem);
    platform_sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

int server_buffer_pop(char* dest_data){
    udp_server_buffer_t* buffer_data = &udp_server_data.udp_server_buffer;
    platform_sem_wait(&buffer_data->buffer_messages_counting_sem);
    platform_sem_wait(&buffer_data->buffer_messages_mutex);
    
    int head = buffer_data->buffer_messages_head;
    int tail = buffer_data->buffer_messages_tail;
    int max_size = buffer_data->buffer_max_size;
    int max_line_size = buffer_data->buffer_max_line_size;
    int next = ((tail + 1) == max_size) ? 0 : tail + 1;

    char* src_data = buffer_data->buffer_messages[tail];
    int size_of_src_data = strlen(src_data);
    memcpy(dest_data, src_data, size_of_src_data);
    dest_data[size_of_src_data] = '\0';
    
    buffer_data->buffer_messages_tail = next;
    buffer_data->buffer_is_full = 0;

    platform_sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

void server_listen_port(){
    platform_flag_t buffer_is_full = 0;
    char single_message_buffer[BUFFER_SIZE];
    platform_ssize_t recv_bytes = 0;
    char should_exit = 0;

    udp_server_data.size_of_client_info = (platform_socklen_t)sizeof(udp_server_data.client_info);           // shared but shouldn't be used (C trust)
    do{
        platform_sem_wait(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
        buffer_is_full = udp_server_data.udp_server_buffer.buffer_is_full;
        platform_sem_post(&udp_server_data.udp_server_buffer.buffer_messages_mutex);

        while((!buffer_is_full) && (!udp_server_data.stop_server)){        // get new messages up to max capacity (no message priority)
            platform_print("waiting for packet...\n");
            recv_bytes = server_socket_wait_packets(udp_server_data.socket_fd, single_message_buffer, BUFFER_SIZE, (platform_sockaddr_t* )&udp_server_data.client_info, &udp_server_data.size_of_client_info);
            platform_print("recvfrom returned: %d\n", recv_bytes);
            if(recv_bytes>0){
                int push_ret = server_buffer_push(single_message_buffer, recv_bytes);
                if (push_ret == -1){
                    platform_print("Server buffer is full. New messages will not be received! \n");
                }
            }else{
                continue;
            }
        }
        if(udp_server_data.stop_server){                            // destroy the semaphore and the socket 
            platform_sem_destroy(&udp_server_data.udp_server_buffer.buffer_messages_mutex);
            platform_sem_destroy(&udp_server_data.udp_server_buffer.buffer_messages_counting_sem);
            platform_socket_close(udp_server_data.socket_fd);
            should_exit = 1;
        }
        platform_sleep_ms(3000);                                    // wait to avoid keeping busy the semaphore 
    }while(!should_exit);
}