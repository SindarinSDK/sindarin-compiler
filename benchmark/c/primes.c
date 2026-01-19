/*
 * primes.c - Prime Sieve Benchmark for C
 * Tests memory allocation and CPU-bound computation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/* Sieve of Eratosthenes */
int sieve_of_eratosthenes(int limit) {
    bool *is_prime = malloc(limit + 1);
    if (!is_prime) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(1);
    }

    /* Initialize all as potentially prime */
    for (int i = 0; i <= limit; i++) {
        is_prime[i] = true;
    }
    is_prime[0] = is_prime[1] = false;

    /* Sieve */
    for (int i = 2; i * i <= limit; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= limit; j += i) {
                is_prime[j] = false;
            }
        }
    }

    /* Count primes */
    int count = 0;
    for (int i = 2; i <= limit; i++) {
        if (is_prime[i]) count++;
    }

    free(is_prime);
    return count;
}

/* Get current time in milliseconds */
long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int main(void) {
    int limit = 1000000;
    long start, end;

    start = get_time_ms();
    int count = sieve_of_eratosthenes(limit);
    end = get_time_ms();

    printf("Primes up to %d: %d\n", limit, count);
    printf("Sieve time: %ldms\n", end - start);

    return 0;
}
