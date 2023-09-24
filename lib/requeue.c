#include "requeue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "global.h"

Queue *requeue = NULL;  // Global requeue

struct RequeueArgs {
    Queue **queues;
    int max_threads;
};

void *requeue_thread(void *arg) {
    set_thread_name("requeue");

    struct RequeueArgs *requeueArgs = (struct RequeueArgs *)arg;
    Queue **queues = requeueArgs->queues;
    int max_threads = requeueArgs->max_threads;

    while (1) {
        for (int i = 0; i < max_threads; ++i) {
            char *data = dequeue(requeue);
            if (data != NULL) {
                enqueue(queues[i], data);
            }
        }
        sleep(1);  // Wait for 1 second before checking again
    }
    return NULL;
}

int init_requeue_thread(pthread_t *thread, int max_threads, Queue **worker_queues) {
    requeue = initQueue(10000); // Initialize with a size of 10,000
    struct RequeueArgs *args = malloc(sizeof(struct RequeueArgs));
    args->queues = worker_queues;
    args->max_threads = max_threads;

    if (pthread_create(thread, NULL, requeue_thread, args) != 0) {
        perror("Could not create requeue thread");
        return -1;
    }
    return 0;
}
