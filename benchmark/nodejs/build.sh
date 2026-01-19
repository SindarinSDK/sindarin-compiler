#!/bin/bash
# Build script for Node.js benchmarks
# Node.js is interpreted, so this script validates syntax only

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building Node.js benchmarks (syntax validation)..."

node --check fib.js
node --check primes.js
node --check strings.js
node --check arrays.js

echo "Build complete."
