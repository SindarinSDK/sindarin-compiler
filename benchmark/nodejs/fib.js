#!/usr/bin/env node
// fib.js - Fibonacci Benchmark for Node.js
// Tests function call overhead (recursive) and loop performance (iterative)

// Recursive Fibonacci - tests function call overhead
function fibRecursive(n) {
    if (n <= 1) return BigInt(n);
    return fibRecursive(n - 1) + fibRecursive(n - 2);
}

// Iterative Fibonacci - tests loop performance
function fibIterative(n) {
    if (n <= 1) return BigInt(n);
    let a = 0n, b = 1n;
    for (let i = 2; i <= n; i++) {
        const temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

function main() {
    let start, result, elapsed;

    // Recursive fib(35)
    start = Date.now();
    result = fibRecursive(35);
    elapsed = Date.now() - start;
    console.log(`Recursive fib(35) = ${result}`);
    console.log(`Recursive time: ${elapsed}ms`);

    // Iterative fib(50)
    start = Date.now();
    result = fibIterative(50);
    elapsed = Date.now() - start;
    console.log(`Iterative fib(50) = ${result}`);
    console.log(`Iterative time: ${elapsed}ms`);
}

main();
