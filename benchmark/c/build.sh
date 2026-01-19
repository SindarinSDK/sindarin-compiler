#!/bin/bash
# Build script for C benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building C benchmarks..."

gcc -O2 -o fib fib.c
gcc -O2 -o primes primes.c
gcc -O2 -o strings strings.c
gcc -O2 -o arrays arrays.c

echo "Build complete."
