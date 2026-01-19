#!/usr/bin/env python3
# arrays.py - Array Operations Benchmark for Python
# Tests dynamic array operations and iteration

import time

def main():
    size = 1000000

    start = time.time()

    # Create array with 1,000,000 integers (0 to 999,999)
    arr = list(range(size))

    # Sum all elements
    sum1 = sum(arr)

    # Reverse the array in-place
    arr.reverse()

    # Sum again to verify
    sum2 = sum(arr)

    elapsed = int((time.time() - start) * 1000)

    print(f"Sum: {sum1}")
    print(f"Reversed sum: {sum2}")
    print(f"Array time: {elapsed}ms")

if __name__ == "__main__":
    main()
