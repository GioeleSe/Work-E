#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H
#include "common_platform_abstr.h"

#define SERVER_ADDRESS "127.0.0.1\0"                                // TODO: check laptop address in hostspot interface  
#define SERVER_PORT 8000
#define BUFFER_SIZE 256
#define RETRY_SEND_MESSAGE 1                                        // set to 0 to set it to best effort mode (max attempts will be ignored)
#define RETRY_SEND_MESSAGE_MAX_ATTEMPTS 3

int client_main_test();
int client_send_packet(char* msg, ssize_t msg_size);                // direct packet sending function
// TODO: add feedback and debug messages sending wrapper functions

#endif