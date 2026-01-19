// fib.rs - Fibonacci Benchmark for Rust
// Tests function call overhead (recursive) and loop performance (iterative)

use std::time::Instant;

// Recursive Fibonacci - tests function call overhead
fn fib_recursive(n: i32) -> i64 {
    if n <= 1 {
        return n as i64;
    }
    fib_recursive(n - 1) + fib_recursive(n - 2)
}

// Iterative Fibonacci - tests loop performance
fn fib_iterative(n: i32) -> i64 {
    if n <= 1 {
        return n as i64;
    }
    let mut a: i64 = 0;
    let mut b: i64 = 1;
    for _ in 2..=n {
        let temp = a + b;
        a = b;
        b = temp;
    }
    b
}

fn main() {
    // Recursive fib(35)
    let start = Instant::now();
    let result = fib_recursive(35);
    let elapsed = start.elapsed().as_millis();
    println!("Recursive fib(35) = {}", result);
    println!("Recursive time: {}ms", elapsed);

    // Iterative fib(50)
    let start = Instant::now();
    let result = fib_iterative(50);
    let elapsed = start.elapsed().as_millis();
    println!("Iterative fib(50) = {}", result);
    println!("Iterative time: {}ms", elapsed);
}
