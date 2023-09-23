#ifndef ATOMIC_H
#define ATOMIC_H

#include <stdatomic.h>
#include <stddef.h>

typedef struct {
    char packet_data[256];  // Mock packet data
} Packet;

typedef struct atmNode {
    Packet data;
    struct atmNode *next;
} atmNode;

typedef struct {
    atmNode *_Atomic head;
    atmNode *_Atomic tail;
} AtomicQueue;

void atmInitQueue(AtomicQueue *q);
int atmEnqueue(AtomicQueue *q, const char *buffer);
int atmDequeue(AtomicQueue *q, Packet *outPacket);
int atmQueueSize(AtomicQueue *q);

#endif // ATOMIC_H
