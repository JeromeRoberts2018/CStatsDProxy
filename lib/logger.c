#include <errno.h>
#include "logger.h"
#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    // Get the current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf("[%04d-%02d-%02d %02d:%02d:%02d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    va_list args_log;
    va_start(args_log, format);
    if (vprintf(format, args_log) < 0) {
        perror("Error writing to stdout");
    }
    va_end(args_log);

    printf("\n");
    fflush(stdout);  // Flush the output buffer

    pthread_mutex_unlock(&log_mutex);
}

