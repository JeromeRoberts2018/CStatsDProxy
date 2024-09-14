#include "unity.h"
#include "../include/http.h"
#include "../include/config_reader.h"
#include "test_config.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Declare synchronization variables
int server_ready = 0;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t server_cond = PTHREAD_COND_INITIALIZER;

extern Config config;
pthread_t server_thread;
extern test_conf_t test_config;

// Helper function to create a real socket, send an HTTP request, and read the response
int create_test_socket(const char* request, char* response, size_t response_size) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(test_config.http_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection to server failed");
        close(sockfd);
        return -1;
    }

    // Send the request to the server
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("write failed");
        close(sockfd);
        return -1;
    }

    // Read the response from the server
    ssize_t bytes_read = read(sockfd, response, response_size - 1);
    if (bytes_read < 0) {
        perror("read failed");
        close(sockfd);
        return -1;
    }

    // Null-terminate the response string
    response[bytes_read] = '\0';

    return sockfd;
}

// Define the callback function for testing
void server_ready_callback(void) {
    pthread_mutex_lock(&server_mutex);
    server_ready = 1;
    pthread_cond_signal(&server_cond);
    pthread_mutex_unlock(&server_mutex);
}

// Function to start the HTTP server with the callback in tests
pthread_t http_start() {
    config.LOGGING_ENABLED = 0;
    config.HTTP_ENABLED = 1;
    pthread_t thread;

    // Set up the server configuration including the callback
    HttpConfig mock_http_config = { 
        .port = test_config.http_port, 
        .ip_address = "127.0.0.1", 
        .callback = server_ready_callback
    };
    // Start the server with the config (callback included)
    int ret = pthread_create(&thread, NULL, http_server, &mock_http_config);
    TEST_ASSERT_EQUAL(0, ret);

    // Wait for the server_ready_callback to signal that the server is ready
    pthread_mutex_lock(&server_mutex);
    while (server_ready == 0) {
        pthread_cond_wait(&server_cond, &server_mutex);
    }
    pthread_mutex_unlock(&server_mutex);

    return thread;
}

// Function to stop the HTTP server by passing the thread handle
void http_end(pthread_t thread) {
    pthread_cancel(thread);  // Cancel the server thread
    pthread_join(thread, NULL);  // Wait for the server thread to exit

    pthread_mutex_lock(&server_mutex);
    server_ready = 0;
    pthread_mutex_unlock(&server_mutex);
}

// Test function for handle_request (200 OK)
void test_handle_request_healthcheck(void) {
    char response[256] = {0};
    int sockfd = create_test_socket("GET /healthcheck HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(-1, sockfd);

    // Check the response
    TEST_ASSERT_TRUE(strstr(response, "200 OK") != NULL);

    close(sockfd);
}

// Test function for handle_request (404 NOT FOUND)
void test_handle_request_not_found(void) {
    char response[256] = {0};
    int sockfd = create_test_socket("GET /unknown HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(-1, sockfd);

    // Check the response
    TEST_ASSERT_TRUE(strstr(response, "404 NOT FOUND") != NULL);

    close(sockfd);
}

void test_handle_request_invalid_method(void) {
    char response[256] = {0};
    int sockfd = create_test_socket("POST /healthcheck HTTP/1.1\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(-1, sockfd);

    // Check for a "405 Method Not Allowed" response
    TEST_ASSERT_TRUE(strstr(response, "405 METHOD NOT ALLOWED") != NULL);

    close(sockfd);
}

void test_handle_request_malformed(void) {
    char response[256] = {0};
    int sockfd = create_test_socket("BAD REQUEST\r\n\r\n", response, sizeof(response));
    TEST_ASSERT_NOT_EQUAL(-1, sockfd);

    // Check for a "400 Bad Request" response or any appropriate error handling
    TEST_ASSERT_TRUE(strstr(response, "400 BAD REQUEST") != NULL);

    close(sockfd);
}

void* thread_request(void* arg) {
    char response[256] = {0};
    int sockfd = create_test_socket("GET /healthcheck HTTP/1.1\r\n\r\n", response, sizeof(response));
    if (sockfd != -1) {
        close(sockfd);
    }
    return NULL;
}

void test_concurrent_requests(void) {
    pthread_t threads[10];

    // Spawn multiple threads to simulate concurrent requests
    for (int i = 0; i < 10; i++) {
        int ret = pthread_create(&threads[i], NULL, thread_request, NULL);
        TEST_ASSERT_EQUAL(0, ret);
    }

    // Wait for all threads to complete
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }
}

