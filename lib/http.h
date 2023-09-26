#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

typedef struct {
    int port;
    char ip_address[16];  // For storing IPs like "255.255.255.255\0"
} HttpConfig;

void *http_server(void *config);

#endif
