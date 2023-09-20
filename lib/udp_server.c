#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include "udp_server.h"
#include "logger.h"
#include <arpa/inet.h>

// Initialize a shared UDP socket for sending data
int initialize_shared_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name) {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        write_log(logging_file_name, "Socket creation failed");
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

void udp_server_loop(int udpSocket, Queue **queues, int max_threads, int max_message_size, int logging_enabled, const char *logging_file_name, pthread_mutex_t *packet_counter_mutex, long long unsigned int *packet_counter) {
    int RoundRobinCounter = 0;
    while (1) {
        char *buffer = malloc(max_message_size);
        struct sockaddr_in clientAddr;
        socklen_t addrSize = sizeof(clientAddr);
        ssize_t recvLen = recvfrom(udpSocket, buffer, max_message_size - 1, 0, (struct sockaddr *)&clientAddr, &addrSize);

        if (recvLen > 0) {
            buffer[recvLen] = '\0';
            enqueue(queues[RoundRobinCounter], buffer);
            RoundRobinCounter = (RoundRobinCounter + 1) % max_threads;

            if (logging_enabled) {
                pthread_mutex_lock(packet_counter_mutex);
                (*packet_counter)++;
                pthread_mutex_unlock(packet_counter_mutex);
            }
        } else if (recvLen == 0) {
            if (logging_enabled) { write_log(logging_file_name, "Received zero bytes. Connection closed or terminated."); }
            free(buffer);
        } else {
            if (logging_enabled) { write_log(logging_file_name, "recvfrom() returned an error: %zd", recvLen); }
            free(buffer);
        }
    }
}
