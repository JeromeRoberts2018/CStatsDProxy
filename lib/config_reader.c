#include "config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void trim(char *str) {
    if (str == NULL) {
        return;
    }

    char *end;
    while (isspace((unsigned char)*str)) str++;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}

int case_insensitive_compare(const char *str1, const char *str2) {
    if (!str1 || !str2) {
        return 0;
    }

    while (*str1 && *str2) {
        if (tolower((unsigned char)*str1) != tolower((unsigned char)*str2)) {
            return 0;
        }
        str1++;
        str2++;
    }

    return (*str1 == '\0' && *str2 == '\0');
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
        } else if (case_insensitive_compare(key, "HTTP_ENABLED")) {
            HTTP_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "HTTP_PORT")) {
            HTTP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "HTTP_LISTEN_IP")) {
            strncpy(HTTP_LISTEN_IP, value, sizeof(HTTP_LISTEN_IP) - 1);
            HTTP_LISTEN_IP[sizeof(HTTP_LISTEN_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "OUTBOUND_UDP_TIMEOUT")) {
            OUTBOUND_UDP_TIMEOUT = atoi(value);
        }
    }

    fclose(file);
    return 0;
}
