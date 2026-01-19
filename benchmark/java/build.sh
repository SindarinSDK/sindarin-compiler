#!/bin/bash
# Build script for Java benchmarks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building Java benchmarks..."

javac Fib.java
javac Primes.java
javac Strings.java
javac Arrays.java

echo "Build complete."
