// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "lib/queue.h"
#include "lib/worker.h"
#include "lib/logger.h"
#include "lib/config_reader.h"
#include "lib/requeue.h"
#include "lib/global.h"


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
int CLONE_ENABLED;
int CLONE_DEST_UDP_PORT;
char CLONE_DEST_UDP_IP[50];

char VERSION[] = "0.9.1";

int packet_counter = 0;
pthread_mutex_t packet_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
Queue **queues = NULL;

struct WorkerArgs {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int workerID;
    int bufferSize;
};

int initialize_shared_udp_socket(const char *ip, int port, struct sockaddr_in *address) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        write_log("Shared Socket creation failed");
        return -1;
    }

    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    address->sin_addr.s_addr = inet_addr(ip);

    return udpSocket;
}

int initialize_listener_udp_socket(const char *ip, int port, struct sockaddr_in *address) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (strcmp(ip, "0.0.0.0") == 0) {
        address->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_pton(AF_INET, ip, &(address->sin_addr)) <= 0) {
            write_log("Invalid IP address: %s", ip);
            return -1;
        }
    }

    if (bind(udpSocket, (struct sockaddr *)address, sizeof(*address)) < 0) {
        write_log("Bind failed");
        perror("Bind failed");
        return -1;
    }

    return udpSocket;
}

struct MonitorArgs {
    pthread_t *threads;
    struct WorkerArgs *args;
    int num_threads;
};


void *monitor_worker_threads(void *arg) {
    struct MonitorArgs *monitorArgs = (struct MonitorArgs *)arg;
    set_thread_name("Supervisor");
    pthread_t *threads = monitorArgs->threads;
    struct WorkerArgs *args = monitorArgs->args;
    int num_threads = monitorArgs->num_threads;

    while (1) {
        for (int i = 0; i < num_threads; ++i) {
            int ret = pthread_kill(threads[i], 0);  // Check the thread status
            if (ret != 0) {
                if (LOGGING_ENABLED) {
                    write_log("StatsD Worker Thread %d died. Restarting...", i);
                }
                pthread_cancel(threads[i]);
                pthread_join(threads[i], NULL);
                pthread_create(&threads[i], NULL, worker_thread, &args[i]);
            }
        }
        sleep(5);  // Wait for 5 seconds before checking again
    }

    return NULL;
}


int main() {
    printf("Starting CStatsDProxy server: Version %s\n", VERSION);

    if (read_config("conf/config.conf") == -1) {
        write_log("Failed to read configuration");
        return 1;
    }

    if (LOGGING_ENABLED) {
        write_log("Starting server on %s:%d", LISTEN_UDP_IP, UDP_PORT);
        write_log("Forwarding to %s:%d", DEST_UDP_IP, DEST_UDP_PORT);
        if (CLONE_ENABLED) {
            write_log("Cloning to %s:%d", CLONE_DEST_UDP_IP, CLONE_DEST_UDP_PORT);
        }
    }

    struct sockaddr_in destAddr, serverAddr;
    int sharedUdpSocket = initialize_shared_udp_socket(DEST_UDP_IP, DEST_UDP_PORT, &destAddr);
    int udpSocket = initialize_listener_udp_socket(LISTEN_UDP_IP, UDP_PORT, &serverAddr);
    if (sharedUdpSocket == -1 || udpSocket == -1) {
        write_log("Failed to initialize sockets");
        return 1;
    } else {
        write_log("Sockets initialized");
    }

    pthread_t threads[MAX_THREADS];
    struct WorkerArgs args[MAX_THREADS];
    queues = malloc(sizeof(Queue*) * MAX_QUEUE_SIZE);
    write_log("Starting %d worker threads", MAX_THREADS);
    for (int i = 0; i < MAX_THREADS; ++i) {
        queues[i] = initQueue(MAX_QUEUE_SIZE);
        //sleep(1); //this may or may not prevent segfaults
        args[i].queue = queues[i];
        args[i].udpSocket = sharedUdpSocket;
        args[i].destAddr = destAddr;
        args[i].workerID = i;
        args[i].bufferSize = BUFFER_SIZE;
        pthread_create(&threads[i], NULL, worker_thread, &args[i]);
    }
    struct MonitorArgs monitorArgs = { threads, args, MAX_THREADS };
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_worker_threads, &monitorArgs);

    pthread_t requeueThread;
    if (init_requeue_thread(&requeueThread, MAX_THREADS, queues) != 0) {
        write_log("Failed to initialize requeue thread");
        return 1;
    }

    if (LOGGING_ENABLED) {
        write_log("Logging enabled");
    }
    
    int RoundRobinCounter = 0;
    while (1) {
        char *buffer = malloc(MAX_MESSAGE_SIZE);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &addrSize);

        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            if (isMetricValid(buffer)) {
                enqueue(queues[RoundRobinCounter], buffer);
                RoundRobinCounter = (RoundRobinCounter + 1) % MAX_THREADS;
            } else {
                injectMetric("invalid_packets", 1);
                free(buffer); 
            }
        } else if (recvLen == 0) {
            free(buffer); 
        } else {
            free(buffer); 
        }
    }


    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
    close(sharedUdpSocket);
    close(udpSocket);

    return 0;
}