#include <errno.h>
#include "logger.h"
#include <pthread.h>
#include <string.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Enum to represent the logging levels
typedef enum {
    LEVEL_INFO,
    LEVEL_ERROR,
    LEVEL_DEBUG
} LogLevel;

// Function to convert a string level to enum
LogLevel getLogLevel(const char *str) {
    if (strcmp(str, "INFO") == 0) return LEVEL_INFO;
    if (strcmp(str, "ERROR") == 0) return LEVEL_ERROR;
    if (strcmp(str, "DEBUG") == 0) return LEVEL_DEBUG;
    return LEVEL_INFO; // Default
}

void write_log(const char *level, const char *format, ...) {
    // Get the configured log level from the settings
    LogLevel configuredLevel = getLogLevel(LOGGING_LEVEL);
    LogLevel currentLevel = getLogLevel(level);

    // Check if the current log message should be written based on the configured level
    if (currentLevel > configuredLevel) {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    FILE *file = fopen(LOGGING_FILE_NAME, "a"); // Append to the file
    if (file == NULL) {
        perror("Error opening file");  // Print the error
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, level);

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