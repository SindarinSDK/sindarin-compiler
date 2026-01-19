#!/bin/bash
# Build script for Python benchmarks
# Python is interpreted, so this script validates syntax only

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building Python benchmarks (syntax validation)..."

python3 -m py_compile fib.py
python3 -m py_compile primes.py
python3 -m py_compile strings.py
python3 -m py_compile arrays.py

echo "Build complete."
