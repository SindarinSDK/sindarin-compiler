/*
 * arrays.c - Array Operations Benchmark for C
 * Tests dynamic array operations and iteration
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/* Get current time in milliseconds */
long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Reverse array in-place */
void reverse_array(int *arr, int size) {
    for (int i = 0; i < size / 2; i++) {
        int temp = arr[i];
        arr[i] = arr[size - 1 - i];
        arr[size - 1 - i] = temp;
    }
}

/* Sum all elements in array */
int64_t sum_array(int *arr, int size) {
    int64_t sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

int main(void) {
    int size = 1000000;
    long start, end;

    start = get_time_ms();

    /* Create array with 1,000,000 integers (0 to 999,999) */
    int *arr = malloc(size * sizeof(int));
    if (!arr) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    for (int i = 0; i < size; i++) {
        arr[i] = i;
    }

    /* Sum all elements */
    int64_t sum1 = sum_array(arr, size);

    /* Reverse the array in-place */
    reverse_array(arr, size);

    /* Sum again to verify */
    int64_t sum2 = sum_array(arr, size);

    end = get_time_ms();

    printf("Sum: %ld\n", (long)sum1);
    printf("Reversed sum: %ld\n", (long)sum2);
    printf("Array time: %ldms\n", end - start);

    free(arr);
    return 0;
}
