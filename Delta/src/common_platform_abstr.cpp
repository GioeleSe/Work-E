#include "common_platform_abstr.h"

static int platform_sem_init_mutex(platform_sem_t *sem, int initial_val, int max_val)
{
#if defined(PLATFORM_ESP32)
    *sem = xSemaphoreCreateCounting(max_val, initial_val);
    return (*sem == NULL) ? -1 : 0;
#elif defined(PLATFORM_LINUX)
    return sem_init(sem, 0, initial_val);
#endif
}

static int platform_sem_wait(platform_sem_t *sem)
{
#if defined(PLATFORM_ESP32)
    return (xSemaphoreTake(*sem, portMAX_DELAY) == pdTRUE) ? 0 : -1;
#elif defined(PLATFORM_LINUX)
    return sem_wait(sem);
#endif
}

static int platform_sem_wait_isr(platform_sem_t *sem)
{
#if defined(PLATFORM_ESP32) // semaphores with ISR must be treated in a special way
    BaseType_t woken = pdFALSE;
    BaseType_t ret = xSemaphoreTakeFromISR(*sem, &woken);
    portYIELD_FROM_ISR(woken);
    return (ret == pdTRUE) ? 0 : -1;
#else
    return 0;
#endif
}

static int platform_sem_post(platform_sem_t *sem)
{
#if defined(PLATFORM_ESP32)
    return (xSemaphoreGive(*sem) == pdTRUE) ? 0 : -1;
#elif defined(PLATFORM_LINUX)
    return sem_post(sem);
#endif
}

static void platform_sem_destroy(platform_sem_t *sem)
{
#if defined(PLATFORM_ESP32)
    vSemaphoreDelete(*sem);
    *sem = NULL;
#elif defined(PLATFORM_LINUX)
    sem_destroy(sem);
#endif
}

static platform_socket_fd_t platform_socket_create_udp()            // no real implementation distinction but more coherent interface
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

static int platform_socket_bind(platform_socket_fd_t sockfd, const platform_sockaddr_in_t *addr, platform_socklen_t addrlen)
{
    return bind(sockfd, (const platform_sockaddr_t *)addr, addrlen);
}

static platform_ssize_t platform_socket_recvfrom(platform_socket_fd_t sockfd, void *buf, platform_ssize_t len, platform_sockaddr_t *src_addr, platform_socklen_t *addrlen)
{
    return recvfrom(sockfd, buf, len, 0, src_addr, addrlen);
}

static platform_ssize_t platform_socket_sendto(platform_socket_fd_t sockfd, const void *buf, platform_ssize_t len, const platform_sockaddr_t *dest_addr, platform_socklen_t addrlen)
{
    return sendto(sockfd, buf, len, 0, dest_addr, addrlen);
}

static int platform_socket_close(platform_socket_fd_t fd)
{
    return close(fd);
}

static void platform_thread_create(platform_thread_t *thread, void *(*func)(void *), void *arg, const char *name){
#ifdef PLATFORM_LINUX
    (void)name;
    return pthread_create(thread, NULL, func, arg);
#elif defined(PLATFORM_ESP32)
    _platform_task_ctx_t *ctx = (_platform_task_ctx_t *)malloc(sizeof(_platform_task_ctx_t));
    if (!ctx){
        return -1;
    }
    ctx->func = func;
    ctx->arg  = arg;
    BaseType_t rc = xTaskCreate(_platform_task_wrapper, name ? name : "task", PLATFORM_THREAD_STACK_SIZE / sizeof(StackType_t), ctx, PLATFORM_THREAD_PRIORITY, thread);
    if (rc != pdPASS) {
        free(ctx);
        return -1;
    }
    return 0;
#endif
}
static void platform_thread_join(platform_thread_t thread){
#ifdef PLATFORM_LINUX
    return pthread_join(thread, NULL);
#elif defined(PLATFORM_ESP32)
    while (eTaskGetState(thread) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    return 0;
#endif
}

static void platform_thread_exit(){
#ifdef PLATFORM_LINUX
    pthread_exit(NULL);
#elif defined(PLATFORM_ESP32)
    vTaskDelete(NULL);                                              // TODO: test the behaviour of tasks after deletion
#endif
}



static void platform_sleep_ms(int sleep_time_ms)
{
#if defined(PLATFORM_ESP32)
    vTaskDelay((sleep_time_ms / portTICK_PERIOD_MS));
#elif defined(PLATFORM_LINUX)
    struct timespec ts = {
        .tv_sec = sleep_time_ms / 1000,
        .tv_nsec = (sleep_time_ms % 1000) * 1000000L};
    nanosleep(&ts, NULL); // POSIX suggested one, kindof long but it's not the real usage
#endif
}

static void platform_panic(int exit_value)
{
#if defined(PLATFORM_ESP32)
    esp_restart();
#elif defined(PLATFORM_LINUX)
    exit(exit_value);
#endif
}

static void platform_print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#if defined(PLATFORM_ESP32)
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
#elif defined(PLATFORM_LINUX)
    vprintf(format, args);
    fflush(stdout);
#endif
    va_end(args);
}

void platform_init_time()
{
#if defined(PLATFORM_ESP32)
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
#endif
}