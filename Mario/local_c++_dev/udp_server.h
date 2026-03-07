#ifndef UDP_SERVER
#define UDP_SERVER

#define SERVER_PORT 8181
#define BUFFER_SIZE 1024
#include <semaphore.h>                                              // needed for sem_t
#include <sys/socket.h>                                             // needed for socklen_t
#include <netinet/in.h>


typedef struct udp_server_buffer_t {
    sem_t buffer_messages_counting_sem;                             // use this one to wait for new messages
    sem_t buffer_messages_mutex;                                    // use this semaphore to access the buffer and its descriptors
    char buffer_messages[BUFFER_SIZE][BUFFER_SIZE];                 // byte buffer,
    int buffer_max_size;                                            // row max count, set initially then constant to BUFFER_SIZE or some multiple of it
    int buffer_max_line_size;                                       // column max count, set initially then constant to BUFFER_SIZE
    int buffer_messages_head;
    int buffer_messages_tail;
    char buffer_is_full;
} udp_server_buffer_t;

typedef struct udp_server_data_t {
    int socket_fd;
    struct sockaddr_in server_info;
    socklen_t size_of_server_info;
    struct sockaddr_in client_info;
    socklen_t size_of_client_info;
    udp_server_buffer_t udp_server_buffer;
    char stop_server;
} udp_server_data_t;

extern udp_server_data_t udp_server_data;                           // shared global variable (C trust)

// initial function to test udp sockets, might not work now
// int server_udp_channel_test();


// return 0 for successful push
// return -1  for full buffer (couldn't push)
int server_buffer_push(char* data,  int size_of_data);

// return 0 for successful pop
// return -1  for empty buffer (nothing to pop)
int server_buffer_pop(char* dest_data);

// Start the server and bind it to the port defined in this file
// return the handler of the server data struct.
// Note that inside it there's the struct udp_server_buffer that contains a semaphore and a matrix (char) buffer
udp_server_data_t* server_init();

// start the actual server, will manage inside the buffer.
// to get new messages check for line
void server_listen_port();



#endif