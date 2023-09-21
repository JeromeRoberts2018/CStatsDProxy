
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "queue.h"

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
void fast_tokenize(char *packet, char **key, char **valueStr, char **type);


/**
 * Serves as a worker thread for processing and sending UDP packets.
 * This function is designed to run as a separate thread and processes data from a queue to send via UDP.
 *
 * The function fetches arguments from a WorkerArgs structure and executes the following logic in an infinite loop:
 * - It dequeues up to `processBatchSize` items from the queue.
 * - Accumulates statistics based on the dequeued items.
 * - Sends the statistics via UDP.
 * 
 * The function performs the following actions for each item in the batch:
 * - Tokenizes the item to extract key, value, and type.
 * - Updates or inserts the key-value-type triplet in an array of Stats structures.
 * - Frees the memory of the processed item.
 *
 * After processing the batch, the function sends the accumulated statistics via UDP.
 *
 * The function uses mutexes to ensure thread-safe dequeue operations and uses a UDP socket for sending data.
 * 
 * @param arg A pointer to a WorkerArgs structure containing the queue, UDP socket, destination address, and buffer size.
 * @return This function returns NULL as it is designed to run indefinitely.
 */
void *worker_thread(void *arg) {
    struct WorkerArgs *args = (struct WorkerArgs *)arg;
    Queue *queue = args->queue;
    int udpSocket = args->udpSocket;
    struct sockaddr_in destAddr = args->destAddr;

    while (1) {
        char *buffer = dequeue(queue);
        if (buffer != NULL) {
            ssize_t sentBytes = sendto(udpSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));

            // If sending fails, re-queue the buffer to be sent later
            if (sentBytes == -1) {
                enqueue(queue, buffer);  // Re-queue the message to be sent later
            } else {
                free(buffer);
            }
        }
    }



    return NULL;
}

/**
 * Quickly tokenizes a packet string into key, value, and type components.
 *
 * This function uses in-place modification of the input string for performance.
 * It updates the provided pointers to point to the relevant sub-strings within the packet string.
 * The function is designed for a specific format: "key:value|type". 
 * Any other format may result in undefined behavior.
 *
 * @param packet   The input string to tokenize. Will be modified in-place.
 * @param key      Pointer to store the starting address of the 'key' part in the packet.
 * @param valueStr Pointer to store the starting address of the 'value' part in the packet.
 * @param type     Pointer to store the starting address of the 'type' part in the packet.
 *
 * Examples:
 * Input:  "cpu:100|gauge", Output: key="cpu", valueStr="100", type="gauge"
 * Input:  "mem:2048|counter", Output: key="mem", valueStr="2048", type="counter"
 *
 * Note: Function does not return any value but modifies the pointers.
 */
void fast_tokenize(char *packet, char **key, char **valueStr, char **type) {
    char *cursor;

    cursor = strchr(packet, ':');
    if (cursor == NULL) return;
    *cursor = '\0';
    *key = packet;

    *valueStr = cursor + 1;
    cursor = strchr(*valueStr, '|');
    if (cursor == NULL) return;
    *cursor = '\0';

    *type = cursor + 1;
    cursor = strchr(*type, '|');
    if (cursor != NULL) {
        *cursor = '\0';
    }
}