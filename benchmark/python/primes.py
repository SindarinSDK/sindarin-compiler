#!/usr/bin/env python3
# primes.py - Prime Sieve Benchmark for Python
# Tests memory allocation and CPU-bound computation

import time

def sieve_of_eratosthenes(limit):
    """Sieve of Eratosthenes"""
    is_prime = [True] * (limit + 1)
    is_prime[0] = False
    is_prime[1] = False

    i = 2
    while i * i <= limit:
        if is_prime[i]:
            j = i * i
            while j <= limit:
                is_prime[j] = False
                j += i
        i += 1

    return sum(is_prime)

def main():
    limit = 1000000

    start = time.time()
    count = sieve_of_eratosthenes(limit)
    elapsed = int((time.time() - start) * 1000)

    print(f"Primes up to {limit}: {count}")
    print(f"Sieve time: {elapsed}ms")

if __name__ == "__main__":
    main()
