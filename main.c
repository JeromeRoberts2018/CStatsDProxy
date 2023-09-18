// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"

// Declare variables to hold constants
int UDP_PORT = 8125;
int DEST_UDP_PORT = 8127;
char DEST_UDP_IP[16] = "127.0.0.1";
int MAX_MESSAGE_SIZE = 2024;
int BUFFER_SIZE = 2048;
int MAX_THREADS = 15;
int MAX_QUEUE_SIZE = 10000;

struct Stats {
    char key[256];
    char type[4];
    int value;
};

struct WorkerArgs {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int workerID;
};

// Function prototypes
void *logging_thread(void *arg);
void *worker_thread(void *arg);

int main() {
    Queue *queue = initQueue(MAX_QUEUE_SIZE);
    int sharedUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(DEST_UDP_PORT);
    destAddr.sin_addr.s_addr = inet_addr(DEST_UDP_IP);

    pthread_t threads[MAX_THREADS];
    struct WorkerArgs args[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; ++i) {
        args[i].queue = queue; // Assign the specific queue to each worker
        args[i].udpSocket = sharedUdpSocket; // Assign the shared UDP socket
        args[i].destAddr = destAddr; // Assign the shared destination address
        args[i].workerID = i; // Assign a unique worker ID to each thread

        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    pthread_t log_thread;
    pthread_create(&log_thread, NULL, logging_thread, args); // Pass args array to logging_thread

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        char *buffer = malloc(MAX_MESSAGE_SIZE);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &addrSize);

        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            enqueue(queue, buffer);
        } else if (recvLen == 0) {
            printf("Received zero bytes. Connection closed or terminated.\n"); // Debug log
            free(buffer);
        } else {
            printf("recvfrom() returned an error: %zd\n", recvLen); // Debug log
            free(buffer);
        }
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    pthread_cancel(log_thread);
    pthread_join(log_thread, NULL);
    close(sharedUdpSocket);
    close(udpSocket);

    return 0;
}

void *logging_thread(void *arg) {
    struct WorkerArgs *workerArgs = (struct WorkerArgs *) arg;
    int numWorkers = MAX_THREADS; // Change this to the actual number of worker threads

    while (1) {
        for (int i = 0; i < numWorkers; ++i) {
            Queue *queue = workerArgs[i].queue;
            pthread_mutex_lock(&queue->mutex);
            printf("{ \"WorkerID\": %d, \"QueueSize\": %d }\n", workerArgs[i].workerID, workerArgs[i].queue->currentSize);
            fflush(stdout);
            pthread_mutex_unlock(&queue->mutex);
        }
        sleep(30);
    }

    return NULL;
}

void *worker_thread(void *arg) {
    struct WorkerArgs *args = (struct WorkerArgs *) arg;
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;

    struct Stats stats[BUFFER_SIZE];
    int statsCount = 0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        stats[i].value = 0;
    }

    while (1) {
        char *buffer = dequeue(queue);
        if (buffer == NULL) {
            continue;
        }
        char *key, *valueStr, *type;
        int value;
        char *savePtr1;

        key = strtok_r(buffer, ":", &savePtr1);
        valueStr = strtok_r(NULL, "|", &savePtr1);
        type = strtok_r(NULL, "|", &savePtr1);

        if (key && valueStr && type) {
                value = atoi(valueStr);

            int found = 0;
            for (int j = 0; j < statsCount; j++) {
                if (strcmp(stats[j].key, key) == 0 && strcmp(stats[j].type, type) == 0) {
                    stats[j].value += value;
                    found = 1;
                    break;
                }
            }

            if (!found) {
                strncpy(stats[statsCount].key, key, sizeof(stats[statsCount].key));
                strncpy(stats[statsCount].type, type, sizeof(stats[statsCount].type));
                stats[statsCount].value = value;
                statsCount++;
            }
            //if (statsCount >= BUFFER_SIZE) {
                for (int j = 0; j < statsCount; j++) {
                    char sendBuffer[2024];
                    snprintf(sendBuffer, sizeof(sendBuffer), "%s:%d|%s", stats[j].key, stats[j].value, stats[j].type);

                    ssize_t sentBytes = sendto(udpSocket, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
                    if (sentBytes == -1) {
                        perror("sendto");
                    }
                }

                for (int j = 0; j < statsCount; j++) {
                    stats[j].value = 0;
                }
                statsCount = 0;
            //}
        }

        free(buffer);
    }

    return NULL;
}
