#include "atomic.h"
#include <stdlib.h>
#include <string.h>

void atmInitQueue(AtomicQueue *q) {
    atmNode *dummy = malloc(sizeof(atmNode));
    dummy->next = NULL;
    atomic_store(&q->head, dummy);
    atomic_store(&q->tail, dummy);
}

int atmEnqueue(AtomicQueue *q, const char *buffer) {
    atmNode *atmnode = malloc(sizeof(atmNode));
    if (!atmnode) {
        return 0; // Memory allocation failed
    }
    
    // Copy the buffer to Packet struct
    strncpy(atmnode->data.packet_data, buffer, sizeof(atmnode->data.packet_data) - 1);
    atmnode->data.packet_data[sizeof(atmnode->data.packet_data) - 1] = '\0'; // Null-terminate just to be safe
    atmnode->next = NULL;

    atmNode *tail, *next;

    while (1) {
        tail = atomic_load(&q->tail);
        next = atomic_load(&tail->next);

        if (tail == atomic_load(&q->tail)) {
            if (next == NULL) {
                if (atomic_compare_exchange_weak(&tail->next, &next, atmnode)) {
                    break;
                }
            } else {
                atomic_compare_exchange_weak(&q->tail, &tail, next);
            }
        }
    }
    atomic_compare_exchange_weak(&q->tail, &tail, atmnode);

    return 1; // Enqueue successful
}

int atmDequeue(AtomicQueue *q, Packet *outPacket) {
    atmNode *head, *tail, *next;
    
    while (1) {
        head = atomic_load(&q->head);
        tail = atomic_load(&q->tail);
        next = atomic_load(&head->next);

        if (head == atomic_load(&q->head)) {
            if (head == tail) {
                if (next == NULL) {
                    return 0; // Queue is empty
                }
                atomic_compare_exchange_weak(&q->tail, &tail, next);
            } else {
                if (outPacket) {
                    *outPacket = next->data;
                }
                if (atomic_compare_exchange_weak(&q->head, &head, next)) {
                    break;
                }
            }
        }
    }

    free(head);
    return 1; // Dequeue successful
}

int atmQueueSize(AtomicQueue *q) {
    int count = 0;
    atmNode *current = atomic_load(&q->head);

    while (current != NULL) {
        count++;
        current = atomic_load(&current->next);
    }

    return count;
}