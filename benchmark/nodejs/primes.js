#!/usr/bin/env node
// primes.js - Prime Sieve Benchmark for Node.js
// Tests memory allocation and CPU-bound computation

// Sieve of Eratosthenes
function sieveOfEratosthenes(limit) {
    const isPrime = new Array(limit + 1).fill(true);
    isPrime[0] = false;
    isPrime[1] = false;

    for (let i = 2; i * i <= limit; i++) {
        if (isPrime[i]) {
            for (let j = i * i; j <= limit; j += i) {
                isPrime[j] = false;
            }
        }
    }

    let count = 0;
    for (let i = 2; i <= limit; i++) {
        if (isPrime[i]) count++;
    }
    return count;
}

function main() {
    const limit = 1000000;

    const start = Date.now();
    const count = sieveOfEratosthenes(limit);
    const elapsed = Date.now() - start;

    console.log(`Primes up to ${limit}: ${count}`);
    console.log(`Sieve time: ${elapsed}ms`);
}

main();
