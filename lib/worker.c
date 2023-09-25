#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>  // Include for time()
#include "queue.h"
#include "worker.h"
#include "logger.h"
#include "global.h"


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
    char thread_name[16]; // 15 characters + null terminator
    snprintf(thread_name, sizeof(thread_name), "Worker_%d", args->workerID);
    set_thread_name(thread_name);

    time_t last_packet_time = time(NULL);

    cloneDestAddr.sin_family = AF_INET;
    cloneDestAddr.sin_port = htons(CLONE_DEST_UDP_PORT);
    inet_aton(CLONE_DEST_UDP_IP, &cloneDestAddr.sin_addr);

    int error_counter = 0;
    time_t error_time = 0;
  
    write_log("Worker thread %d started", args->workerID);
    int current_packets = 0;
    int error_counter_pack = 0;
    time_t error_time_pack = 0;

    while (1) {
        char *buffer = dequeue(queue);
        if (buffer != NULL) {
            last_packet_time = time(NULL);
            time_t current_time_pack = time(NULL);
            if (error_counter_pack == 0 || difftime(current_time_pack, error_time_pack) >= 60) {
                if (difftime(current_time_pack, error_time_pack) >= 60) {
                    pthread_mutex_lock(&queue->mutex);
                    if(queue->currentSize > 1) {
                        char metric_name_queue[256];
                        snprintf(metric_name_queue, 256, "Worker-%d.QueueSize", args->workerID);
                        injectMetric(metric_name_queue, queue->currentSize);
                    }
                    pthread_mutex_unlock(&queue->mutex);
                    if (current_packets > 1) {
                        char metric_name[256];
                        snprintf(metric_name, 256, "Worker-%d.PacketsSent", args->workerID);
                        injectMetric(metric_name, current_packets);
                    }
                    error_counter_pack = 0;
                    current_packets = 0;

                }
                
                error_counter_pack++;
                if (error_counter_pack == 1) {
                    error_time_pack = current_time_pack;
                }
            }
            current_packets++;


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
                    char metric_name2[256];
                    snprintf(metric_name2, 256, "Worker-%d.PacketsDropped",current_packets);
                    injectMetric(metric_name2, current_packets);
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
                write_log("Exiting thread due to inactivity.\n");
                return 0;  // Exit the thread
            }
        }
    }
    return NULL;
}
