/*
 * strings.c - String Operations Benchmark for C
 * Tests string concatenation and manipulation performance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Get current time in milliseconds */
long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Count occurrences of a substring in a string */
int count_occurrences(const char *str, const char *substr) {
    int count = 0;
    size_t substr_len = strlen(substr);
    const char *pos = str;

    while ((pos = strstr(pos, substr)) != NULL) {
        count++;
        pos += 1;  /* Move forward to find overlapping matches */
    }

    return count;
}

int main(void) {
    int count = 100000;
    const char *hello = "Hello";
    size_t hello_len = strlen(hello);
    size_t total_len = count * hello_len;
    long start, end;

    start = get_time_ms();

    /* Allocate buffer for the full string */
    char *result = malloc(total_len + 1);
    if (!result) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    /* Build string by concatenating "Hello" 100,000 times */
    char *ptr = result;
    for (int i = 0; i < count; i++) {
        memcpy(ptr, hello, hello_len);
        ptr += hello_len;
    }
    *ptr = '\0';

    /* Count occurrences of 'llo' */
    int occurrences = count_occurrences(result, "llo");

    end = get_time_ms();

    printf("String length: %zu\n", strlen(result));
    printf("Occurrences of 'llo': %d\n", occurrences);
    printf("String time: %ldms\n", end - start);

    free(result);
    return 0;
}
