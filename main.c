// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"

// Declare variables to hold constants
int UDP_PORT;
int DEST_UDP_PORT;
char DEST_UDP_IP[16];
int MAX_MESSAGE_SIZE;
int BUFFER_SIZE;
int MAX_THREADS;
int MAX_QUEUE_SIZE;

struct Stats {
    char key[256];
    char type[4];
    int value;
};

// Thread to log queue size every 30 seconds
void *logging_thread(void *arg) {
    Queue *queue = (Queue *) arg;
    while (1) {
        sleep(30);  // Wait for 30 seconds

        pthread_mutex_lock(&queue->mutex);
        printf("Current Queue Size: %d\n", queue->currentSize);
        pthread_mutex_unlock(&queue->mutex);
    }
    return NULL;
}

// Read config file and populate constants
void read_config() {
    FILE *fp = fopen("config.conf", "r");
    if (fp == NULL) {
        perror("Could not open config file");
        exit(EXIT_FAILURE);
    }
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[128];
        char value[128];
        
        sscanf(line, "%[^=]=%s", key, value);
        
        if (strcmp(key, "UDP_PORT") == 0) UDP_PORT = atoi(value);
        else if (strcmp(key, "DEST_UDP_PORT") == 0) DEST_UDP_PORT = atoi(value);
        else if (strcmp(key, "DEST_UDP_IP") == 0) strncpy(DEST_UDP_IP, value, sizeof(DEST_UDP_IP));
        else if (strcmp(key, "MAX_MESSAGE_SIZE") == 0) MAX_MESSAGE_SIZE = atoi(value);
        else if (strcmp(key, "BUFFER_SIZE") == 0) BUFFER_SIZE = atoi(value);
        else if (strcmp(key, "MAX_THREADS") == 0) MAX_THREADS = atoi(value);
        else if (strcmp(key, "MAX_QUEUE_SIZE") == 0) MAX_QUEUE_SIZE = atoi(value);
    }
    fclose(fp);
}

void *worker_thread(void *arg);

int main() {
    // Read config file
    read_config();

    // Initialize job queue
    Queue *queue = initQueue(MAX_QUEUE_SIZE);

    // Start worker threads
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_create(&threads[i], NULL, worker_thread, queue);
    }

    // Create logging thread
    pthread_t log_thread;
    pthread_create(&log_thread, NULL, logging_thread, queue);

    // UDP server setup
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    // Receive loop
    while (1) {
        char *buffer = malloc(MAX_MESSAGE_SIZE);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *) &clientAddr, &addrSize);
        
        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            enqueue(queue, buffer);
        } else {
            free(buffer);
        }
    }
}

// Worker thread to process the buffer and send stats to another server

void *worker_thread(void *arg) {
    char (*buffer)[MAX_MESSAGE_SIZE] = (char (*)[MAX_MESSAGE_SIZE])arg;
    struct Stats stats[BUFFER_SIZE];
    int statsCount = 0;
    int udpSocket;
    struct sockaddr_in destAddr;
    
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(DEST_UDP_PORT);
    destAddr.sin_addr.s_addr = inet_addr(DEST_UDP_IP);

    // Initialize stats
    for (int i = 0; i < BUFFER_SIZE; i++) {
        stats[i].value = 0;
    }

    // Process the buffer and populate stats
    for (int i = 0; i < BUFFER_SIZE; i++) {
        char *key, *valueStr, *type;
        int value;
        char *savePtr1, *savePtr2;

        // Tokenize the received string into key, value, and type
        key = strtok_r(buffer[i], ":", &savePtr1);
        valueStr = strtok_r(NULL, "|", &savePtr1);
        type = strtok_r(NULL, "|", &savePtr1);

        if (key && valueStr && type) {
            value = atoi(valueStr);

            // Check if this key-type pair already exists in stats
            int found = 0;
            for (int j = 0; j < statsCount; j++) {
                if (strcmp(stats[j].key, key) == 0 && strcmp(stats[j].type, type) == 0) {
                    stats[j].value += value;
                    found = 1;
                    break;
                }
            }

            // If not found, add new entry
            if (!found) {
                strncpy(stats[statsCount].key, key, sizeof(stats[statsCount].key));
                strncpy(stats[statsCount].type, type, sizeof(stats[statsCount].type));
                stats[statsCount].value = value;
                statsCount++;
            }
        }
    }

    // Send the stats to another server
    for (int i = 0; i < statsCount; i++) {
        char sendBuffer[MAX_MESSAGE_SIZE];
        snprintf(sendBuffer, sizeof(sendBuffer), "%s:%d|%s", stats[i].key, stats[i].value, stats[i].type);
        sendto(udpSocket, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *) &destAddr, sizeof(destAddr));
    }

    free(buffer);
    close(udpSocket);
    pthread_exit(NULL);
}