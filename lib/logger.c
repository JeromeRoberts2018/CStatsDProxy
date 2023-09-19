#include "logger.h"

void write_log(const char *filename, const char *format, ...) {
    FILE *file = fopen(filename, "a"); // Append to the file
    if (file == NULL) {
        return;
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fprintf(file, "\n");

    fclose(file);
}
