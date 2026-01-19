#!/bin/bash
# Benchmark runner script
# Executes all benchmarks 3 times per language and captures timing results

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Configuration
RUNS=3
LANGUAGES=("sindarin" "c" "go" "rust" "java" "csharp" "python" "nodejs")
BENCHMARKS=("fib" "primes" "strings" "arrays")

# Output files
RESULTS_JSON="$SCRIPT_DIR/results.json"
RESULTS_CSV="$SCRIPT_DIR/results.csv"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper function to extract time in milliseconds from output
# Matches patterns like "time: 123ms" or "time: 123 ms"
extract_time() {
    local output="$1"
    local pattern="$2"
    # Extract the time value from lines matching the pattern
    echo "$output" | grep -i "$pattern" | grep -oE '[0-9]+' | head -1
}

# Calculate median of an array of numbers
calculate_median() {
    local -a arr=("$@")
    local n=${#arr[@]}

    if [ $n -eq 0 ]; then
        echo "0"
        return
    fi

    # Sort the array
    IFS=$'\n' sorted=($(sort -n <<<"${arr[*]}")); unset IFS

    # Get median
    local mid=$((n / 2))
    if [ $((n % 2)) -eq 0 ]; then
        # Even number of elements - average of two middle values
        local val1=${sorted[$((mid - 1))]}
        local val2=${sorted[$mid]}
        echo $(( (val1 + val2) / 2 ))
    else
        echo "${sorted[$mid]}"
    fi
}

# Get the command to run a benchmark for a given language
get_run_command() {
    local lang="$1"
    local bench="$2"

    case "$lang" in
        sindarin)
            echo "./$bench"
            ;;
        c)
            echo "./$bench"
            ;;
        go)
            if [ "$bench" = "strings" ]; then
                echo "./strings_bench"
            else
                echo "./$bench"
            fi
            ;;
        rust)
            echo "./$bench"
            ;;
        java)
            # Java class names are capitalized
            local class_name
            case "$bench" in
                fib) class_name="Fib" ;;
                primes) class_name="Primes" ;;
                strings) class_name="Strings" ;;
                arrays) class_name="Arrays" ;;
            esac
            echo "java $class_name"
            ;;
        csharp)
            # C# uses dotnet project structure
            local class_name
            case "$bench" in
                fib) class_name="Fib" ;;
                primes) class_name="Primes" ;;
                strings) class_name="Strings" ;;
                arrays) class_name="Arrays" ;;
            esac
            echo "./$class_name/bin/$class_name"
            ;;
        python)
            echo "python3 $bench.py"
            ;;
        nodejs)
            echo "node $bench.js"
            ;;
        *)
            echo ""
            ;;
    esac
}

# Check if a benchmark exists for a language
check_benchmark_exists() {
    local lang="$1"
    local bench="$2"
    local lang_dir="$SCRIPT_DIR/$lang"

    case "$lang" in
        sindarin)
            [ -x "$lang_dir/$bench" ] || [ -f "$lang_dir/$bench.sn" ]
            ;;
        c)
            [ -x "$lang_dir/$bench" ]
            ;;
        go)
            if [ "$bench" = "strings" ]; then
                [ -x "$lang_dir/strings_bench" ]
            else
                [ -x "$lang_dir/$bench" ]
            fi
            ;;
        rust)
            [ -x "$lang_dir/$bench" ]
            ;;
        java)
            local class_name
            case "$bench" in
                fib) class_name="Fib" ;;
                primes) class_name="Primes" ;;
                strings) class_name="Strings" ;;
                arrays) class_name="Arrays" ;;
            esac
            [ -f "$lang_dir/$class_name.class" ]
            ;;
        csharp)
            local class_name
            case "$bench" in
                fib) class_name="Fib" ;;
                primes) class_name="Primes" ;;
                strings) class_name="Strings" ;;
                arrays) class_name="Arrays" ;;
            esac
            [ -f "$lang_dir/$class_name/bin/$class_name" ]
            ;;
        python)
            [ -f "$lang_dir/$bench.py" ]
            ;;
        nodejs)
            [ -f "$lang_dir/$bench.js" ]
            ;;
        *)
            return 1
            ;;
    esac
}

# Build all benchmarks first
echo -e "${BLUE}Building all benchmarks...${NC}"
for lang in "${LANGUAGES[@]}"; do
    if [ -f "$SCRIPT_DIR/$lang/build.sh" ]; then
        echo -e "  Building ${GREEN}$lang${NC}..."
        if ! (cd "$SCRIPT_DIR/$lang" && ./build.sh > /dev/null 2>&1); then
            echo -e "  ${YELLOW}Warning: Failed to build $lang${NC}"
        fi
    fi
done
echo ""

# Initialize results storage
declare -A results
declare -A raw_results

