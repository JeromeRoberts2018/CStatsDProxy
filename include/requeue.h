#ifndef REQUEUE_H
#define REQUEUE_H

#include <pthread.h>
#include "queue.h"

// Initialize the requeue thread
int init_requeue_thread(pthread_t *thread, int max_threads, Queue **worker_queues);

#endif // REQUEUE_H
