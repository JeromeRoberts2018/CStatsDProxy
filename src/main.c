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

unsigned long long int packet_counter = 0;
pthread_mutex_t packet_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
Queue **queues = NULL;

struct WorkerArgs {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int workerID;
    int bufferSize;
};

void *logging_thread(void *arg);

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

void *udp_listener_thread(void *arg) {
    int udpSocket = *(int*)arg;
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
            if (LOGGING_ENABLED) { write_log("Received zero bytes. Connection closed or terminated."); }
        } else {
            if (LOGGING_ENABLED) { write_log("recvfrom() returned an error: %zd", recvLen); }
        }
    }

    return NULL;
}

void *udp_activity_monitor(void *arg) {
    int udpSocket = *(int*)arg;
    fd_set read_fds;
    struct timeval timeout;
    pthread_t udp_listener_tid;

    pthread_create(&udp_listener_tid, NULL, udp_listener_thread, &udpSocket);
    sleep(1);  // Wait for the listener thread to start
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(udpSocket, &read_fds);

        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int activity = select(udpSocket + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0) {
            write_log("select error");
        } else if (activity == 0) {
            write_log("UDP Listener inactive for %d second, restarting thread...",timeout.tv_sec );

            pthread_cancel(udp_listener_tid);  // Cancel the old listener thread
            pthread_join(udp_listener_tid, NULL);  // Wait for it to finish

            pthread_create(&udp_listener_tid, NULL, udp_listener_thread, &udpSocket);  // Create a new listener thread
        }
    }

    return NULL;
}


int main() {
    printf("Starting CStatsDProxy server...\n");

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
    pthread_t udp_monitor_thread;
    pthread_create(&udp_monitor_thread, NULL, udp_activity_monitor, &udpSocket);

    pthread_t threads[MAX_THREADS];
    struct WorkerArgs args[MAX_THREADS];
    queues = malloc(sizeof(Queue*) * MAX_QUEUE_SIZE);
    write_log("Starting %d worker threads", MAX_THREADS);
    for (int i = 0; i < MAX_THREADS; ++i) {
        queues[i] = initQueue(MAX_QUEUE_SIZE);
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


    if (LOGGING_ENABLED) {
        write_log("Logging enabled");
        pthread_t log_thread;
        pthread_create(&log_thread, NULL, logging_thread, args);
    }
    
    while (1) {
        sleep(5); // idle main thread
    }


    for (int i = 0; i < MAX_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    pthread_cancel(monitor_thread);
    pthread_join(monitor_thread, NULL);
    pthread_cancel(udp_monitor_thread);
    pthread_join(udp_monitor_thread, NULL);
    close(sharedUdpSocket);
    close(udpSocket);

    return 0;
}

void *logging_thread(void *arg) {
    struct WorkerArgs *workerArgs = (struct WorkerArgs *) arg;
    int numWorkers = MAX_THREADS;

    while (1) {
        sleep(LOGGING_INTERVAL);

        for (int i = 0; i < numWorkers; ++i) {
            Queue *queue = workerArgs[i].queue;
            pthread_mutex_lock(&queue->mutex);
            if (queue->currentSize > 0) { 
                write_log("WorkerID: %d, QueueSize: %d", workerArgs[i].workerID, queue->currentSize);
                char *statsd_worker = malloc(256);
                snprintf(statsd_worker, 256, "CStatsDProxy.logging_interval.worker%d:%d|c", workerArgs[i].workerID, queue->currentSize);
                enqueue(queues[1], statsd_worker);
            }
            pthread_mutex_unlock(&queue->mutex);
        }
        
        pthread_mutex_lock(&packet_counter_mutex);
        if (packet_counter > 0) {
            write_log("Packets Since Last Logging: %llu", packet_counter);
            char *statsd_metric = malloc(256); // Allocate enough space for the metric
            snprintf(statsd_metric, 256, "CStatsDProxy.logging_interval.packetsreceived:%llu|c", packet_counter);
            enqueue(queues[1], statsd_metric);
        }        
        packet_counter = 0;
        pthread_mutex_unlock(&packet_counter_mutex);

    }

    return NULL;
}
