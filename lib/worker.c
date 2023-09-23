#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "atomic.h"
#include "worker.h"

struct WorkerArgs {
    AtomicQueue *queue;  // Update to AtomicQueue
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
    AtomicQueue *queue = args->queue;  // Update to AtomicQueue
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;

    struct sockaddr_in cloneDestAddr;
    cloneDestAddr.sin_family = AF_INET;
    cloneDestAddr.sin_port = htons(CLONE_DEST_UDP_PORT);
    inet_aton(CLONE_DEST_UDP_IP, &cloneDestAddr.sin_addr);


    Packet packet;  // Use Packet struct for handling data
    int cloneUdpSocket = -1;
    if (CLONE_ENABLED) {
        cloneUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (cloneUdpSocket < 0) {
            perror("Could not create clone socket");
            exit(1);
        }
    }

    while (1) {
        if (atmDequeue(queue, &packet)) {
            ssize_t sentBytes = sendto(udpSocket, packet.packet_data, strlen(packet.packet_data), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
            if (sentBytes == -1) {
                atmEnqueue(queue, "CStatsDProxy.logging_interval.packetslost:1|c"); 
            } else {
                if (CLONE_ENABLED) {
                    sendto(cloneUdpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&cloneDestAddr, sizeof(cloneDestAddr));
                }
            }
        }
    }

    if (cloneUdpSocket != -1) {
        close(cloneUdpSocket);
    }

    return NULL;
}
