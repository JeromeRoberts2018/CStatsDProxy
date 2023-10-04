#ifndef CONFIG_READER_H
#define CONFIG_READER_H

int read_config(const char *filepath);
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
extern int CLONE_ENABLED;
extern int CLONE_DEST_UDP_PORT;
extern char CLONE_DEST_UDP_IP[50];
extern int HTTP_ENABLED;
extern int HTTP_PORT;
extern char HTTP_LISTEN_IP[16];
extern int OUTBOUND_UDP_TIMEOUT;


#endif // CONFIG_READER_H
