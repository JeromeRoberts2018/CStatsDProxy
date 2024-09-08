#ifndef CONFIG_READER_H
#define CONFIG_READER_H

typedef struct {
    int UDP_PORT;
    char LISTEN_UDP_IP[50];
    int DEST_UDP_PORT;
    char DEST_UDP_IP[50];
    int MAX_MESSAGE_SIZE;
    int BUFFER_SIZE;
    int MAX_THREADS;
    int MAX_QUEUE_SIZE;
    int LOGGING_INTERVAL;
    int LOGGING_ENABLED;
    int CLONE_ENABLED;
    int CLONE_DEST_UDP_PORT;
    char CLONE_DEST_UDP_IP[50];
    int HTTP_ENABLED;
    int HTTP_PORT;
    char HTTP_LISTEN_IP[50];
    int OUTBOUND_UDP_TIMEOUT;
} Config;

extern Config config;

int read_config(const char *filepath);

#endif // CONFIG_READER_H
