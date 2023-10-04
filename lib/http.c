#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "logger.h"
#include "config_reader.h"

void *handle_request(void *client_sock) {
    int sock = *((int *)client_sock);
    free(client_sock);

    char buffer[256];
    char response[] = "HTTP/1.1 200 OK\r\n"
                      "Content-Length: 2\r\n"
                      "Content-Type: text/plain\r\n"
                      "\r\n"
                      "OK";
    char response404[] = "HTTP/1.1 404 NOT FOUND\r\n"
                      "Content-Length: 3\r\n"
                      "Content-Type: text/plain\r\n"
                      "\r\n"
                      "404";

    ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("read failed");
        return NULL;
    }
    if (bytes_read > 0 && bytes_read < sizeof(buffer)) {
        buffer[bytes_read] = '\0';
    }


    if (strstr(buffer, "/healthcheck")) {
        write(sock, response, sizeof(response) - 1);
    } else {
        write(sock, response404, sizeof(response404) - 1);
    }

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

    int sockfd, newsockfd;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return NULL;
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
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

    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("accept failed");
            return NULL;
        }
;

        int *new_sock = malloc(sizeof(int));
        *new_sock = newsockfd;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_request, (void *)new_sock) < 0) {
            perror("could not create thread");
            return NULL;
        }
        pthread_detach(client_thread);
    }

    return NULL;
}
