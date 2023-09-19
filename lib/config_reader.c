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

/**
 * Reads and parses a configuration file to set various application parameters.
 * The function expects the configuration file to have key-value pairs separated by an '=' symbol.
 * Each line should represent a single setting and should be in the form "KEY=VALUE".
 * Comments can be included in the file with lines starting with a '#'.
 *
 * Supported keys and their respective variables:
 * - UDP_PORT: Sets the variable UDP_PORT to the integer value.
 * - LISTEN_UDP_IP: Sets the string LISTEN_UDP_IP.
 * - DEST_UDP_PORT: Sets the variable DEST_UDP_PORT to the integer value.
 * - DEST_UDP_IP: Sets the string DEST_UDP_IP.
 * - MAX_MESSAGE_SIZE: Sets the variable MAX_MESSAGE_SIZE to the integer value.
 * - BUFFER_SIZE: Sets the variable BUFFER_SIZE to the integer value.
 * - MAX_THREADS: Sets the variable MAX_THREADS to the integer value.
 * - MAX_QUEUE_SIZE: Sets the variable MAX_QUEUE_SIZE to the integer value.
 * - LOGGING_INTERVAL: Sets the variable LOGGING_INTERVAL to the integer value.
 * - LOGGING_ENABLED: Sets the variable LOGGING_ENABLED to the integer value.
 * - LOGGING_FILE_NAME: Sets the string LOGGING_FILE_NAME.
 *
 * @param filepath The path to the configuration file.
 * @return Returns 0 on successful parsing and setting of all keys. Returns -1 if the file could not be opened.
 */
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
            LOGGING_ENABLED = atoi(value);
        } else if (strcmp(key, "LOGGING_FILE_NAME") == 0) {
            strncpy(LOGGING_FILE_NAME, value, sizeof(LOGGING_FILE_NAME) - 1);
            LOGGING_FILE_NAME[sizeof(LOGGING_FILE_NAME) - 1] = '\0';  // Ensure null-termination
        }
    }

    fclose(file);
    return 0;
}
