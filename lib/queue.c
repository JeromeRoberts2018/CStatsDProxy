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
/**
 * Adds an element to the end of a thread-safe queue.
 * The function locks a mutex to ensure thread-safety while modifying the queue.
 *
 * If the queue is full (i.e., the current size is equal to or greater than the max size),
 * a message "Queue is full. Dropping packet." is printed to stdout, and the function returns without adding the data.
 *
 * Otherwise, a new Node is allocated and its data field is set to the incoming data.
 * The Node is then added to the end of the queue, and the current size of the queue is incremented by 1.
 * If the queue was empty, both the head and the tail pointers are set to this new node.
 * Otherwise, the new node is set as the next node after the current tail and becomes the new tail.
 *
 * After successfully adding a new node, a conditional variable is signaled to indicate that the queue is not empty.
 *
 * @param queue A pointer to the Queue structure.
 * @param data A pointer to the data to be enqueued.
 */
void enqueue(Queue *queue, void *data) {
    pthread_mutex_lock(&queue->mutex);
    if (queue->currentSize >= queue->maxSize) {
        printf("Queue is full. Dropping packet. %d\n", queue->maxSize);
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
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

/**
 * Removes and returns an element from the front of a thread-safe queue.
 * The function locks a mutex to ensure thread-safety while modifying the queue.
 *
 * If the queue is empty, the function blocks and waits for a signal from a conditional variable.
 * The signal is expected to be sent by another thread that enqueues data into the queue.
 *
 * Once the queue is not empty, the function removes the Node at the head of the queue,
 * decrements the current size by 1, and returns the data contained in the removed Node.
 *
 * If the queue becomes empty after the dequeue operation, both the head and tail pointers are set to NULL.
 *
 * @param queue A pointer to the Queue structure.
 * @return A pointer to the dequeued data. The data needs to be cast to the appropriate type by the caller.
 */
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
    free(temp);
    pthread_mutex_unlock(&queue->mutex);
    return data;
}
