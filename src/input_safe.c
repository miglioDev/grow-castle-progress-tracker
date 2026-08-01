#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../include/input_safe.h"

int safe_input_int(const char *prompt, int *result, int min_val, int max_val) {
    char buffer[64];
    char *endptr;
    long val;

    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }

    errno = 0;
    val = strtol(buffer, &endptr, 10);

    if (errno != 0 || endptr == buffer || val < min_val || val > max_val) {
        return 0;
    }

    *result = (int)val;
    return 1;
}

int safe_input_long_long(const char *prompt, long long *result, long long min_val, long long max_val) {
    char buffer[64];
    char *endptr;
    long long val;

    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }

    errno = 0;
    val = strtoll(buffer, &endptr, 10);

    if (errno != 0 || endptr == buffer || val < min_val || val > max_val) {
        return 0;
    }

    *result = val;
    return 1;
}

int safe_input_float(const char *prompt, float *result, float min_val, float max_val) {
    char buffer[64];
    char *endptr;
    float val;

    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }

    errno = 0;
    val = strtof(buffer, &endptr);

    if (errno != 0 || endptr == buffer || val < min_val || val > max_val) {
        return 0;
    }

    *result = val;
    return 1;
}

int safe_input_string(const char *prompt, char *result, size_t size) {
    char buffer[256];

    printf("%s", prompt);
    if (!fgets(buffer, sizeof(buffer), stdin)) {
        return 0;
    }

    buffer[strcspn(buffer, "\r\n")] = '\0';
    if (strlen(buffer) == 0) {
        return 0;
    }

    snprintf(result, size, "%s", buffer);
    return 1;
}
