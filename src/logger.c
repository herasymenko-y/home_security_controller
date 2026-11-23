#include "logger.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mut = PTHREAD_MUTEX_INITIALIZER;

int logger_init(const char *filepath) {
    if (filepath == NULL) {
        return -1;
    }

    pthread_mutex_lock(&log_mut);
    log_file = fopen(filepath, "a");
    if (log_file == NULL) {
        perror("Logger fopen error");
        pthread_mutex_unlock(&log_mut);
        return -1;
    }
    pthread_mutex_unlock(&log_mut);
    return 0;
}

int log_event(const char *state_str, const char *message) {
    if (state_str == NULL || message == NULL) {
        return -1;
    }

    pthread_mutex_lock(&log_mut);
    if (log_file == NULL) {
        pthread_mutex_unlock(&log_mut);
        return -1;
    }

    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    int res = fprintf(log_file, "[%s] [%s] %s\n", time_buf, state_str, message);
    int flush_res = fflush(log_file);
    pthread_mutex_unlock(&log_mut);

    if (res < 0 || flush_res != 0) {
        return -1;
    }
    return 0;
}

int logger_close(void) {
    int ret = 0;

    pthread_mutex_lock(&log_mut);
    if (log_file != NULL) {
        if (fclose(log_file) != 0) {
            ret = -1;
        }
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mut);

    return ret;
}
