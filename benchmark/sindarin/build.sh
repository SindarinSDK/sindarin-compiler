#!/bin/bash
# Build script for Sindarin benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Go to project root to access bin/sn
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Building Sindarin benchmarks..."

"$PROJECT_ROOT/bin/sn" fib.sn -o fib -O2
"$PROJECT_ROOT/bin/sn" primes.sn -o primes -O2
"$PROJECT_ROOT/bin/sn" strings.sn -o strings -O2
"$PROJECT_ROOT/bin/sn" arrays.sn -o arrays -O2

echo "Build complete."
