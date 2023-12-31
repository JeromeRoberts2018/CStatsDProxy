#ifndef WORKER_H
#define WORKER_H


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