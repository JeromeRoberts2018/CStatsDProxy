// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"

int UDP_PORT;
char LISTEN_UDP_IP[16];
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

struct WorkerArgs {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int workerID;
};

// Function prototypes
void *logging_thread(void *arg);
void *worker_thread(void *arg);

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
        else if (strcmp(key, "LISTEN_UDP_IP") == 0) strncpy(LISTEN_UDP_IP, value, sizeof(LISTEN_UDP_IP));
        else if (strcmp(key, "DEST_UDP_PORT") == 0) DEST_UDP_PORT = atoi(value);
        else if (strcmp(key, "DEST_UDP_IP") == 0) strncpy(DEST_UDP_IP, value, sizeof(DEST_UDP_IP));
        else if (strcmp(key, "MAX_MESSAGE_SIZE") == 0) MAX_MESSAGE_SIZE = atoi(value);
        else if (strcmp(key, "BUFFER_SIZE") == 0) BUFFER_SIZE = atoi(value);
        else if (strcmp(key, "MAX_THREADS") == 0) MAX_THREADS = atoi(value);
        else if (strcmp(key, "MAX_QUEUE_SIZE") == 0) MAX_QUEUE_SIZE = atoi(value);
    }
    fclose(fp);
}

int main() {
    // Read config file
    read_config();
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

    //pthread_t log_thread;
    //pthread_create(&log_thread, NULL, logging_thread, args); // Pass args array to logging_thread

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(UDP_PORT);

    // Configure the IP address based on the LISTEN_UDP_IP macro
    if (strcmp(LISTEN_UDP_IP, "0.0.0.0") == 0) {
        // Bind to any available IP address
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        // Use the specified IP address
        if (inet_pton(AF_INET, LISTEN_UDP_IP, &(serverAddr.sin_addr)) <= 0) {
            perror("Invalid IP address");
            exit(1);
        }
    }


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
    //pthread_cancel(log_thread);
    //pthread_join(log_thread, NULL);
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
            //printf("{ \"WorkerID\": %d, \"QueueSize\": %d }\n", workerArgs[i].workerID, workerArgs[i].queue->currentSize);
            fflush(stdout);
            pthread_mutex_unlock(&queue->mutex);
        }
        sleep(30);
    }

    return NULL;
}

void *worker_thread(void *arg) {
    struct WorkerArgs *args = (struct WorkerArgs *)arg;
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;

    char *batchBuffer[100];
    int bufferIndex = 0;
    int processBatchSize = 100;  // Define how many items to process before sending

    while (1) {
        // Dequeue items and store them in batchBuffer
        if (bufferIndex < processBatchSize) {
            char *buffer = dequeue(queue);
            if (buffer != NULL) {
                batchBuffer[bufferIndex++] = buffer;
            }
            continue;
        }

        // Create and clear stats here for each batch
        struct Stats stats[BUFFER_SIZE];
        for (int i = 0; i < BUFFER_SIZE; i++) {
            stats[i].value = 0;
        }

        // Process the items in batchBuffer
        for (int i = 0; i < processBatchSize; ++i) {
            char *buffer = batchBuffer[i];
            // Your existing code for processing each buffer into `stats`
            // ...

            // Once you've processed each buffer, it's safe to free it
            free(buffer);
        }

        // Send the processed data
        for (int i = 0; i < BUFFER_SIZE; i++) {
            if (stats[i].value != 0) { // Assuming zero means the stat is not filled
                char sendBuffer[BUFFER_SIZE];
                snprintf(sendBuffer, sizeof(sendBuffer), "%s:%d|%s", stats[i].key, stats[i].value, stats[i].type);

                ssize_t sentBytes = sendto(udpSocket, sendBuffer, strlen(sendBuffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
                if (sentBytes == -1) {
                    perror("sendto");
                }
            }
        }

        bufferIndex = 0; // Reset the batch buffer index
    }

    return NULL;
}