#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

Queue* initQueue(int maxSize) {
    Queue *queue = malloc(sizeof(Queue));
    queue->head = NULL;
    queue->tail = NULL;
    queue->maxSize = maxSize;
    queue->currentSize = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
    return queue;
}
void enqueue(Queue *queue, void *data) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->currentSize >= queue->maxSize) {
        printf("Queue is full. Dropping packet.\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }
    Node *node = malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    if (queue->tail == NULL) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }
    queue->currentSize++;
//    printf("Enqueued packet. Queue size: %d\n", queue->currentSize); // Debug log
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

void* dequeue(Queue *queue) {
    pthread_mutex_lock(&queue->mutex);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
    Node *temp = queue->head;
    void *data = temp->data;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->currentSize--;
//    printf("Dequeued packet. Queue size: %d\n", queue->currentSize); // Debug log
    free(temp);
    pthread_mutex_unlock(&queue->mutex);
    return data;
}
