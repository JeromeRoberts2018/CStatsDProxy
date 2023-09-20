#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <netinet/in.h>
#include "queue.h"

int initialize_udp_socket(const char *ip, int port, struct sockaddr_in *address, const char *logging_file_name);
void udp_server_loop(int udpSocket, Queue **queues, int max_threads, int max_message_size, int logging_enabled, const char *logging_file_name, pthread_mutex_t *packet_counter_mutex, long long unsigned int *packet_counter);

#endif // UDP_SERVER_H