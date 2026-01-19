# Benchmark Specification

This document defines the benchmark algorithms used to compare Sindarin against other programming languages.

## Overview

The benchmark suite measures computational performance across several categories:
- CPU-bound computation
- Memory allocation patterns
- String manipulation
- Iteration/loop performance

Each implementation must follow the same algorithm and produce identical output to ensure fair comparison.

## Benchmark 1: Fibonacci Sequence (CPU-Bound)

**File:** `fib.{ext}`

Calculate the Nth Fibonacci number using recursive implementation (to test function call overhead) and iterative implementation (to test loop performance).

### Requirements:
- Implement recursive `fib_recursive(n)` function
- Implement iterative `fib_iterative(n)` function
- Compute fib(35) using recursive approach and measure time
- Compute fib(50) using iterative approach and measure time
- Print results and execution times

### Expected Output Format:
```
Recursive fib(35) = 9227465
Recursive time: <milliseconds>ms
Iterative fib(50) = 12586269025
Iterative time: <milliseconds>ms
```

## Benchmark 2: Prime Sieve (Memory + CPU)

**File:** `primes.{ext}`

Implement the Sieve of Eratosthenes to find all prime numbers up to N.

### Requirements:
- Implement sieve algorithm for N = 1,000,000
- Count the number of primes found
- Measure execution time

### Expected Output Format:
```
Primes up to 1000000: 78498
Sieve time: <milliseconds>ms
```

## Benchmark 3: String Operations

**File:** `strings.{ext}`

Test string concatenation and manipulation performance.

### Requirements:
- Build a string by concatenating "Hello" 100,000 times
- Count occurrences of a substring
- Measure total time

### Expected Output Format:
```
String length: 500000
Occurrences of 'llo': 100000
String time: <milliseconds>ms
```

## Benchmark 4: Array/List Operations

**File:** `arrays.{ext}`

Test dynamic array/list operations.

### Requirements:
- Create array with 1,000,000 integers (0 to 999,999)
- Sum all elements
- Reverse the array in-place
- Sum again to verify
- Measure total time

### Expected Output Format:
```
Sum: 499999500000
Reversed sum: 499999500000
Array time: <milliseconds>ms
```

## Implementation Guidelines

### Language-Specific Notes

| Language | Compiler/Runtime | Optimization Flags |
|----------|------------------|-------------------|
| Sindarin | sn → gcc | -O2 |
| C | gcc | -O2 |
| Rust | rustc | --release |
| Go | go build | (default) |
| Java | javac + java | (default) |
| C# | dotnet | --configuration Release |
| Python | python3 | (default) |
| Node.js | node | (default) |

### Measurement Standards

1. **Warm-up:** Each benchmark should be run once for warm-up (especially for JIT languages)
2. **Iterations:** Run each benchmark 3 times and report the median
3. **Timing:** Use high-resolution timers where available
4. **Output:** All output must go to stdout
5. **No external dependencies:** Use only standard library features

### File Structure

Each language directory should contain:
```
benchmark/<language>/
  ├── fib.{ext}      # Fibonacci benchmark
  ├── primes.{ext}   # Prime sieve benchmark
  ├── strings.{ext}  # String operations benchmark
  ├── arrays.{ext}   # Array operations benchmark
  └── build.sh       # Build script (if compilation needed)
```

## Running Benchmarks

The benchmark harness (to be implemented) will:
1. Build all implementations
2. Run each benchmark 3 times per language
3. Collect timing data
4. Generate BENCHMARKS.md with comparison tables

## Success Criteria

A benchmark implementation is correct if:
1. It produces the expected output values (ignoring timing)
2. It follows the algorithm specification exactly
3. It uses only standard library features
4. It compiles/runs without errors
