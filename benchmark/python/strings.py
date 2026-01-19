#!/usr/bin/env python3
# strings.py - String Operations Benchmark for Python
# Tests string concatenation and manipulation performance

import time

def count_occurrences(s, substr):
    """Count occurrences of substring"""
    count = 0
    pos = 0
    while True:
        pos = s.find(substr, pos)
        if pos == -1:
            break
        count += 1
        pos += 1
    return count

def main():
    count = 100000
    hello = "Hello"

    start = time.time()

    # Build string using list join (efficient in Python)
    parts = [hello] * count
    result = "".join(parts)

    # Count occurrences of 'llo'
    occurrences = count_occurrences(result, "llo")

    elapsed = int((time.time() - start) * 1000)

    print(f"String length: {len(result)}")
    print(f"Occurrences of 'llo': {occurrences}")
    print(f"String time: {elapsed}ms")

if __name__ == "__main__":
    main()
