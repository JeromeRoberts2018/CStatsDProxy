#include <errno.h>
#include <stdarg.h>  // for va_list and related functions
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *format, ...) {
    if (pthread_setname_np("Logger") != 0) {
        perror("pthread_setname_np");
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
