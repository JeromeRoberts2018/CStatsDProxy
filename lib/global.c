#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <processthreadsapi.h>
#elif defined(__APPLE__) || defined(__MACH__) || defined(__linux__)
    #define _GNU_SOURCE
    #include <pthread.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include "global.h"
#include <string.h>
#include <stdbool.h>


void set_thread_name(const char *thread_name) {
#if defined(_WIN32) || defined(_WIN64)
    wchar_t wtext[20];
    mbstowcs(wtext, thread_name, strlen(thread_name) + 1);
    HRESULT hr = SetThreadDescription(GetCurrentThread(), wtext);
    if (FAILED(hr)) {
        // Handle error here
    }
#elif defined(__APPLE__) || defined(__MACH__)
    if (pthread_setname_np(thread_name) != 0) {
        perror("pthread_setname_np");
    }
#elif defined(__linux__)
    if (pthread_setname_np(pthread_self(), thread_name) != 0) {
        perror("pthread_setname_np");
    }
#else
    #warning "Unknown platform. Cannot set thread name."
#endif
}

void injectMetric(const char *metricName, int metricValue) {
    char *statsd_metric = malloc(256);
    if (statsd_metric != NULL) {
        snprintf(statsd_metric, 256, "CStatsDProxy.metrics.%s:%d|c", metricName, metricValue);
        enqueue(requeue, statsd_metric);
    }
}

void injectPacket(const char *packet) {
        enqueue(requeue, packet);
}

bool isMetricValid(const char *metric) {
    if (strlen(metric) >= 500) {
        return false;  // Too long
    }

    const char *valid_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.:|-_@";
    
    for (int i = 0; i < strlen(metric); ++i) {
        if (strchr(valid_chars, metric[i]) == NULL) {
            return false;  // Invalid character found
        }
    }

    return true;
}

bool is_safe_string(const char *str) {
    // Define a whitelist of safe characters.
    // You may need to adjust this list depending on your specific needs.
    const char *whitelist = "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789"
                            " .,;:!i?'\"()-%";
    
    // Loop through each character in the input string
    for (int i = 0; i < strlen(str); i++) {
        // Check if the character is NOT in the whitelist
        if (strchr(whitelist, str[i]) == NULL) {
            return false;
        }
    }

    // All characters are in the whitelist, so the string is safe
    return true;
}