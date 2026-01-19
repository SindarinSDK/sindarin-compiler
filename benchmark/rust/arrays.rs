// arrays.rs - Array Operations Benchmark for Rust
// Tests dynamic array operations and iteration

use std::time::Instant;

fn main() {
    let size = 1000000;

    let start = Instant::now();

    // Create array with 1,000,000 integers (0 to 999,999)
    let mut arr: Vec<i32> = (0..size).collect();

    // Sum all elements
    let sum1: i64 = arr.iter().map(|&x| x as i64).sum();

    // Reverse the array in-place
    arr.reverse();

    // Sum again to verify
    let sum2: i64 = arr.iter().map(|&x| x as i64).sum();

    let elapsed = start.elapsed().as_millis();

    println!("Sum: {}", sum1);
    println!("Reversed sum: {}", sum2);
    println!("Array time: {}ms", elapsed);
}
