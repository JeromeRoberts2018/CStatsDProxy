#include "unity.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/logger.h"
#include "../include/config_reader.h"


static char output_buffer[1024];
static FILE *original_stdout;
static FILE *log_capture;

extern Config config;

// Helper function to capture stdout in the buffer (used for log tests)
void redirect_stdout_to_buffer() {
    fflush(stdout);
    original_stdout = stdout;
    log_capture = fmemopen(output_buffer, sizeof(output_buffer), "w");
    stdout = log_capture;
}

// Restore stdout to its original state
void restore_stdout() {
    fflush(log_capture);
    stdout = original_stdout;
}

// Test case 1: Ensure that log is written when logging is enabled
void test_write_log_logging_enabled(void) {
    config.LOGGING_ENABLED = 1;
    memset(output_buffer, 0, sizeof(output_buffer));

    redirect_stdout_to_buffer();
    write_log("Test log message: %d", 42);
    restore_stdout();

    fflush(stdout);
    TEST_ASSERT_NOT_NULL(strstr(output_buffer, "Test log message: 42"));
}

// Test case 2: Ensure that nothing is written when logging is disabled
void test_write_log_logging_disabled(void) {
    config.LOGGING_ENABLED = 0;
    memset(output_buffer, 0, sizeof(output_buffer));

    redirect_stdout_to_buffer();
    write_log("This should not be logged");
    restore_stdout();

    fflush(stdout);
    TEST_ASSERT_EQUAL_STRING("", output_buffer);  // Nothing should be written
}
