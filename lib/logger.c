#include <errno.h>
#include <stdarg.h>  // for va_list and related functions
#include <stdio.h>
#include <pthread.h>
#include "logger.h"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *format, ...) {
    if (LOGGING_ENABLED == 0) {
        return;
    }
    pthread_mutex_lock(&log_mutex);

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
