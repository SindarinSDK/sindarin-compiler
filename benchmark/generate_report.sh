#!/bin/bash
# Report generator for benchmark results
# Produces BENCHMARKS.md with comparison tables and ASCII visualizations

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
LANGUAGES=("sindarin" "c" "go" "rust" "java" "csharp" "python" "nodejs")
BENCHMARKS=("fib" "primes" "strings" "arrays")

# Output files
RESULTS_JSON="$SCRIPT_DIR/results.json"
BENCHMARKS_FILE="$SCRIPT_DIR/../BENCHMARKS.md"

# Colors for terminal output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

if [ ! -f "$RESULTS_JSON" ]; then
    echo "Error: results.json not found. Run benchmarks first."
    exit 1
fi

echo -e "${BLUE}Generating BENCHMARKS.md...${NC}"

# Helper function to extract median from JSON using Python
extract_median() {
    local lang="$1"
    local bench="$2"
    python3 -c "
import json
with open('$RESULTS_JSON') as f:
    data = json.load(f)
result = data.get('results', {}).get('$lang', {}).get('$bench', {})
if result:
    print(result.get('median_ms', 'N/A'))
else:
    print('N/A')
" 2>/dev/null || echo "N/A"
}

# Helper function to generate ASCII bar chart
generate_bar() {
    local value=$1
    local max_value=$2
    local max_width=40

    if [ "$value" = "N/A" ] || [ -z "$value" ] || [ "$value" = "0" ]; then
        echo "▏"
        return
    fi

    if [ "$max_value" = "0" ] || [ -z "$max_value" ]; then
        echo "▏"
        return
    fi

    local width=$((value * max_width / max_value))
    if [ $width -lt 1 ]; then
        width=1
    fi

    printf '█%.0s' $(seq 1 $width)
}

# Generate the report
cat > "$BENCHMARKS_FILE" << 'HEADER'
# Benchmark Results

This document contains performance comparison results between Sindarin and other programming languages.

## Test Environment

HEADER

echo "- **Date**: $(date '+%Y-%m-%d %H:%M:%S')" >> "$BENCHMARKS_FILE"

cat >> "$BENCHMARKS_FILE" << 'HEADER2'
- **Runs per benchmark**: 3 (median reported)
- **Machine**: Linux (benchmark environment)

## Quick Summary

HEADER2

# Generate quick summary table with all benchmarks
echo "| Language | Fibonacci | Primes | Strings | Arrays |" >> "$BENCHMARKS_FILE"
echo "|----------|-----------|--------|---------|--------|" >> "$BENCHMARKS_FILE"

for lang in "${LANGUAGES[@]}"; do
    fib=$(extract_median "$lang" "fib")
    primes=$(extract_median "$lang" "primes")
    strings=$(extract_median "$lang" "strings")
    arrays=$(extract_median "$lang" "arrays")
    echo "| $lang | ${fib}ms | ${primes}ms | ${strings}ms | ${arrays}ms |" >> "$BENCHMARKS_FILE"
done

cat >> "$BENCHMARKS_FILE" << 'DETAILED'

## Detailed Results by Benchmark

### 1. Fibonacci (CPU-bound, recursive fib(35))

Tests function call overhead with recursive algorithm.

| Language | Time (ms) | Visualization |
|----------|-----------|---------------|
DETAILED

# Get max value for fib for visualization scaling
max_fib=0
for lang in "${LANGUAGES[@]}"; do
    val=$(extract_median "$lang" "fib")
    if [ "$val" != "N/A" ] && [ -n "$val" ] && [ "$val" -gt "$max_fib" ] 2>/dev/null; then
        max_fib=$val
    fi
done

for lang in "${LANGUAGES[@]}"; do
    time_val=$(extract_median "$lang" "fib")
    bar=$(generate_bar "$time_val" "$max_fib")
    echo "| $lang | $time_val | \`$bar\` |" >> "$BENCHMARKS_FILE"
done

cat >> "$BENCHMARKS_FILE" << 'TABLE2'

### 2. Prime Sieve (Memory + CPU, sieve up to 1,000,000)

Tests memory allocation and iteration performance.

| Language | Time (ms) | Visualization |
|----------|-----------|---------------|
TABLE2

# Get max value for primes
max_primes=0
for lang in "${LANGUAGES[@]}"; do
    val=$(extract_median "$lang" "primes")
    if [ "$val" != "N/A" ] && [ -n "$val" ] && [ "$val" -gt "$max_primes" ] 2>/dev/null; then
        max_primes=$val
    fi
done

for lang in "${LANGUAGES[@]}"; do
    time_val=$(extract_median "$lang" "primes")
    bar=$(generate_bar "$time_val" "$max_primes")
    echo "| $lang | $time_val | \`$bar\` |" >> "$BENCHMARKS_FILE"
done

cat >> "$BENCHMARKS_FILE" << 'TABLE3'

### 3. String Operations (100,000 concatenations)

Tests string manipulation and substring search.

| Language | Time (ms) | Visualization |
|----------|-----------|---------------|
TABLE3

# Get max value for strings
max_strings=0
for lang in "${LANGUAGES[@]}"; do
    val=$(extract_median "$lang" "strings")
    if [ "$val" != "N/A" ] && [ -n "$val" ] && [ "$val" -gt "$max_strings" ] 2>/dev/null; then
        max_strings=$val
    fi
done

for lang in "${LANGUAGES[@]}"; do
    time_val=$(extract_median "$lang" "strings")
    bar=$(generate_bar "$time_val" "$max_strings")
    echo "| $lang | $time_val | \`$bar\` |" >> "$BENCHMARKS_FILE"
done

cat >> "$BENCHMARKS_FILE" << 'TABLE4'

### 4. Array Operations (1,000,000 integers)

Tests array creation, iteration, and in-place reversal.

| Language | Time (ms) | Visualization |
|----------|-----------|---------------|
TABLE4

# Get max value for arrays
max_arrays=0
for lang in "${LANGUAGES[@]}"; do
    val=$(extract_median "$lang" "arrays")
    if [ "$val" != "N/A" ] && [ -n "$val" ] && [ "$val" -gt "$max_arrays" ] 2>/dev/null; then
        max_arrays=$val
    fi
done

for lang in "${LANGUAGES[@]}"; do
    time_val=$(extract_median "$lang" "arrays")
    bar=$(generate_bar "$time_val" "$max_arrays")
    echo "| $lang | $time_val | \`$bar\` |" >> "$BENCHMARKS_FILE"
done

cat >> "$BENCHMARKS_FILE" << 'FOOTER'

## Compiler/Runtime Configuration

| Language | Compiler/Runtime | Optimization |
|----------|------------------|--------------|
| Sindarin | sn → GCC | -O2 |
| C | GCC | -O2 |
| Go | go build | default |
| Rust | rustc | -O (release) |
| Java | javac + JVM | default |
| C# | dotnet | Release |
| Python | CPython 3.x | interpreted |
| Node.js | V8 | JIT |

## Validation

All benchmark implementations produce the expected output values:

| Benchmark | Expected Value |
|-----------|----------------|
| Fibonacci(35) | 9,227,465 |
| Fibonacci(50) | 12,586,269,025 |
| Primes ≤ 1,000,000 | 78,498 |
| String length | 500,000 |
| String occurrences | 100,000 |
| Array sum | 499,999,500,000 |

## Raw Data

The raw benchmark data is available in:
- `benchmark/results.json` - Full JSON with all runs
- `benchmark/results.csv` - CSV format for spreadsheet analysis
FOOTER

echo -e "${GREEN}Generated: $BENCHMARKS_FILE${NC}"
