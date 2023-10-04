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
#include "http.h"
#include <sys/time.h>

char VERSION[] = "0.9.6.1";

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

int create_thread_with_retry(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg, int max_retries) {
    int retries = 0;
    int result;
    while (retries < max_retries) {
        result = pthread_create(thread, attr, start_routine, arg);
        if (result == 0) {
            return 1; // Successfully created the thread
        }
        retries++;
        fprintf(stderr, "Thread creation failed. Retrying %d/%d\n", retries, max_retries);
        sleep(1); // Wait for a short while before retrying
    }
    return 0; // Failed to create the thread after max_retries
}

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
                if (config.LOGGING_ENABLED) {
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
    if (read_config("conf/config.conf") == -1) {
        write_log("Failed to read configuration");
        return 1;
    }
    
    write_log("Starting CStatsDProxy server: Version %s\n", VERSION);

    if (config.LOGGING_ENABLED) {
        write_log("Starting server on %s:%d", config.LISTEN_UDP_IP, config.UDP_PORT);
        write_log("Forwarding to %s:%d", config.DEST_UDP_IP, config.DEST_UDP_PORT);
        if (config.CLONE_ENABLED) {
            write_log("Cloning to %s:%d", config.CLONE_DEST_UDP_IP, config.CLONE_DEST_UDP_PORT);
        }
    }

    HttpConfig conf;
    conf.port = config.HTTP_PORT;
    strncpy(conf.ip_address, config.HTTP_LISTEN_IP, sizeof(conf.ip_address));
    pthread_t http_thread;
    if (pthread_create(&http_thread, NULL, http_server, (void *)&config) < 0) {
        write_log("could not create http server thread");
        return 1;
    }

    struct sockaddr_in destAddr, serverAddr;
    int sharedUdpSocket = initialize_shared_udp_socket(config.DEST_UDP_IP, config.DEST_UDP_PORT, &destAddr);
    int udpSocket = initialize_listener_udp_socket(config.LISTEN_UDP_IP, config.UDP_PORT, &serverAddr);
    if (sharedUdpSocket == -1 || udpSocket == -1) {
        write_log("Failed to initialize sockets");
        return 1;
    } else {
        write_log("Sockets initialized");
    }

    struct timeval timeout;
    timeout.tv_sec = config.OUTBOUND_UDP_TIMEOUT;  // Use the defined constant
    timeout.tv_usec = 0;
    if (setsockopt(sharedUdpSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        write_log("setsockopt failed");
    }

    pthread_t threads[config.MAX_THREADS];
    struct WorkerArgs args[config.MAX_THREADS];
    queues = malloc(sizeof(Queue*) * config.MAX_QUEUE_SIZE);
    write_log("Starting %d worker threads", config.MAX_THREADS);
    for (int i = 0; i < config.MAX_THREADS; ++i) {
        queues[i] = initQueue(config.MAX_QUEUE_SIZE);
        args[i].queue = queues[i];
        args[i].udpSocket = sharedUdpSocket;
        args[i].destAddr = destAddr;
        args[i].workerID = i;
        args[i].bufferSize = config.BUFFER_SIZE;
        if (!create_thread_with_retry(&threads[i], NULL, worker_thread, &args[i], 10)) {
            fprintf(stderr, "Failed to create thread after multiple attempts. Exiting.\n");
            exit(EXIT_FAILURE);
        }
    }
    struct MonitorArgs monitorArgs = { threads, args, config.MAX_THREADS };
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_worker_threads, &monitorArgs);

    pthread_t requeueThread;
    if (init_requeue_thread(&requeueThread, config.MAX_THREADS, queues) != 0) {
        write_log("Failed to initialize requeue thread");
        return 1;
    }

    if (config.LOGGING_ENABLED) {
        write_log("Logging enabled");
    }
    
    int RoundRobinCounter = 0;
    while (1) {
        char *buffer = malloc(config.MAX_MESSAGE_SIZE);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, config.MAX_MESSAGE_SIZE - 1, 0, (struct sockaddr *)&clientAddr, &addrSize);

        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            if (isMetricValid(buffer)) {
                enqueue(queues[RoundRobinCounter], buffer);
                RoundRobinCounter = (RoundRobinCounter + 1) % config.MAX_THREADS;
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


    for (int i = 0; i < config.MAX_THREADS; ++i) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    pthread_cancel(monitor_thread);
    if (config.HTTP_ENABLED) {
        pthread_join(http_thread, NULL);
    }
    pthread_join(monitor_thread, NULL);
    close(sharedUdpSocket);
    close(udpSocket);

    return 0;
}