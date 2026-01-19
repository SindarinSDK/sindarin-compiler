// primes.go - Prime Sieve Benchmark for Go
// Tests memory allocation and CPU-bound computation
package main

import (
	"fmt"
	"time"
)

// Sieve of Eratosthenes
func sieveOfEratosthenes(limit int) int {
	isPrime := make([]bool, limit+1)
	for i := range isPrime {
		isPrime[i] = true
	}
	isPrime[0] = false
	isPrime[1] = false

	for i := 2; i*i <= limit; i++ {
		if isPrime[i] {
			for j := i * i; j <= limit; j += i {
				isPrime[j] = false
			}
		}
	}

	count := 0
	for i := 2; i <= limit; i++ {
		if isPrime[i] {
			count++
		}
	}
	return count
}

func main() {
	limit := 1000000

	start := time.Now()
	count := sieveOfEratosthenes(limit)
	elapsed := time.Since(start).Milliseconds()

	fmt.Printf("Primes up to %d: %d\n", limit, count)
	fmt.Printf("Sieve time: %dms\n", elapsed)
}
