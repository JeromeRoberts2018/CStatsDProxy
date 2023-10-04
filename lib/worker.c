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

/**
 * @brief Worker thread function for a multi-threaded UDP packet handling application.
 * 
 * This function serves as the entry point for worker threads. It is responsible for
 * dequeuing packets from a unique queue for each thread and sending them via UDP. 
 * It also handles metrics collection and error tracking. The function runs 
 * indefinitely, but will exit if there are no packets to process for a specific 
 * time period.
 * 
 * @param arg A pointer to a WorkerArgs structure containing:
 *            - queue: Pointer to a Queue structure for packet storage.
 *            - udpSocket: File descriptor for the UDP socket.
 *            - destAddr: Destination address for the UDP packets.
 *            - workerID: An identifier for the worker thread, used for logging.
 *            - bufferSize: The size of the packet buffer.
 * 
 * @return NULL Always returns NULL, but can also exit the thread upon inactivity.
 *
 * @note This function uses several global constants and functions including:
 *       - CLONE_DEST_UDP_PORT
 *       - CLONE_DEST_UDP_IP
 *       - CLONE_ENABLED
 *       - set_thread_name()
 *       - write_log()
 *       - injectMetric()
 * 
 * I am only doing UDP, as I am staying true currently to the original etsy code.
 * if TCP is added, it will have it's own worker threads.
 */

void *worker_thread(void *arg) {
    // Cast the argument to its actual type.
    struct WorkerArgs *args = (struct WorkerArgs *)arg;

    // Initialize variables from the argument structure.
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;
    struct sockaddr_in cloneDestAddr;

    // Create and set the thread name for debugging and logging.
    char thread_name[16]; // 15 characters + null terminator
    snprintf(thread_name, sizeof(thread_name), "Worker_%d", args->workerID);
    set_thread_name(thread_name);

    // Initialize time variable for tracking last packet time.
    time_t last_packet_time = time(NULL);

    // Initialize variables for the destination address of the cloned packets.
    cloneDestAddr.sin_family = AF_INET;
    cloneDestAddr.sin_port = htons(CLONE_DEST_UDP_PORT);
    inet_aton(CLONE_DEST_UDP_IP, &cloneDestAddr.sin_addr);

    // Initialize error tracking variables.
    int error_counter = 0;
    time_t error_time = 0;

    // Log the start of the worker thread.
    write_log("Worker thread %d started", args->workerID);

    // Initialize counters for packets and errors.
    int current_packets = 0;
    int error_counter_pack = 0;
    time_t error_time_pack = 0;

    // Main loop to process incoming packets.
    while (1) {
        // Dequeue a packet from the unique queue.
        char *buffer = dequeue(queue);

        // If a packet is available.
        if (buffer != NULL) {
            // Update the last packet time.
            last_packet_time = time(NULL);

            // Error rate limiting and metric injection for packet rate.
            time_t current_time_pack = time(NULL);
            if (error_counter_pack == 0 || difftime(current_time_pack, error_time_pack) >= 60) {
                // Reset counters and inject metrics if applicable.
                if (difftime(current_time_pack, error_time_pack) >= 60) {
                    pthread_mutex_lock(&queue->mutex);
                    if (queue->currentSize > 1) {
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
                
                // Update the packet error counter and time.
                error_counter_pack++;
                if (error_counter_pack == 1) {
                    error_time_pack = current_time_pack;
                }
            }

            // Increment the packet counter.
            current_packets++;

            // Send the packet via UDP.
            ssize_t sentBytes = sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

            // If cloning is enabled, send the packet to the cloned destination.
            if (CLONE_ENABLED) {
                sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&cloneDestAddr, sizeof(cloneDestAddr));
            }

            // Handle send errors.
            if (sentBytes == -1) {
                time_t current_time = time(NULL);

                // Rate limiting and metric injection for packet drop errors.
                if (error_counter < 2 || difftime(current_time, error_time) >= 120) {
                    // Reset counters and inject metrics if applicable.
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
                    // I don't free here because it would cause segmentation fault
                    // I think it's because the buffer is not allocated with some of the bad packets..
                    // so this will cause memory leak, but I don't know how to fix it just yet.
                }
            } else {
                // Free the packet buffer if the send was successful.
                free(buffer);
            }
        } else {
            // Exit the thread if there has been no packet for 5 seconds.
            if (difftime(time(NULL), last_packet_time) >= 5) {
                write_log("Exiting thread due to inactivity.\n");
                return 0;  // Exit the thread
            }
        }
    }
    return NULL;
}

