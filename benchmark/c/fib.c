/*
 * fib.c - Fibonacci Benchmark for C
 * Tests function call overhead (recursive) and loop performance (iterative)
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

/* Recursive Fibonacci - tests function call overhead */
int64_t fib_recursive(int n) {
    if (n <= 1) return n;
    return fib_recursive(n - 1) + fib_recursive(n - 2);
}

/* Iterative Fibonacci - tests loop performance */
int64_t fib_iterative(int n) {
    if (n <= 1) return n;
    int64_t a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int64_t temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

/* Get current time in milliseconds */
long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(void) {
    long start, end;
    int64_t result;

    /* Recursive fib(35) */
    start = get_time_ms();
    result = fib_recursive(35);
    end = get_time_ms();
    printf("Recursive fib(35) = %ld\n", (long)result);
    printf("Recursive time: %ldms\n", end - start);

    /* Iterative fib(50) */
    start = get_time_ms();
    result = fib_iterative(50);
    end = get_time_ms();
    printf("Iterative fib(50) = %ld\n", (long)result);
    printf("Iterative time: %ldms\n", end - start);

    return 0;
}
