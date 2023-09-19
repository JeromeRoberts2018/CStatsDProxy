#include "config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void trim(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;
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
        trim(value);
        if (strcmp(key, "UDP_PORT") == 0) {
            UDP_PORT = atoi(value);
        } else if (strcmp(key, "LISTEN_UDP_IP") == 0) {
            strncpy(LISTEN_UDP_IP, value, sizeof(LISTEN_UDP_IP) - 1);
            LISTEN_UDP_IP[sizeof(LISTEN_UDP_IP) - 1] = '\0';  // Ensure null-termination
}        else if (strcmp(key, "DEST_UDP_PORT") == 0) {
            DEST_UDP_PORT = atoi(value);
        } else if (strcmp(key, "DEST_UDP_IP") == 0) {
            strncpy(DEST_UDP_IP, value, sizeof(DEST_UDP_IP) - 1);
            DEST_UDP_IP[sizeof(DEST_UDP_IP) - 1] = '\0';  // Ensure null-termination
        } else if (strcmp(key, "MAX_MESSAGE_SIZE") == 0) {
            MAX_MESSAGE_SIZE = atoi(value);
        } else if (strcmp(key, "BUFFER_SIZE") == 0) {
            BUFFER_SIZE = atoi(value);
        } else if (strcmp(key, "MAX_THREADS") == 0) {
            MAX_THREADS = atoi(value);
        } else if (strcmp(key, "MAX_QUEUE_SIZE") == 0) {
            MAX_QUEUE_SIZE = atoi(value);
        } else if (strcmp(key, "LOGGING_INTERVAL") == 0) {
            LOGGING_INTERVAL = atoi(value);
        } else if (strcmp(key, "LOGGING_ENABLED") == 0) {
            if (strcasecmp(value, "true") == 0) {
                LOGGING_ENABLED = 1;
            } else {
                LOGGING_ENABLED = 0;
            }
        } else if (strcmp(key, "LOGGING_FILE_NAME") == 0) {
            strncpy(LOGGING_FILE_NAME, value, sizeof(LOGGING_FILE_NAME) - 1);
            LOGGING_FILE_NAME[sizeof(LOGGING_FILE_NAME) - 1] = '\0';  // Ensure null-termination
        }
    }

    fclose(file);
    return 0;
}
