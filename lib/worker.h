#ifndef WORKER_H
#define WORKER_H

typedef struct {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int bufferSize;
} WorkerArgs;
extern int BUFFER_SIZE;

void *worker_thread(void *arg);

#endif // WORKER_H