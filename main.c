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

unsigned long long int packet_counter = 0;
pthread_mutex_t packet_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
Queue **queues = NULL;
Queue *inbound_queue = NULL;

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



// Prototypes
//void udp_server_loop(int udpSocket, Queue **queues, int MAX_THREADS, int MAX_MESSAGE_SIZE, int LOGGING_ENABLED, const char *LOGGING_FILE_NAME, pthread_mutex_t *packet_counter_mutex, int *packet_counter);
void *logging_thread(void *arg);


// Initialize a shared UDP socket for sending data
int initialize_shared_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        write_log(logging_file_name, "Shared Socket creation failed");
        return -1;
    }

    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    address->sin_addr.s_addr = inet_addr(ip);

    return udpSocket;
}

// Initialize a UDP socket for listening
int initialize_listener_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (strcmp(ip, "0.0.0.0") == 0) {
        address->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, ip, &(address->sin_addr)) <= 0) {
            write_log(logging_file_name, "Invalid IP address: %s", ip);
            return -1;
        }
    }

    if (bind(udpSocket, (struct sockaddr *)address, sizeof(*address)) < 0) {
        write_log(logging_file_name, "Bind failed");
        perror("Bind failed");
        return -1;
    }

    return udpSocket;
}

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
    printf("Starting CStatsDProxy server...\n");

    if (read_config("conf/config.conf") == -1) {
        write_log(LOGGING_FILE_NAME, "Failed to read configuration");
        return 1;
    }

    printf("Starting server on %s:%d\n", LISTEN_UDP_IP, UDP_PORT);
    printf("Forwarding to %s:%d\n", DEST_UDP_IP, DEST_UDP_PORT);

    if (LOGGING_ENABLED) {
        write_log(LOGGING_FILE_NAME, "Starting server on %s:%d", LISTEN_UDP_IP, UDP_PORT);
        write_log(LOGGING_FILE_NAME, "Forwarding to %s:%d", DEST_UDP_IP, DEST_UDP_PORT);
    }

    //Queue *queue = initQueue(MAX_QUEUE_SIZE);

    struct sockaddr_in destAddr, serverAddr;
    int sharedUdpSocket = initialize_shared_udp_socket(DEST_UDP_IP, DEST_UDP_PORT, &destAddr, LOGGING_FILE_NAME);
    int udpSocket = initialize_listener_udp_socket(LISTEN_UDP_IP, UDP_PORT, &serverAddr, LOGGING_FILE_NAME);


    if (sharedUdpSocket == -1 || udpSocket == -1) {
        return 1;
    }

    pthread_t threads[MAX_THREADS];
    struct WorkerArgs args[MAX_THREADS];
    queues = malloc(sizeof(Queue*) * MAX_QUEUE_SIZE);

    for (int i = 0; i < MAX_THREADS; ++i) {
        queues[i] = initQueue(MAX_QUEUE_SIZE);
        args[i].queue = queues[i];
        args[i].udpSocket = sharedUdpSocket;
        args[i].destAddr = destAddr;
        args[i].workerID = i;
        args[i].bufferSize = BUFFER_SIZE;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }

    if (LOGGING_ENABLED) {
        write_log(LOGGING_FILE_NAME, "Logging enabled");
        pthread_t log_thread;
        pthread_create(&log_thread, NULL, logging_thread, args);
    }

    inbound_queue = malloc(sizeof(Queue*) * MAX_QUEUE_SIZE);
    inbound_queue = initQueue(MAX_QUEUE_SIZE);
    int RoundRobinCounter = 0;

    while (1) {
        char *buffer = malloc(MAX_MESSAGE_SIZE);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &addrSize);

        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            enqueue(queues[RoundRobinCounter], buffer);
            RoundRobinCounter = (RoundRobinCounter + 1) % MAX_THREADS;

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
    
    //udp_server_loop(udpSocket, queues, MAX_THREADS, MAX_MESSAGE_SIZE, LOGGING_ENABLED, LOGGING_FILE_NAME, &packet_counter_mutex, &packet_counter);

    // Cleanup - although you'd typically never reach here in an infinite loop server.
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
            if (queue->currentSize > 0) { 
                write_log(LOGGING_FILE_NAME, "WorkerID: %d, QueueSize: %d", workerArgs[i].workerID, queue->currentSize);
            }
            pthread_mutex_unlock(&queue->mutex);
        }
        
        pthread_mutex_lock(&packet_counter_mutex);
        if (packet_counter > 0) {
            write_log(LOGGING_FILE_NAME, "Packets Since Last Logging: %llu", packet_counter);
            // Create a StatsD metric string
            char *statsd_metric = malloc(256); // Allocate enough space for the metric
            snprintf(statsd_metric, 256, "CStatsDProxy.logging_interval.packetsreceived:%llu|c", packet_counter);

            // Enqueue the StatsD metric to the worker's queue at index 1
            enqueue(queues[1], statsd_metric);
        }        
        packet_counter = 0;
        pthread_mutex_unlock(&packet_counter_mutex);

    }

    return NULL;
}