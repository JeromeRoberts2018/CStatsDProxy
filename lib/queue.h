#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

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