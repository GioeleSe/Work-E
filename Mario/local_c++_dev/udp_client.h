#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H
#include <sys/types.h>

#define SERVER_ADDRESS "127.0.0.1\0"
#define SERVER_PORT 8181
#define BUFFER_SIZE 1024
#define RETRY_SEND_MESSAGE 1                                        // set to 0 to set it to best effort mode (max attempts will be ignored)
#define RETRY_SEND_MESSAGE_MAX_ATTEMPTS 3

int client_main_test();
int client_send_packet(char* msg, ssize_t msg_size);

#endif