#include <errno.h>
#include "logger.h"
#include <pthread.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Writes a log entry to a given file.
 * The function locks a mutex to ensure that only one thread writes to the file at a given time.
 *
 * @param filename The name of the file to write the log entry to.
 * @param format A format string for the log entry.
 * @param ... Variable arguments to include in the log.
 */
void write_log(const char *filename, const char *format, ...) {
    pthread_mutex_lock(&log_mutex);

    FILE *file = fopen(filename, "a"); // Append to the file
    if (file == NULL) {
        perror("Error opening file");  // Print the error
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    va_list args;
    va_start(args, format);
    if (vfprintf(file, format, args) < 0) {
        perror("Error writing to file");
    }
    va_end(args);

    fprintf(file, "\n");
    fflush(file);  // Flush the output buffer
    fclose(file);

    pthread_mutex_unlock(&log_mutex);
}
