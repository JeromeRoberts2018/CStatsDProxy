#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

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

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct Queue {
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int maxSize;
    int currentSize;
} Queue;

Queue* initQueue(int maxSize);
void enqueue(Queue *queue, void *data);
void* dequeue(Queue *queue);

#endif