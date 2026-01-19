// fib.go - Fibonacci Benchmark for Go
// Tests function call overhead (recursive) and loop performance (iterative)
package main

import (
	"fmt"
	"time"
)

// Recursive Fibonacci - tests function call overhead
func fibRecursive(n int) int64 {
	if n <= 1 {
		return int64(n)
	}
	return fibRecursive(n-1) + fibRecursive(n-2)
}

// Iterative Fibonacci - tests loop performance
func fibIterative(n int) int64 {
	if n <= 1 {
		return int64(n)
	}
	var a, b int64 = 0, 1
	for i := 2; i <= n; i++ {
		a, b = b, a+b
	}
	return b
}

func main() {
	// Recursive fib(35)
	start := time.Now()
	result := fibRecursive(35)
	elapsed := time.Since(start).Milliseconds()
	fmt.Printf("Recursive fib(35) = %d\n", result)
	fmt.Printf("Recursive time: %dms\n", elapsed)

	// Iterative fib(50)
	start = time.Now()
	result = fibIterative(50)
	elapsed = time.Since(start).Milliseconds()
	fmt.Printf("Iterative fib(50) = %d\n", result)
	fmt.Printf("Iterative time: %dms\n", elapsed)
}
