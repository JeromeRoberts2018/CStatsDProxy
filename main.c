// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "lib/queue.h"
#include "lib/worker.h"
#include "lib/logger.h"
#include "lib/config_reader.h"

int UDP_PORT;
char LISTEN_UDP_IP[16];
int DEST_UDP_PORT;
char DEST_UDP_IP[16];
int MAX_MESSAGE_SIZE;
int BUFFER_SIZE;
int MAX_THREADS;
int MAX_QUEUE_SIZE;
int LOGGING_INTERVAL;
int LOGGING_ENABLED;
char LOGGING_FILE_NAME[256];

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

// Function prototypes
void *logging_thread(void *arg);
//void *worker_thread(void *arg);


int main() {
    // Read config file
    if (read_config("conf/config.conf") == -1) {
        write_log(LOGGING_FILE_NAME, "Failed to read configuration");
        return 1;
    }
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
        args[i].bufferSize = BUFFER_SIZE; // Assign the buffer size
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    if (LOGGING_ENABLED) {
        write_log(LOGGING_FILE_NAME, "Logging enabled");
        pthread_t log_thread;
        pthread_create(&log_thread, NULL, logging_thread, args);
    }

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
            printf("Invalid IP address: %s", LISTEN_UDP_IP);
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
    if (LOGGING_ENABLED) {
        //pthread_cancel(log_thread);
        //pthread_join(log_thread, NULL);
    }
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
            write_log(LOGGING_FILE_NAME, "{ \"WorkerID\": %d, \"QueueSize\": %d }", workerArgs[i].workerID, queue->currentSize);
            //printf("{ \"WorkerID\": %d, \"QueueSize\": %d }\n", workerArgs[i].workerID, workerArgs[i].queue->currentSize);
            //fflush(stdout);
            pthread_mutex_unlock(&queue->mutex);
        }
        sleep(LOGGING_INTERVAL);
    }

    return NULL;
}