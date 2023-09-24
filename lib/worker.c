#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>  // Include for time()
#include "queue.h"
#include "worker.h"

extern int CLONE_ENABLED;
extern int CLONE_DEST_UDP_PORT;
extern char CLONE_DEST_UDP_IP[];
extern Queue **queues;

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
    struct sockaddr_in cloneDestAddr;

    time_t last_packet_time = time(NULL);

    cloneDestAddr.sin_family = AF_INET;
    cloneDestAddr.sin_port = htons(CLONE_DEST_UDP_PORT);
    inet_aton(CLONE_DEST_UDP_IP, &cloneDestAddr.sin_addr);

    int error_counter = 0;
    time_t error_time = 0;
  

    while (1) {
        char *buffer = dequeue(queue);
        if (buffer != NULL) {
            last_packet_time = time(NULL);

            ssize_t sentBytes = sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
            if (CLONE_ENABLED) {
                sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&cloneDestAddr, sizeof(cloneDestAddr));
            }
            if (sentBytes == -1) {
                time_t current_time = time(NULL);
            
                if (error_counter < 2 || difftime(current_time, error_time) >= 120) {
                    if (difftime(current_time, error_time) >= 120) {
                        error_counter = 0;
                        error_time = 0;
                    }
                    char *statsd_metric = malloc(256);
                    snprintf(statsd_metric, 256, "CStatsDProxy.logging_interval.packetsdropped:1|c");
                    printf("Error sending packet to %s:%d\n", inet_ntoa(destAddr.sin_addr), ntohs(destAddr.sin_port));
                    enqueue(queues[1], statsd_metric);
                    error_counter++;
                    if (error_counter == 1) {
                        error_time = current_time;
                    }
                }
            
            } else {
                free(buffer);
            }
        } else {
            if (difftime(time(NULL), last_packet_time) >= 5) {
                printf("Exiting thread due to inactivity.\n");
                return 0;  // Exit the thread
            }
        }
    }
    return NULL;
}
