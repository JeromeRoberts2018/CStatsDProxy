#include "unity.h"
#include "../include/requeue.h"
#include "../include/queue.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// Global variables for testing
Queue *worker_queues[2];
pthread_t requeue_thread_id;
extern Queue *requeue;

// Setup function to initialize queues
void requeue_setup(void) {
    for (int i = 0; i < 2; ++i) {
        worker_queues[i] = initQueue(10);
    }
}

// Teardown function to clean up queues
void requeue_teardown(void) {
    for (int i = 0; i < 2; ++i) {
        free(worker_queues[i]);
    }
    free(requeue);
}

void test_init_requeue_thread(void) {
    requeue_setup();
    int result = init_requeue_thread(&requeue_thread_id, 2, worker_queues);
    TEST_ASSERT_EQUAL_INT(0, result);

    usleep(100000);  // Sleep for 0.1 seconds

    // Cancel and join the thread to ensure proper cleanup
    pthread_cancel(requeue_thread_id);
    pthread_join(requeue_thread_id, NULL);

    requeue_teardown();
}

// Test if data is transferred from requeue to worker queues
void test_requeue_thread_logic(void) {
    requeue_setup();
    init_requeue_thread(&requeue_thread_id, 2, worker_queues);

    // Enqueue data into the requeue
    char *data1 = malloc(20);
    snprintf(data1, 20, "Data for Worker 1");
    enqueue(requeue, data1);

    char *data2 = malloc(20);
    snprintf(data2, 20, "Data for Worker 2");
    enqueue(requeue, data2);

    usleep(800000);

    // Verify that data is transferred to worker queues
    char *dequeued1 = (char *)dequeue(worker_queues[0]);
    char *dequeued2 = (char *)dequeue(worker_queues[1]);

    TEST_ASSERT_NOT_NULL(dequeued1);
    TEST_ASSERT_NOT_NULL(dequeued2);

    TEST_ASSERT_EQUAL_STRING("Data for Worker 1", dequeued1);
    TEST_ASSERT_EQUAL_STRING("Data for Worker 2", dequeued2);

    free(dequeued1);
    free(dequeued2);

    pthread_cancel(requeue_thread_id);
    requeue_teardown();
}
