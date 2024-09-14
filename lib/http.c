#include "../include/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../include/logger.h"
#include "../include/config_reader.h"

#define MAX_THREADS 25
volatile int active_threads = 0;
pthread_mutex_t active_threads_mutex = PTHREAD_MUTEX_INITIALIZER;


void *handle_request(void *client_sock) {
    int sock = *((int *)client_sock);
    free(client_sock);

    char buffer[256];
    char response_ok[] = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: 2\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n"
                         "OK";
    char response404[] = "HTTP/1.1 404 NOT FOUND\r\n"
                         "Content-Length: 3\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n"
                         "404";
    char response405[] = "HTTP/1.1 405 METHOD NOT ALLOWED\r\n"
                         "Content-Length: 18\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n"
                         "Method Not Allowed";
    char response400[] = "HTTP/1.1 400 BAD REQUEST\r\n"
                         "Content-Length: 12\r\n"
                         "Content-Type: text/plain\r\n"
                         "\r\n"
                         "Bad Request";

    ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read failed");
        return NULL;
    }
    if (bytes_read > 0 && bytes_read < sizeof(buffer)) {
        buffer[bytes_read] = '\0';
    }

    // Basic check for malformed request (e.g., no HTTP version)
    if (!strstr(buffer, "HTTP/1.1")) {
        write(sock, response400, sizeof(response400) - 1);
    }
    // Only allow GET requests, respond 405 for anything else
    else if (strncmp(buffer, "GET", 3) != 0) {
        write(sock, response405, sizeof(response405) - 1);
    }
    // Check for /healthcheck
    else if (strstr(buffer, "/healthcheck")) {
        write(sock, response_ok, sizeof(response_ok) - 1);
    }
    // If any other URL, respond with 404 NOT FOUND
    else {
        write(sock, response404, sizeof(response404) - 1);
    }

    // Clean up
    pthread_mutex_lock(&active_threads_mutex);
    active_threads--;
    pthread_mutex_unlock(&active_threads_mutex);
    close(sock);
    return NULL;
}



void *http_server(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    if (config.HTTP_ENABLED == 0) {
        return NULL;
    }
    write_log("Starting HTTP server");
    HttpConfig *conf = (HttpConfig *)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return NULL;
    }
    // Set the SO_REUSEADDR option on the socket
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        return NULL;
    }
    // Set a receive timeout for the client connection
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds timeout for receiving data
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt for SO_RCVTIMEO failed");
        close(sockfd);
        return NULL;
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(conf->port);
    inet_pton(AF_INET, conf->ip_address, &(serv_addr.sin_addr));

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind failed");
        return NULL;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen failed");
        return NULL;
    }

    if (conf->callback != NULL) { // Used for Unity Testing
        conf->callback();
    }
    
    clilen = sizeof(cli_addr);

    while (1) {
        pthread_mutex_lock(&active_threads_mutex);
        if (active_threads >= MAX_THREADS) {
            pthread_mutex_unlock(&active_threads_mutex);
            usleep(1000);  // Sleep for a short duration before rechecking
            continue;
        }
        pthread_mutex_unlock(&active_threads_mutex);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("accept failed");
            return NULL;
        }

        int *new_sock = malloc(sizeof(int));
        *new_sock = newsockfd;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_request, (void *)new_sock) < 0) {
            perror("could not create thread");
            return NULL;
        }
        pthread_mutex_lock(&active_threads_mutex);
        active_threads++;
        pthread_mutex_unlock(&active_threads_mutex);    
        pthread_detach(client_thread);
    }

    return NULL;
}
