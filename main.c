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

unsigned long long packet_counter = 0;
pthread_mutex_t packet_counter_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void *logging_thread(void *arg);


/**
 * Entry point of the CStatsDProxy server application.
 * In summary, this function sets up worker threads to process the data, a logging thread to log statistics, and a main loop to receive and enqueue data.
 *
 * Upon startup, the function:
 * - Reads the configuration file and validates it.
 * - Initializes a shared UDP socket and destination address.
 * - Initializes a queue.
 * - Spawns worker threads and optionally, a logging thread.
 *
 * Once the setup is complete, it enters an infinite loop, where it:
 * - Waits for incoming UDP packets.
 * - Enqueues received packets into the shared queue.
 * - Updates packet counters if logging is enabled.
 *
 * In case of failure at various stages (e.g., bind failure, invalid IP, config read failure), appropriate log messages are generated.
 *
 * @return Returns 1 if the configuration file could not be read; otherwise, the function runs indefinitely and does not return.
 */
int main() {

    printf("Starting CStatsDProxy server...\n"); // To STDOUT, shows that the server is starting

    if (read_config("conf/config.conf") == -1) {
        write_log(LOGGING_FILE_NAME, "Failed to read configuration");
        return 1;
    }

    printf("Starting server on %s:%d\n", LISTEN_UDP_IP, UDP_PORT); // STDOUT, shows where the server is listening
    printf("Forwarding to %s:%d\n", DEST_UDP_IP, DEST_UDP_PORT); // STDOUT, shows where the server is forwarding to

    if (LOGGING_ENABLED) {
        write_log(LOGGING_FILE_NAME, "Starting server on %s:%d", LISTEN_UDP_IP, UDP_PORT);
        write_log(LOGGING_FILE_NAME, "Forwarding to %s:%d", DEST_UDP_IP, DEST_UDP_PORT);
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

    if (strcmp(LISTEN_UDP_IP, "0.0.0.0") == 0) {
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, LISTEN_UDP_IP, &(serverAddr.sin_addr)) <= 0) {
            write_log(LOGGING_FILE_NAME, "Invalid IP address: %s", LISTEN_UDP_IP);
            exit(1);
        }
    }

    if (bind(udpSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        write_log(LOGGING_FILE_NAME, "Bind failed");
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
            if (LOGGING_ENABLED) {
                pthread_mutex_lock(&packet_counter_mutex);
                packet_counter++;
                pthread_mutex_unlock(&packet_counter_mutex);
            }
        } else if (recvLen == 0) {
            if (LOGGING_ENABLED) { write_log(LOGGING_FILE_NAME, "Received zero bytes. Connection closed or terminated."); }
            free(buffer);
        } else {
            if (LOGGING_ENABLED) { write_log(LOGGING_FILE_NAME, "recvfrom() returned an error: %zd", recvLen); }
            free(buffer);
        }
    }

    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }

    close(sharedUdpSocket);
    close(udpSocket);

    return 0;
}

/**
 * Acts as a logging thread that periodically writes status information to a log file.
 * This function is designed to run as a separate thread and logs information about worker queues and packet counts.
 *
 * The function fetches arguments from a WorkerArgs structure and logs the following:
 * - The current size of each worker's queue.
 * - The number of packets processed since the last logging interval.
 *
 * Logging is controlled by the following global variables:
 * - LOGGING_INTERVAL: The time interval (in seconds) between each log.
 * - LOGGING_FILE_NAME: The name of the file where logs are written.
 * - MAX_THREADS: The maximum number of worker threads.
 * 
 * The function uses mutexes to ensure thread-safe access to shared resources.
 *
 * @param arg A pointer to an array of WorkerArgs structures. Each structure contains a queue and a workerID.
 * @return This function returns NULL as it is designed to run indefinitely.
 */
void *logging_thread(void *arg) {
    struct WorkerArgs *workerArgs = (struct WorkerArgs *) arg;
    int numWorkers = MAX_THREADS;

    while (1) {
        sleep(LOGGING_INTERVAL);

        for (int i = 0; i < numWorkers; ++i) {
            Queue *queue = workerArgs[i].queue;
            pthread_mutex_lock(&queue->mutex);
            write_log(LOGGING_FILE_NAME, "WorkerID: %d, QueueSize: %d", workerArgs[i].workerID, queue->currentSize);
            pthread_mutex_unlock(&queue->mutex);
        }
        
        pthread_mutex_lock(&packet_counter_mutex);
        write_log(LOGGING_FILE_NAME, "Packets Since Last Logging: %llu", packet_counter);
        packet_counter = 0;
        pthread_mutex_unlock(&packet_counter_mutex);

    }

    return NULL;
}