#!/usr/bin/env python3
# fib.py - Fibonacci Benchmark for Python
# Tests function call overhead (recursive) and loop performance (iterative)

import time

def fib_recursive(n):
    """Recursive Fibonacci - tests function call overhead"""
    if n <= 1:
        return n
    return fib_recursive(n - 1) + fib_recursive(n - 2)

def fib_iterative(n):
    """Iterative Fibonacci - tests loop performance"""
    if n <= 1:
        return n
    a, b = 0, 1
    for _ in range(2, n + 1):
        a, b = b, a + b
    return b

def main():
    # Recursive fib(35)
    start = time.time()
    result = fib_recursive(35)
    elapsed = int((time.time() - start) * 1000)
    print(f"Recursive fib(35) = {result}")
    print(f"Recursive time: {elapsed}ms")

    # Iterative fib(50)
    start = time.time()
    result = fib_iterative(50)
    elapsed = int((time.time() - start) * 1000)
    print(f"Iterative fib(50) = {result}")
    print(f"Iterative time: {elapsed}ms")

if __name__ == "__main__":
    main()
