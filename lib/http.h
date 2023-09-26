#ifndef HTTP_H
#define HTTP_H

#include <netinet/in.h>

extern int HTTP_ENABLED;
extern int HTTP_PORT;
extern char HTTP_LISTEN_IP[16];

typedef struct {
    int port;
    char ip_address[16];  // For storing IPs like "255.255.255.255\0"
} HttpConfig;

void *http_server(void *config);

#endif
