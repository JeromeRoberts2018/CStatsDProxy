#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <netinet/in.h>
#include "queue.h"

// Initialize a shared UDP socket for sending data
int initialize_shared_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name);

// Initialize a UDP socket for listening
int initialize_listener_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name);

// Main loop function for UDP server
void udp_server_loop(int udpSocket, Queue **queues, int max_threads, int max_message_size, int logging_enabled, const char *logging_file_name, pthread_mutex_t *packet_counter_mutex, unsigned long long *packet_counter);

#endif // UDP_SERVER_H
