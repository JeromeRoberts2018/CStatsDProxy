#include "config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void trim(char *str) {
    if (str == NULL) {
        return;
    }
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    char *end = start + strlen(start) - 1;
    while (end >= start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
   memmove(str, start, end - start + 2);
}

int case_insensitive_compare(const char *str1, const char *str2) {
    if (!str1 || !str2) {
        return (str1 == str2) ? 0 : (str1 ? 1 : -1);
    }

    while (*str1 && *str2) {
        int c1 = tolower((unsigned char)*str1);
        int c2 = tolower((unsigned char)*str2);
        if (c1 != c2) {
            return c1 - c2;
        }
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}


int read_config(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || strlen(line) < 3) {
            continue;
        }

        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        trim(key);
        trim(value);

        if (value == NULL || key == NULL) {
            continue; // Skip lines with missing key or value
        }

        if (case_insensitive_compare(key, "UDP_PORT")) {
            UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "LISTEN_UDP_IP")) {
            strncpy(LISTEN_UDP_IP, value, sizeof(LISTEN_UDP_IP) - 1);
            LISTEN_UDP_IP[sizeof(LISTEN_UDP_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "DEST_UDP_PORT")) {
            DEST_UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "DEST_UDP_IP")) {
            strncpy(DEST_UDP_IP, value, sizeof(DEST_UDP_IP) - 1);
            DEST_UDP_IP[sizeof(DEST_UDP_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "MAX_MESSAGE_SIZE")) {
            MAX_MESSAGE_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "BUFFER_SIZE")) {
            BUFFER_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "MAX_THREADS")) {
            MAX_THREADS = atoi(value);
        } else if (case_insensitive_compare(key, "MAX_QUEUE_SIZE")) {
            MAX_QUEUE_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "LOGGING_INTERVAL")) {
            LOGGING_INTERVAL = atoi(value);
        } else if (case_insensitive_compare(key, "LOGGING_ENABLED")) {
            LOGGING_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_ENABLED")) {
            CLONE_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_DEST_UDP_PORT")) {
            CLONE_DEST_UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_DEST_UDP_IP")) {
            strncpy(CLONE_DEST_UDP_IP, value, sizeof(CLONE_DEST_UDP_IP) - 1);
            CLONE_DEST_UDP_IP[sizeof(CLONE_DEST_UDP_IP) - 1] = '\0'; // Ensure null-termination
        }
    }

    fclose(file);
    return 0;
}
