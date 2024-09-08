/**
 * @file config_reader.c
 * @brief Implementation of functions to read configuration from a file.
 * 
 * This file contains the implementation of the read_config function, which reads configuration
 * from a file and stores it in a Config struct. The file must be in the format of key-value pairs
 * separated by an equal sign (=). Lines starting with '#' are ignored. The function returns 0 on
 * success and -1 on failure.
 */
#include "../include/config_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

Config config;

static void trim(char *str) {
    if (str == NULL) {
        return;
    }

    char *end;
    while (isspace((unsigned char)*str)) str++;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}

static int case_insensitive_compare(const char *str1, const char *str2) {
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

/**
 * Reads configuration values from a file and sets them in the global `config` struct.
 * 
 * @param filepath The path to the configuration file.
 * @return Returns 0 on success, -1 if the file could not be opened.
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

        char *saveptr;  // Pointer to keep track of tokenization state
        char *key = strtok_r(line, "=", &saveptr);  // key = "LISTEN_UDP_IP"
        char *value = strtok_r(NULL, "=", &saveptr);  // value = "127.0.0.1"

        trim(key);
        trim(value);

        if (value == NULL || key == NULL) {
            continue; // Skip lines with missing key or value
        }

        if (case_insensitive_compare(key, "UDP_PORT")) {
            config.UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "LISTEN_UDP_IP")) {
            strncpy(config.LISTEN_UDP_IP, value, sizeof(config.LISTEN_UDP_IP) - 1);
            config.LISTEN_UDP_IP[sizeof(config.LISTEN_UDP_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "DEST_UDP_PORT")) {
            config.DEST_UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "DEST_UDP_IP")) {
            strncpy(config.DEST_UDP_IP, value, sizeof(config.DEST_UDP_IP) - 1);
            config.DEST_UDP_IP[sizeof(config.DEST_UDP_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "MAX_MESSAGE_SIZE")) {
            config.MAX_MESSAGE_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "BUFFER_SIZE")) {
            config.BUFFER_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "MAX_THREADS")) {
            config.MAX_THREADS = atoi(value);
        } else if (case_insensitive_compare(key, "MAX_QUEUE_SIZE")) {
            config.MAX_QUEUE_SIZE = atoi(value);
        } else if (case_insensitive_compare(key, "LOGGING_INTERVAL")) {
            config.LOGGING_INTERVAL = atoi(value);
        } else if (case_insensitive_compare(key, "LOGGING_ENABLED")) {
            config.LOGGING_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_ENABLED")) {
            config.CLONE_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_DEST_UDP_PORT")) {
            config.CLONE_DEST_UDP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "CLONE_DEST_UDP_IP")) {
            strncpy(config.CLONE_DEST_UDP_IP, value, sizeof(config.CLONE_DEST_UDP_IP) - 1);
            config.CLONE_DEST_UDP_IP[sizeof(config.CLONE_DEST_UDP_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "HTTP_ENABLED")) {
            config.HTTP_ENABLED = atoi(value);
        } else if (case_insensitive_compare(key, "HTTP_PORT")) {
            config.HTTP_PORT = atoi(value);
        } else if (case_insensitive_compare(key, "HTTP_LISTEN_IP")) {
            strncpy(config.HTTP_LISTEN_IP, value, sizeof(config.HTTP_LISTEN_IP) - 1);
            config.HTTP_LISTEN_IP[sizeof(config.HTTP_LISTEN_IP) - 1] = '\0'; // Ensure null-termination
        } else if (case_insensitive_compare(key, "OUTBOUND_UDP_TIMEOUT")) {
            config.OUTBOUND_UDP_TIMEOUT = atoi(value);
        }
    }

    fclose(file);
    return 0;
}
