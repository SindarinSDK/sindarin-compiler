#!/bin/bash
# Build script for Go benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building Go benchmarks..."

go build -o fib fib.go
go build -o primes primes.go
go build -o strings_bench strings.go
go build -o arrays arrays.go

echo "Build complete."
