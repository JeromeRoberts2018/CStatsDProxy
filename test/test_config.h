#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

// Struct to hold argument configuration
typedef struct {
    int http_port;
    int verbose;
} test_conf_t;

extern test_conf_t test_config;  // Declare the global configuration

#endif  // ARG_CONFIG_H
