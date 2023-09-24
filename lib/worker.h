#ifndef WORKER_H
#define WORKER_H

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
extern char LOGGING_FILE_NAME[256];
extern int CLONE_ENABLED;
extern int CLONE_DEST_UDP_PORT;
extern char CLONE_DEST_UDP_IP[50];
extern Queue *requeue;
extern Queue **queues;

typedef struct {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int bufferSize;
} WorkerArgs;
extern int BUFFER_SIZE;

void *worker_thread(void *arg);

#endif // WORKER_H