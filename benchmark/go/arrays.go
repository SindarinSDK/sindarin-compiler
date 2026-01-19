// arrays.go - Array Operations Benchmark for Go
// Tests dynamic array operations and iteration
package main

import (
	"fmt"
	"time"
)

func main() {
	size := 1000000

	start := time.Now()

	// Create array with 1,000,000 integers (0 to 999,999)
	arr := make([]int, size)
	for i := 0; i < size; i++ {
		arr[i] = i
	}

	// Sum all elements
	var sum1 int64 = 0
	for i := 0; i < size; i++ {
		sum1 += int64(arr[i])
	}

	// Reverse the array in-place
	for i := 0; i < size/2; i++ {
		arr[i], arr[size-1-i] = arr[size-1-i], arr[i]
	}

	// Sum again to verify
	var sum2 int64 = 0
	for i := 0; i < size; i++ {
		sum2 += int64(arr[i])
	}

	elapsed := time.Since(start).Milliseconds()

	fmt.Printf("Sum: %d\n", sum1)
	fmt.Printf("Reversed sum: %d\n", sum2)
	fmt.Printf("Array time: %dms\n", elapsed)
}
