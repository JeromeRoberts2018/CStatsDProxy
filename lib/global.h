#ifndef THREAD_UTILS_H
#define THREAD_UTILS_H

#include "queue.h" 
#include "logger.h"
#include "requeue.h"
#include <stdbool.h>


extern Queue *requeue;

void set_thread_name(const char *thread_name);
void injectMetric(const char *metricName, int metricValue);
bool isMetricValid(const char *metric);
bool is_safe_string(const char *str);
void injectPacket(const char *packet);


#endif // THREAD_UTILS_H
