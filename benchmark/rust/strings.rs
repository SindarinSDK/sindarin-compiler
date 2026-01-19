// strings.rs - String Operations Benchmark for Rust
// Tests string concatenation and manipulation performance

use std::time::Instant;

fn count_occurrences(s: &str, substr: &str) -> usize {
    let mut count = 0;
    let mut pos = 0;
    while let Some(idx) = s[pos..].find(substr) {
        count += 1;
        pos += idx + 1;
    }
    count
}

fn main() {
    let count = 100000;
    let hello = "Hello";

    let start = Instant::now();

    // Build string using String with pre-allocated capacity
    let mut result = String::with_capacity(count * hello.len());
    for _ in 0..count {
        result.push_str(hello);
    }

    // Count occurrences of 'llo'
    let occurrences = count_occurrences(&result, "llo");

    let elapsed = start.elapsed().as_millis();

    println!("String length: {}", result.len());
    println!("Occurrences of 'llo': {}", occurrences);
    println!("String time: {}ms", elapsed);
}
