#include <ArduinoJson.h>                                            // direclty installed as local self-contained lib
#include "udp_server.h"

platform_socket_fd_t server_socket_init()
{
    platform_socket_fd_t sockfd = platform_socket_create_udp();
    if (sockfd < 0)
    {
        platform_print("server - socket creation failed");
        platform_panic(-1);
    }
    else
    {
        platform_print("server - socket created\n");
    }
    return sockfd;
}

void server_socket_set_server_info(platform_sockaddr_in_t *server_info, const int server_port)
{
    server_info->sin_family      = AF_INET;
    server_info->sin_port        = htons(server_port);
    server_info->sin_addr.s_addr = INADDR_ANY;
}

void server_socket_bind(const platform_socket_fd_t sockfd, platform_sockaddr_in_t *server_info, platform_socklen_t *size_of_server_info)
{
    platform_print("binding on port %d, fd %d, size %d\n",
                   ntohs(server_info->sin_port), sockfd, *size_of_server_info);

    if (platform_socket_bind(sockfd, server_info, *size_of_server_info) < 0)  // fixed: removed cast, signature matches
    {
        platform_print("bind failed");
        platform_panic(-1);
    }
    else
    {
        platform_print("server - bind successfully\n");
    }
}

platform_ssize_t server_socket_wait_packets(const platform_socket_fd_t sockfd, char *buffer, const int buffer_size, platform_sockaddr_t *client_info, platform_socklen_t *size_of_client_info)
{
    // fixed: removed extra 0 flags argument — handled inside platform_socket_recvfrom
    platform_ssize_t n = platform_socket_recvfrom(sockfd, buffer, buffer_size - 1, client_info, size_of_client_info);
    if (n > 0 && n < buffer_size)
        buffer[n] = '\0';
    return n;
}

void server_socket_send_message(const platform_socket_fd_t sockfd, const char *msg, const size_t msg_size, const platform_sockaddr_t *client_info, platform_socklen_t *size_of_client_info)
{
    // fixed: removed extra 0 flags argument — handled inside platform_socket_sendto
    platform_ssize_t n = platform_socket_sendto(sockfd, msg, msg_size, client_info, *size_of_client_info);
    if (n < 0)
    {
        platform_print("server - sendto function error, exit code %ld\n", n);
        platform_panic(-1);
    }
    else
    {
        platform_print("server - message sent\n");
    }
}
udp_server_data_t* udp_server_data = NULL;
udp_server_data_t *server_init()
{
    udp_server_data = (udp_server_data_t*)malloc(sizeof(udp_server_data_t));
    if (udp_server_data == NULL) {
        platform_panic(-1);
    }
    memset(udp_server_data, 0, sizeof(udp_server_data_t));    

    // platform_sem_init_mutex(&udp_server_data->udp_server_buffer.buffer_messages_counting_sem,   0, 0);
    // platform_sem_init_mutex(&udp_server_data->udp_server_buffer.buffer_messages_mutex,          0, 1);
    if (platform_sem_init_mutex(&udp_server_data->udp_server_buffer.buffer_messages_counting_sem, 0, QUEUE_CAPACITY) < 0) {
        platform_print("Counting semaphore allocation failed!\n");
        platform_panic(-1);
    }
    if (platform_sem_init_mutex(&udp_server_data->udp_server_buffer.buffer_messages_mutex, 1, 1) < 0) {
        platform_print("Mutex semaphore allocation failed!\n");
        platform_panic(-1);
    }
    udp_server_data->socket_fd = platform_socket_create_udp();
    if (udp_server_data->socket_fd < 0) {
        platform_print("Socket creation failed\n");
        platform_panic(-1);
    }
    platform_print("server - socket created\n");

    server_socket_set_server_info(&udp_server_data->server_info, SERVER_PORT);
    udp_server_data->size_of_server_info = (platform_socklen_t)sizeof(udp_server_data->server_info);
    udp_server_data->udp_server_buffer.buffer_max_size      = BUFFER_SIZE;
    udp_server_data->udp_server_buffer.buffer_max_line_size = BUFFER_SIZE;
    udp_server_data->stop_server                            = 0;
    
    server_socket_bind(udp_server_data->socket_fd, &udp_server_data->server_info, &udp_server_data->size_of_server_info);
    
    return udp_server_data;
}

