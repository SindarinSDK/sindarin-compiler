#!/bin/bash
# Build script for Rust benchmarks

set -e

# Source cargo environment if available
[ -f "$HOME/.cargo/env" ] && source "$HOME/.cargo/env"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building Rust benchmarks..."

rustc -O -o fib fib.rs
rustc -O -o primes primes.rs
rustc -O -o strings strings.rs
rustc -O -o arrays arrays.rs

echo "Build complete."
