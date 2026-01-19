#!/usr/bin/env node
// arrays.js - Array Operations Benchmark for Node.js
// Tests dynamic array operations and iteration

function main() {
    const size = 1000000;

    const start = Date.now();

    // Create array with 1,000,000 integers (0 to 999,999)
    const arr = new Array(size);
    for (let i = 0; i < size; i++) {
        arr[i] = i;
    }

    // Sum all elements
    let sum1 = 0n;
    for (let i = 0; i < size; i++) {
        sum1 += BigInt(arr[i]);
    }

    // Reverse the array in-place
    arr.reverse();

    // Sum again to verify
    let sum2 = 0n;
    for (let i = 0; i < size; i++) {
        sum2 += BigInt(arr[i]);
    }

    const elapsed = Date.now() - start;

    console.log(`Sum: ${sum1}`);
    console.log(`Reversed sum: ${sum2}`);
    console.log(`Array time: ${elapsed}ms`);
}

main();