int server_buffer_push(char *data, int size_of_data)
{
    udp_server_buffer_t *buffer_data = &udp_server_data->udp_server_buffer;
    platform_sem_wait(&buffer_data->buffer_messages_mutex);

    if (buffer_data->buffer_is_full)
    {
        platform_sem_post(&buffer_data->buffer_messages_mutex);
        return -1;
    }

    int head             = buffer_data->buffer_messages_head;
    int max_size         = buffer_data->buffer_max_size;
    int max_line_size    = buffer_data->buffer_max_line_size;
    int next             = (head + 1 == max_size) ? 0 : head + 1;
    int saturated_size   = (size_of_data > max_line_size) ? max_line_size : size_of_data;

    memset(buffer_data->buffer_messages[head], 0, max_line_size);
    strncpy(buffer_data->buffer_messages[head], data, saturated_size);
    buffer_data->buffer_messages[head][saturated_size] = '\0';

    buffer_data->buffer_messages_head = next;
    if (next == buffer_data->buffer_messages_tail)
        buffer_data->buffer_is_full = 1;

    platform_sem_post(&buffer_data->buffer_messages_counting_sem);
    platform_sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

int server_buffer_pop(char *dest_data)
{
    udp_server_buffer_t *buffer_data = &udp_server_data->udp_server_buffer;
    platform_sem_wait(&buffer_data->buffer_messages_counting_sem);
    platform_sem_wait(&buffer_data->buffer_messages_mutex);

    int tail          = buffer_data->buffer_messages_tail;
    int max_size      = buffer_data->buffer_max_size;
    int next          = (tail + 1 == max_size) ? 0 : tail + 1;

    char *src         = buffer_data->buffer_messages[tail];
    int   src_size    = strlen(src);
    memcpy(dest_data, src, src_size);
    dest_data[src_size] = '\0';

    buffer_data->buffer_messages_tail = next;
    buffer_data->buffer_is_full       = 0;

    platform_sem_post(&buffer_data->buffer_messages_mutex);
    return 0;
}

void server_listen_port()
{
    platform_flag_t buffer_is_full = 0;
    char            single_message_buffer[BUFFER_SIZE];
    platform_ssize_t recv_bytes = 0;
    char            should_exit = 0;

    // Initialize client address structure size mapping
    udp_server_data->size_of_client_info = (platform_socklen_t)sizeof(udp_server_data->client_info);
    
    platform_print("udp server - Starting listening loop...\n");

    do
    {
        // Thread-safe status check on the internal message queue
        platform_sem_wait(&udp_server_data->udp_server_buffer.buffer_messages_mutex);
        buffer_is_full = udp_server_data->udp_server_buffer.buffer_is_full;
        platform_sem_post(&udp_server_data->udp_server_buffer.buffer_messages_mutex);

        // Core processing cycle: Active while space remains and stop hasn't been requested
        while (!buffer_is_full && !udp_server_data->stop_server)
        {
            // Blocking call to capture incoming raw network payloads
            recv_bytes = server_socket_wait_packets(
                udp_server_data->socket_fd,
                single_message_buffer,
                BUFFER_SIZE,
                (platform_sockaddr_t *)&udp_server_data->client_info,
                &udp_server_data->size_of_client_info);

            if (recv_bytes > 0)
            {
                platform_print("udp server - Received packet. Size: %d bytes\n", (int)recv_bytes);
                
                // Transfer network payload into the main processing FIFO queue
                if (server_buffer_push(single_message_buffer, recv_bytes) == -1)
                {
                    platform_print("udp server - CRITICAL: Server buffer full — message dropped\n");
                }
            }
            else
            {
                platform_print("udp server - WARNING: Socket read error or empty payload received.\n");
            }

            // Re-verify buffer boundary conditions for the next processing frame
            platform_sem_wait(&udp_server_data->udp_server_buffer.buffer_messages_mutex);
            buffer_is_full = udp_server_data->udp_server_buffer.buffer_is_full;
            platform_sem_post(&udp_server_data->udp_server_buffer.buffer_messages_mutex);
        }

        // Clean teardown cascade triggered by system shutdown flag
        if (udp_server_data->stop_server)
        {
            platform_print("udp server - Shutdown flag caught. Cleaning up platform resources...\n");
            
            platform_sem_destroy(&udp_server_data->udp_server_buffer.buffer_messages_mutex);
            platform_sem_destroy(&udp_server_data->udp_server_buffer.buffer_messages_counting_sem);
            platform_socket_close(udp_server_data->socket_fd);
            
            should_exit = 1;
            platform_print("udp server - Socket closed and semaphores destroyed. Exiting listener loop safely.\n");
        }
        else if (buffer_is_full)
        {
            // Backoff period if the loop broke exclusively due to a saturated buffer queue
            platform_print("udp server - Queue saturated. Throttling active read loop for 3000ms...\n");
            platform_sleep_ms(3000);
        }

    } while (!should_exit);
}