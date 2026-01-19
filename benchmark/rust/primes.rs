// primes.rs - Prime Sieve Benchmark for Rust
// Tests memory allocation and CPU-bound computation

use std::time::Instant;

// Sieve of Eratosthenes
fn sieve_of_eratosthenes(limit: usize) -> usize {
    let mut is_prime = vec![true; limit + 1];
    is_prime[0] = false;
    is_prime[1] = false;

    let mut i = 2;
    while i * i <= limit {
        if is_prime[i] {
            let mut j = i * i;
            while j <= limit {
                is_prime[j] = false;
                j += i;
            }
        }
        i += 1;
    }

    is_prime.iter().filter(|&&p| p).count()
}

fn main() {
    let limit = 1000000;

    let start = Instant::now();
    let count = sieve_of_eratosthenes(limit);
    let elapsed = start.elapsed().as_millis();

    println!("Primes up to {}: {}", limit, count);
    println!("Sieve time: {}ms", elapsed);
}
