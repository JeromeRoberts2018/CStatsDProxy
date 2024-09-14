#include "unity.h"
#include "global.h"
#include "queue.h"
#include <string.h>
#include <stdlib.h>

extern Queue *requeue;

// Set up the test environment
void global_setup(void) {
    requeue = initQueue(5);
}

// Tear down the test environment
void global_teardown(void) {
    free(requeue);
}

// Test injectMetric function
void test_injectMetric(void) {
    const char *metricName = "testMetric";
    int metricValue = 100;
    
    injectMetric(metricName, metricValue);
    
    // Check that the queue contains the expected formatted metric
    char *queuedMetric = (char *)dequeue(requeue);
    char expected[256];
    snprintf(expected, sizeof(expected), "CStatsDProxy.metrics.%s:%d|c", metricName, metricValue);

    // Check if the formatted metric matches the enqueued data
    TEST_ASSERT_NOT_NULL(queuedMetric);
    TEST_ASSERT_EQUAL_STRING(expected, queuedMetric);

    free(queuedMetric);
}

// Test injectPacket function
void test_injectPacket(void) {
    const char *packet = "testPacket";

    injectPacket(packet);

    // Check that the packet was correctly enqueued
    char *queuedPacket = (char *)dequeue(requeue);
    TEST_ASSERT_NOT_NULL(queuedPacket);
    TEST_ASSERT_EQUAL_STRING(packet, queuedPacket);
}

// Test queue full behavior for injectMetric
void test_injectMetric_queue_full(void) {
    const char *metricName = "testMetricFull";
    int metricValue = 999;

    // Fill the queue to its max size
    for (int i = 0; i < 5; i++) {
        injectMetric(metricName, metricValue);
    }

    injectMetric("overflowMetric", 123);

    // Check that no additional item was added
    TEST_ASSERT_EQUAL_INT(5, requeue->currentSize);
}

// Test queue full behavior for injectPacket
void test_injectPacket_queue_full(void) {
    const char *packet = "testPacketFull";

    // Fill the queue to its max size
    for (int i = 0; i < 5; i++) {
        injectPacket(packet);
    }

    injectPacket("overflowPacket");

    // Check that no additional packet was added
    TEST_ASSERT_EQUAL_INT(5, requeue->currentSize);
}

// Test isMetricValid
void test_isMetricValid_valid(void) {
    const char *metric = "valid.metric:100|c";
    TEST_ASSERT_TRUE(isMetricValid(metric));
}

void test_isMetricValid_too_long(void) {
    char longMetric[501];
    memset(longMetric, 'a', 500);
    longMetric[500] = '\0';
    
    TEST_ASSERT_FALSE(isMetricValid(longMetric));
}

void test_isMetricValid_invalid_chars(void) {
    const char *metric = "invalid#metric@100|c";
    TEST_ASSERT_FALSE(isMetricValid(metric));
}

// Test is_safe_string
void test_is_safe_string_valid(void) {
    const char *valid_str = "Hello World!";
    TEST_ASSERT_TRUE(is_safe_string(valid_str));
}

void test_is_safe_string_invalid(void) {
    const char *invalid_str = "Hello@World!";
    TEST_ASSERT_FALSE(is_safe_string(invalid_str));
}
