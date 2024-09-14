#include "unity.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>

Queue *test_queue;  // Global queue for tests

// Helper function to simulate enqueue and check if it was successful
void enqueue_data(Queue *queue, const char *data) {
    enqueue(queue, (void *)data);
}

// Setup function for each test
void queue_setup(void) {
    test_queue = initQueue(3);
}

// Tear down function for each test
void queue_teardown(void) {
    free(test_queue);
}

// Test queue initialization
void test_initQueue(void) {
    queue_setup();
    TEST_ASSERT_NOT_NULL(test_queue);                 // Check that the queue is initialized
    TEST_ASSERT_EQUAL_INT(3, test_queue->maxSize);    // Check max size is set correctly
    TEST_ASSERT_EQUAL_INT(0, test_queue->currentSize); // Check that queue is empty
    TEST_ASSERT_NULL(test_queue->head);               // Check that the queue head is NULL
    TEST_ASSERT_NULL(test_queue->tail);               // Check that the queue tail is NULL
    queue_teardown();
}

// Test enqueueing data
void test_enqueue_data(void) {
    queue_setup();
    enqueue_data(test_queue, "Data1");
    enqueue_data(test_queue, "Data2");

    TEST_ASSERT_EQUAL_INT(2, test_queue->currentSize);  // Verify that two items are enqueued
    TEST_ASSERT_NOT_NULL(test_queue->head);             // Verify head is set
    TEST_ASSERT_NOT_NULL(test_queue->tail);             // Verify tail is set

    // Verify that the data in the head and tail nodes is correct
    TEST_ASSERT_EQUAL_STRING("Data1", (char *)test_queue->head->data);
    TEST_ASSERT_EQUAL_STRING("Data2", (char *)test_queue->tail->data);
    queue_teardown();
}

// Test dequeueing data
void test_dequeue_data(void) {
    queue_setup();
    // Enqueue items to the queue
    enqueue_data(test_queue, "Data1");
    enqueue_data(test_queue, "Data2");
    
    // Dequeue the first item and check the result
    char *dequeued_data1 = (char *)dequeue(test_queue);
    TEST_ASSERT_EQUAL_STRING("Data1", dequeued_data1);   // Verify dequeued data
    TEST_ASSERT_EQUAL_INT(1, test_queue->currentSize);   // Verify queue size is decremented

    // Dequeue the second item and check the result
    char *dequeued_data2 = (char *)dequeue(test_queue);
    TEST_ASSERT_EQUAL_STRING("Data2", dequeued_data2);
    TEST_ASSERT_EQUAL_INT(0, test_queue->currentSize);   // Verify queue is empty now

    // Check that head and tail are reset to NULL when the queue is empty
    TEST_ASSERT_NULL(test_queue->head);
    TEST_ASSERT_NULL(test_queue->tail);
    queue_teardown();
}

// Test enqueueing when the queue is full
void test_enqueue_full_queue(void) {
    queue_setup();
    // Fill the queue to its maximum size
    enqueue_data(test_queue, "Data1");
    enqueue_data(test_queue, "Data2");
    enqueue_data(test_queue, "Data3");

    // Try to enqueue an extra item, which should be rejected
    enqueue_data(test_queue, "Data4");

    // Verify that the queue size hasn't changed and no extra data was enqueued
    TEST_ASSERT_EQUAL_INT(3, test_queue->currentSize);

    queue_teardown();
}

// Test queue behavior with different types of data
void test_enqueue_dequeue_various_data(void) {
    queue_setup();
    int num = 42;
    char *str = "TestString";
    float decimal = 3.14f;

    // Enqueue different types of data
    enqueue(test_queue, &num);
    enqueue(test_queue, str);
    enqueue(test_queue, &decimal);

    // Dequeue and check the data
    int *dequeued_num = (int *)dequeue(test_queue);
    TEST_ASSERT_EQUAL_INT(42, *dequeued_num);

    char *dequeued_str = (char *)dequeue(test_queue);
    TEST_ASSERT_EQUAL_STRING("TestString", dequeued_str);

    float *dequeued_float = (float *)dequeue(test_queue);
    TEST_ASSERT_EQUAL_FLOAT(3.14f, *dequeued_float);

    //free(dequeued_str);
    //queue_teardown();
}
