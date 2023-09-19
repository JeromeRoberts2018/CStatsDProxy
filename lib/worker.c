
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"

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
    int bufferSize;
};

void *worker_thread(void *arg) {
    struct WorkerArgs *args = (struct WorkerArgs *)arg;
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;
    int bufferSize = args->bufferSize;

    int statsCount = 0;

    char *batchBuffer[100];
    int bufferIndex = 0;
    int processBatchSize = 100;  // Define how many items to process before sending

    while (1) {
        if (bufferIndex < processBatchSize) {
            char *buffer = dequeue(queue);
            if (buffer != NULL) {
                batchBuffer[bufferIndex++] = buffer;
            }
            continue;
        }

        struct Stats stats[bufferSize];
        for (int i = 0; i < bufferSize; i++) {
            stats[i].value = 0;
        }

        for (int i = 0; i < processBatchSize; ++i) {
            char *buffer = batchBuffer[i];
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
            }

            free(buffer);
        }

        // Send the processed data
        for (int i = 0; i < statsCount; i++) {
            if (stats[i].value != 0) {
                char sendBuffer[bufferSize];
                snprintf(sendBuffer, sizeof(sendBuffer), "%s:%d|%s", stats[i].key, stats[i].value, stats[i].type);

                ssize_t sentBytes = sendto(udpSocket, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
                if (sentBytes == -1) {
                    perror("sendto");
                }
            }
        }

        // Reset the stats and batch buffer index
        for (int i = 0; i < statsCount; i++) {
            stats[i].value = 0;
        }
        statsCount = 0;
        bufferIndex = 0;
    }

    return NULL;
}
