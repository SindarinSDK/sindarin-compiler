#!/usr/bin/env node
// strings.js - String Operations Benchmark for Node.js
// Tests string concatenation and manipulation performance

function countOccurrences(str, substr) {
    let count = 0;
    let pos = 0;
    while ((pos = str.indexOf(substr, pos)) !== -1) {
        count++;
        pos++;
    }
    return count;
}

function main() {
    const count = 100000;
    const hello = "Hello";

    const start = Date.now();

    // Build string using array join (efficient in JS)
    const parts = new Array(count).fill(hello);
    const result = parts.join("");

    // Count occurrences of 'llo'
    const occurrences = countOccurrences(result, "llo");

    const elapsed = Date.now() - start;

    console.log(`String length: ${result.length}`);
    console.log(`Occurrences of 'llo': ${occurrences}`);
    console.log(`String time: ${elapsed}ms`);
}

main();
