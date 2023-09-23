#ifndef LOGGER_H
#define LOGGER_H

extern int UDP_PORT;
extern char LISTEN_UDP_IP[16];
extern int DEST_UDP_PORT;
extern char DEST_UDP_IP[16];
extern int MAX_MESSAGE_SIZE;
extern int BUFFER_SIZE;
extern int MAX_THREADS;
extern int MAX_QUEUE_SIZE;
extern int LOGGING_INTERVAL;
extern int LOGGING_ENABLED;

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void write_log(const char *format, ...);

#endif // LOGGER_H
