#include "unity.h"
#include "../include/config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Mock file for testing
const char *test_config_file = "test_config.conf";

// Helper function to write test config data to a file
void write_test_config(const char *config_data) {
    FILE *file = fopen(test_config_file, "w");
    if (file) {
        fputs(config_data, file);
        fclose(file);
    }
}

// Setup function to create a mock config file
void cfg_read_setup(void) {
    // Sample configuration data
    const char *config_data =
        "# This is a comment\n"
        "UDP_PORT=8080\n"
        "LISTEN_UDP_IP=127.0.0.1\n"
        "DEST_UDP_PORT=9090\n"
        "DEST_UDP_IP=192.168.1.1\n"
        "MAX_MESSAGE_SIZE=1024\n"
        "BUFFER_SIZE=2048\n"
        "MAX_THREADS=4\n"
        "MAX_QUEUE_SIZE=1000\n"
        "LOGGING_INTERVAL=60\n"
        "LOGGING_ENABLED=1\n"
        "CLONE_ENABLED=0\n"
        "CLONE_DEST_UDP_PORT=10000\n"
        "CLONE_DEST_UDP_IP=10.0.0.1\n"
        "HTTP_ENABLED=1\n"
        "HTTP_PORT=80\n"
        "HTTP_LISTEN_IP=0.0.0.0\n"
        "OUTBOUND_UDP_TIMEOUT=5000\n";

    write_test_config(config_data);
}

// Teardown function to clean up after each test
void cfg_read_teardown(void) {
    remove(test_config_file);  // Remove the test config file after each test
}

// Test to check if config values are correctly parsed
void test_read_config_success(void) {
    int result = read_config(test_config_file);
    TEST_ASSERT_EQUAL_INT(0, result);  // Ensure function succeeded

    // Check if values were correctly set in the global config
    TEST_ASSERT_EQUAL_INT(8080, config.UDP_PORT);
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", config.LISTEN_UDP_IP);
    TEST_ASSERT_EQUAL_INT(9090, config.DEST_UDP_PORT);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", config.DEST_UDP_IP);
    TEST_ASSERT_EQUAL_INT(1024, config.MAX_MESSAGE_SIZE);
    TEST_ASSERT_EQUAL_INT(2048, config.BUFFER_SIZE);
    TEST_ASSERT_EQUAL_INT(4, config.MAX_THREADS);
    TEST_ASSERT_EQUAL_INT(1000, config.MAX_QUEUE_SIZE);
    TEST_ASSERT_EQUAL_INT(60, config.LOGGING_INTERVAL);
    TEST_ASSERT_EQUAL_INT(1, config.LOGGING_ENABLED);
    TEST_ASSERT_EQUAL_INT(0, config.CLONE_ENABLED);
    TEST_ASSERT_EQUAL_INT(10000, config.CLONE_DEST_UDP_PORT);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", config.CLONE_DEST_UDP_IP);
    TEST_ASSERT_EQUAL_INT(1, config.HTTP_ENABLED);
    TEST_ASSERT_EQUAL_INT(80, config.HTTP_PORT);
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", config.HTTP_LISTEN_IP);
    TEST_ASSERT_EQUAL_INT(5000, config.OUTBOUND_UDP_TIMEOUT);
}

// Test for handling missing or malformed entries
void test_read_config_missing_values(void) {
    const char *config_data =
        "UDP_PORT=\n"
        "LISTEN_UDP_IP=\n"
        "DEST_UDP_PORT=9090\n";

    write_test_config(config_data);
    int result = read_config(test_config_file);
    TEST_ASSERT_EQUAL_INT(0, result);  // Ensure function succeeded
    
    // Check that unset values remain default (e.g., 0 or empty string)
    TEST_ASSERT_EQUAL_INT(0, config.UDP_PORT);
    TEST_ASSERT_EQUAL_STRING("", config.LISTEN_UDP_IP);
    TEST_ASSERT_EQUAL_INT(9090, config.DEST_UDP_PORT);  // Check proper values
}

// Test for handling comments and blank lines
void test_read_config_ignore_comments_and_blanks(void) {
    const char *config_data =
        "# This is a comment\n"
        "\n"  // Blank line
        "UDP_PORT=8080\n"
        "# Another comment\n"
        "\n"  // Another blank line
        "LISTEN_UDP_IP=127.0.0.1\n";

    write_test_config(config_data);
    int result = read_config(test_config_file);
    TEST_ASSERT_EQUAL_INT(0, result);  // Ensure function succeeded

    // Check that the values were correctly set
    TEST_ASSERT_EQUAL_INT(8080, config.UDP_PORT);
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", config.LISTEN_UDP_IP);
}

// Test if the function returns -1 on file open failure
void test_read_config_file_not_found(void) {
    int result = read_config("non_existent_file.conf");
    TEST_ASSERT_EQUAL_INT(-1, result);  // Ensure failure is handled properly
}

