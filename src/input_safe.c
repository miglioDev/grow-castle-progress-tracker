#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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