echo -e "${BLUE}Running benchmarks (${RUNS} runs each)...${NC}"
echo ""

# Run benchmarks
for lang in "${LANGUAGES[@]}"; do
    echo -e "${GREEN}Language: $lang${NC}"
    lang_dir="$SCRIPT_DIR/$lang"

    if [ ! -d "$lang_dir" ]; then
        echo -e "  ${YELLOW}Warning: Directory $lang not found, skipping${NC}"
        continue
    fi

    for bench in "${BENCHMARKS[@]}"; do
        if ! check_benchmark_exists "$lang" "$bench"; then
            echo -e "  ${YELLOW}Warning: $bench not found for $lang, skipping${NC}"
            continue
        fi

        run_cmd=$(get_run_command "$lang" "$bench")
        if [ -z "$run_cmd" ]; then
            echo -e "  ${YELLOW}Warning: Unknown run command for $lang/$bench${NC}"
            continue
        fi

        echo -n "  Running $bench: "

        declare -a times=()

        for ((run=1; run<=RUNS; run++)); do
            # Run the benchmark and capture output
            output=$(cd "$lang_dir" && $run_cmd 2>&1) || true

            # Extract timing based on benchmark type
            case "$bench" in
                fib)
                    # Extract recursive time (first timing value)
                    time_val=$(extract_time "$output" "Recursive time")
                    ;;
                primes)
                    time_val=$(extract_time "$output" "Sieve time")
                    ;;
                strings)
                    time_val=$(extract_time "$output" "String time")
                    ;;
                arrays)
                    time_val=$(extract_time "$output" "Array time")
                    ;;
            esac

            if [ -n "$time_val" ]; then
                times+=("$time_val")
                echo -n "${time_val}ms "
            else
                echo -n "ERR "
            fi
        done

        # Calculate and store median
        if [ ${#times[@]} -gt 0 ]; then
            median=$(calculate_median "${times[@]}")
            echo -e "-> median: ${GREEN}${median}ms${NC}"
            results["${lang}_${bench}"]="$median"
            raw_results["${lang}_${bench}"]="${times[*]}"
        else
            echo -e "-> ${RED}no valid results${NC}"
            results["${lang}_${bench}"]=""
            raw_results["${lang}_${bench}"]=""
        fi
    done
    echo ""
done

# Generate JSON output
echo -e "${BLUE}Saving results to $RESULTS_JSON...${NC}"
cat > "$RESULTS_JSON" << 'JSONHEADER'
{
  "metadata": {
    "runs_per_benchmark": 3,
JSONHEADER
echo "    \"timestamp\": \"$(date -Iseconds)\"" >> "$RESULTS_JSON"
cat >> "$RESULTS_JSON" << 'JSONMID'
  },
  "results": {
JSONMID

first_lang=true
for lang in "${LANGUAGES[@]}"; do
    if [ "$first_lang" = true ]; then
        first_lang=false
    else
        echo "," >> "$RESULTS_JSON"
    fi

    echo -n "    \"$lang\": {" >> "$RESULTS_JSON"

    first_bench=true
    for bench in "${BENCHMARKS[@]}"; do
        if [ "$first_bench" = true ]; then
            first_bench=false
        else
            echo -n "," >> "$RESULTS_JSON"
        fi

        median="${results[${lang}_${bench}]}"
        raw="${raw_results[${lang}_${bench}]}"

        if [ -n "$median" ]; then
            echo -n "\"$bench\": {\"median_ms\": $median, \"runs\": [${raw// /, }]}" >> "$RESULTS_JSON"
        else
            echo -n "\"$bench\": null" >> "$RESULTS_JSON"
        fi
    done

    echo -n "}" >> "$RESULTS_JSON"
done

cat >> "$RESULTS_JSON" << 'JSONFOOTER'

  }
}
JSONFOOTER

# Generate CSV output
echo -e "${BLUE}Saving results to $RESULTS_CSV...${NC}"
echo "language,benchmark,median_ms,run1_ms,run2_ms,run3_ms" > "$RESULTS_CSV"

for lang in "${LANGUAGES[@]}"; do
    for bench in "${BENCHMARKS[@]}"; do
        median="${results[${lang}_${bench}]}"
        raw="${raw_results[${lang}_${bench}]}"

        if [ -n "$median" ]; then
            # Convert space-separated values to comma-separated
            runs_csv=$(echo "$raw" | tr ' ' ',')
            echo "$lang,$bench,$median,$runs_csv" >> "$RESULTS_CSV"
        else
            echo "$lang,$bench,,," >> "$RESULTS_CSV"
        fi
    done
done

echo ""
echo -e "${GREEN}Benchmark run complete!${NC}"
echo "Results saved to:"
echo "  - $RESULTS_JSON"
echo "  - $RESULTS_CSV"
