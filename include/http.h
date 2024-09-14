#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

typedef void (*server_ready_callback_t)(void);

typedef struct {
    int port;
    char ip_address[16];  // For storing IPs like "255.255.255.255\0"
    server_ready_callback_t callback; // for function tests
} HttpConfig;

void *http_server(void *arg);

#endif
