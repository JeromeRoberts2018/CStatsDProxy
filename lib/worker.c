#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"
#include "worker.h"

struct WorkerArgs {
    Queue *queue;
    int udpSocket;
    struct sockaddr_in destAddr;
    int workerID;
    int bufferSize;
};

void safeFree(void **pp) {
    if (*pp != NULL) {
        free(*pp);
        *pp = NULL;
    }
}


void *worker_thread(void *arg) {
    struct WorkerArgs *args = (struct WorkerArgs *)arg;
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;

    struct sockaddr_in cloneDestAddr;
    cloneDestAddr.sin_family = AF_INET;
    cloneDestAddr.sin_port = htons(CLONE_DEST_UDP_PORT);
    inet_aton(CLONE_DEST_UDP_IP, &cloneDestAddr.sin_addr);

    while (1) {
        char *buffer = dequeue(queue);
        if (buffer != NULL) {
            ssize_t sentBytes = sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

            if (sentBytes == -1) {
                char *statsd_metric = strdup("CStatsDProxy.logging_interval.packetslost:1|c");
                enqueue(queue, statsd_metric);
            } else {
                if (CLONE_ENABLED) {
                    sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&cloneDestAddr, sizeof(cloneDestAddr));
                }
            }
            safeFree((void**)&buffer);
        }
    }
    return NULL;
}