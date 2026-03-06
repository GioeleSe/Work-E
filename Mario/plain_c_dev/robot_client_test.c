#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     8181
#define MAX_MSG_SIZE    256
#define MIN_DELAY_MS    200     // minimum delay between messages in milliseconds
#define MAX_DELAY_MS    2000    // maximum delay between messages in milliseconds

// pool of random message templates to send
static const char* message_pool[] = {
    "hello from client",
    "ping",
    "status: ok",
    "temperature: %d",
    "sensor_reading: %d",
    "heartbeat",
    "data packet %d",
    "alert: value exceeded %d",
    "request: update",
    "info: client running",
};
static const int message_pool_size = sizeof(message_pool) / sizeof(message_pool[0]);

int client_socket_init(){
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("client - socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("client - socket created\n");
    return sockfd;
}

void client_socket_set_server_info(struct sockaddr_in* server_info, const char* server_ip, const int server_port){
    memset(server_info, 0, sizeof(struct sockaddr_in));
    server_info->sin_family = AF_INET;
    server_info->sin_port   = htons(server_port);
    if(inet_pton(AF_INET, server_ip, &server_info->sin_addr) <= 0){
        perror("client - invalid server IP");
        exit(EXIT_FAILURE);
    }
}

void sleep_ms(int milliseconds){
    struct timespec ts;
    ts.tv_sec  = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int main(){
    srand(time(NULL));

    int sockfd = client_socket_init();

    struct sockaddr_in server_info;
    client_socket_set_server_info(&server_info, SERVER_IP, SERVER_PORT);
    socklen_t server_info_size = sizeof(server_info);

    printf("client - sending to %s:%d\n\n", SERVER_IP, SERVER_PORT);

    int msg_count = 0;
    char message[MAX_MSG_SIZE];
    while(1){
        // pick a random message template
        int template_idx = rand() % message_pool_size;
        memset(message, 0, MAX_MSG_SIZE);
        snprintf(message, MAX_MSG_SIZE, message_pool[template_idx], rand() % 100);

        ssize_t sent = sendto(sockfd,
                              message,
                              strlen(message) + 1,     // +1 to include null terminator
                              0,
                              (struct sockaddr*)&server_info,
                              server_info_size);

        if(sent < 0){
            perror("client - sendto failed");
        } else {
            printf("[msg #%d] sent: \"%s\" (%zd bytes)\n", ++msg_count, message, sent);
        }

        // sleep for a random delay between MIN and MAX
        int delay = MIN_DELAY_MS + rand() % (MAX_DELAY_MS - MIN_DELAY_MS);
        printf("         next message in %d ms\n", delay);
        sleep_ms(delay);
    }

    close(sockfd);
    return 0;
}