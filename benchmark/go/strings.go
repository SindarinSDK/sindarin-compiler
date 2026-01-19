// strings.go - String Operations Benchmark for Go
// Tests string concatenation and manipulation performance
package main

import (
	"fmt"
	"strings"
	"time"
)

func countOccurrences(s, substr string) int {
	count := 0
	pos := 0
	for {
		idx := strings.Index(s[pos:], substr)
		if idx == -1 {
			break
		}
		count++
		pos += idx + 1
	}
	return count
}

func main() {
	count := 100000
	hello := "Hello"

	start := time.Now()

	// Build string using strings.Builder for efficiency
	var builder strings.Builder
	builder.Grow(count * len(hello))
	for i := 0; i < count; i++ {
		builder.WriteString(hello)
	}
	result := builder.String()

	// Count occurrences of 'llo'
	occurrences := countOccurrences(result, "llo")

	elapsed := time.Since(start).Milliseconds()

	fmt.Printf("String length: %d\n", len(result))
	fmt.Printf("Occurrences of 'llo': %d\n", occurrences)
	fmt.Printf("String time: %dms\n", elapsed)
}
