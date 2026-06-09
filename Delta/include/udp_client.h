#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H
#include "common_platform_abstr.h"

#define SERVER_ADDRESS "192.168.137.1"  // PC hotspot gateway
#define SERVER_PORT 8000
#define RETRY_SEND_MESSAGE 1                                        // set to 0 to set it to best effort mode (max attempts will be ignored)
#define RETRY_SEND_MESSAGE_MAX_ATTEMPTS 3

#ifdef __cplusplus
extern "C" {
#endif

int client_main_test();
int client_send_packet(char* msg, platform_ssize_t msg_size);

#ifdef __cplusplus
}
#endif

#endif