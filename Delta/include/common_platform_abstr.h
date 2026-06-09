#ifndef COMMON_PLAFTFORM_ABSTR_H
#define COMMON_PLAFTFORM_ABSTR_H

#define PLATFORM_THREAD_STACK_SIZE 8192
#define PLATFORM_THREAD_PRIORITY   5

#ifdef PLATFORM_LINUX
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#elif defined(PLATFORM_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/sys.h"
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
// Arduino IPAddress.h declares "extern IPAddress INADDR_NONE" as an identifier,
// but lwIP defines INADDR_NONE as a macro — undef it to prevent the conflict.
#ifdef INADDR_NONE
#undef INADDR_NONE
#endif

#else
#error "[common_platform_abstr.h] No platform defined. Compile with -DPLATFORM_LINUX or -DPLATFORM_ESP32"
#endif

#ifdef PLATFORM_LINUX
typedef sem_t            platform_sem_t;
typedef ssize_t          platform_ssize_t;
typedef pthread_t        platform_thread_t;
typedef pthread_attr_t   platform_thread_attr_t;

#elif defined(PLATFORM_ESP32)
typedef SemaphoreHandle_t platform_sem_t;
typedef int               platform_ssize_t;
typedef TaskHandle_t      platform_thread_t;
typedef void*             platform_thread_attr_t;
#endif

typedef int              platform_socket_fd_t;
typedef struct sockaddr_in platform_sockaddr_in_t;
typedef struct sockaddr    platform_sockaddr_t;
typedef socklen_t          platform_socklen_t;
typedef volatile char      platform_flag_t;

// FreeRTOS task wrapper: bridges void*(void*) to void(void*) signature
#ifdef PLATFORM_ESP32
typedef struct { void *(*func)(void *); void *arg; } _platform_task_ctx_t;
static inline void _platform_task_wrapper(void *arg) {
    _platform_task_ctx_t *ctx = (_platform_task_ctx_t *)arg;
    ctx->func(ctx->arg);
    free(ctx);
    vTaskDelete(NULL);
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Semaphore abstraction
int  platform_sem_init(platform_sem_t *sem, int initial_val, int max_val);
int  platform_sem_wait(platform_sem_t *sem);
int  platform_sem_wait_isr(platform_sem_t *sem);
int  platform_sem_post(platform_sem_t *sem);
void platform_sem_destroy(platform_sem_t *sem);

// UDP socket abstraction
platform_socket_fd_t platform_socket_create_udp();
int              platform_socket_bind(platform_socket_fd_t sockfd, const platform_sockaddr_t *addr, platform_socklen_t addrlen);
platform_ssize_t platform_socket_recvfrom(platform_socket_fd_t sockfd, void *buf, platform_ssize_t len, int flags, platform_sockaddr_t *src_addr, platform_socklen_t *addrlen);
platform_ssize_t platform_socket_sendto(platform_socket_fd_t sockfd, const void *buf, platform_ssize_t len, int flags, const platform_sockaddr_t *dest_addr, platform_socklen_t addrlen);
int              platform_socket_close(platform_socket_fd_t fd);

// Thread abstraction
int  platform_thread_create(platform_thread_t *thread, void *(*func)(void *), void *arg, const char *name);
int  platform_thread_join(platform_thread_t thread);
void platform_thread_exit();

// General
void platform_sleep_ms(int sleep_time_ms);
void platform_panic(int exit_value);
void platform_print(const char *format, ...);
void platform_init_time();

#ifdef __cplusplus
}
#endif

#endif // COMMON_PLAFTFORM_ABSTR_H
