#include "unity.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "test_config.h"

// Declare test functions from the test files

// HTTP
extern void test_handle_request_healthcheck(void);
extern void test_handle_request_not_found(void);
extern void test_handle_request_invalid_method(void);
extern void test_handle_request_malformed(void);
extern void test_concurrent_requests(void);
extern pthread_t http_start();
extern void http_end(pthread_t thread);

// Logger
extern void test_write_log_logging_enabled(void);
extern void test_write_log_logging_disabled(void);

// Queue
extern void test_initQueue(void);
extern void test_enqueue_data(void);
extern void test_dequeue_data(void);
extern void test_enqueue_full_queue(void);
extern void test_enqueue_dequeue_various_data(void);
extern void queue_setup(void);
extern void queue_teardown(void);

// Global
extern void test_injectMetric(void);
extern void test_injectPacket(void);
extern void test_injectMetric_queue_full(void);
extern void test_injectPacket_queue_full(void);
extern void test_isMetricValid_valid(void);
extern void test_isMetricValid_too_long(void);
extern void test_isMetricValid_invalid_chars(void);
extern void test_is_safe_string_valid(void);
extern void test_is_safe_string_invalid(void);
extern void test_handle_request_malformed(void);
extern void test_concurrent_requests(void);
extern void global_setup(void);
extern void global_teardown(void);

// Requeue
extern void test_init_requeue_thread(void);
extern void test_requeue_thread_logic(void);

// Config Reader
extern void test_read_config_success(void);
extern void test_read_config_missing_values(void);
extern void test_read_config_ignore_comments_and_blanks(void);
extern void test_read_config_file_not_found(void);
extern void cfg_read_setup(void);
extern void cfg_read_teardown(void);


// Struct to hold argument configuration
test_conf_t test_config;

int validate_port(const char *arg, int min, int max) {
    char *endptr;
    long port = strtol(arg, &endptr, 10);

    // Ensure entire string was converted and it's within bounds
    if (*endptr == '\0' && port >= min && port <= max) {
        return (int)port;
    } else {
        return -1;  // Indicate invalid input
    }
}

// Function to parse command-line arguments into the arg_conf struct
void parse_args(int argc, char **argv, test_conf_t *test_config) {
    test_config->http_port = 8080;  // Default HTTP port
    test_config->verbose = 0;       // Default is non-verbose

    // Loop through the arguments to find specific options
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--http_port=", 12) == 0) {
            int port = validate_port(argv[i] + 12, 75, 65500);
            if (port != -1) {
                test_config->http_port = port;
            } else {
                fprintf(stderr, "Error: HTTP port is invalid or out of range (75-65500).\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            test_config->verbose = 1;  // Enable verbose mode
        } else {
            printf("Unknown argument: %s\n", argv[i]);
        }
    }
}

void setUp(void) {
}

void tearDown(void) {
}

int main(int argc, char **argv) {

    // Parse command-line arguments and populate config
    parse_args(argc, argv, &test_config);

    UNITY_BEGIN();
    // Start Logger tests
    RUN_TEST(test_write_log_logging_enabled);
    RUN_TEST(test_write_log_logging_disabled);

    // Start HTTP tests
    // If verbose mode is enabled, print the configuration
    if (test_config.verbose) {
        printf("Running tests with HTTP port: %d\n", test_config.http_port);
    }
    pthread_t thread = http_start(); 
    RUN_TEST(test_handle_request_healthcheck);
    RUN_TEST(test_handle_request_not_found);
    RUN_TEST(test_handle_request_invalid_method);
    RUN_TEST(test_handle_request_malformed);
    RUN_TEST(test_concurrent_requests);
    http_end(thread);

    // Queue (queue.c)
    queue_setup();
    RUN_TEST(test_initQueue);
    RUN_TEST(test_enqueue_data);
    RUN_TEST(test_dequeue_data);
    RUN_TEST(test_enqueue_full_queue);
    RUN_TEST(test_enqueue_dequeue_various_data);
    queue_teardown();

    // Global Functions (global.c)
    global_setup();
    RUN_TEST(test_injectMetric);
    RUN_TEST(test_injectPacket);
    RUN_TEST(test_injectMetric_queue_full);
    RUN_TEST(test_injectPacket_queue_full);
    RUN_TEST(test_isMetricValid_valid);
    RUN_TEST(test_isMetricValid_too_long);
    RUN_TEST(test_isMetricValid_invalid_chars);
    RUN_TEST(test_is_safe_string_valid);
    RUN_TEST(test_is_safe_string_invalid);
    global_teardown();

    // Test Requeue
    RUN_TEST(test_init_requeue_thread);
    RUN_TEST(test_requeue_thread_logic);

    // Config Reader
    cfg_read_setup();
    RUN_TEST(test_read_config_success);
    RUN_TEST(test_read_config_missing_values);
    RUN_TEST(test_read_config_ignore_comments_and_blanks);
    RUN_TEST(test_read_config_file_not_found);
    cfg_read_teardown();

    // Worker
    // ToDo: worker needs a complex test code and maybe even some modification
    // to make it work, the problem is the threads using dequeue. I don't 
    // want to inject a function and risk it slowing down the worker threads
    // as processing speed is the main goal of the worker thread. 

    return UNITY_END();
}
