# Benchmark Results

Performance comparison between Sindarin and other programming languages.

## Executive Summary

Sindarin delivers **compiled-language performance** while offering high-level language ergonomics. By compiling to optimized C code, Sindarin consistently ranks among the fastest languages tested.

### Key Highlights

| Metric | Result |
|--------|--------|
| **Fastest benchmark** | String Operations (1ms) - #1 overall |
| **vs Python** | 3-51x faster |
| **vs Node.js** | 1-19x faster |
| **vs Java** | 0.9-18x faster |
| **vs C (baseline)** | 0.2-2.0x (competitive) |

### Performance Tier

```
S-Tier  ████  C, Rust
A-Tier  ███   Sindarin, C#, Go
B-Tier  ██    Java, Node.js
C-Tier  █     Python
```

Sindarin sits comfortably in the **A-Tier**, trading blows with established compiled languages while significantly outperforming interpreted languages.

---

## Test Environment

- **Date**: 2026-01-01
- **Runs per benchmark**: 3 (median reported)
- **Sindarin**: Compiled to C via GCC with `-O2` optimization

---

## Benchmark Results

### 1. Fibonacci (Recursive fib(35))

*Tests function call overhead and recursion performance.*

```
sindarin  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   29ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   27ms  (fastest)
go        █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   50ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   29ms
java      █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   42ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   57ms
python    ████████████████████████████████████████ 1470ms
nodejs    ██████████████░░░░░░░░░░░░░░░░░░░░░░░░░░  546ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | C | 27ms | 0.93x |
| **2** | **Sindarin** | **29ms** | **1.00x** |
| 3 | Rust | 29ms | 1.00x |
| 4 | Java | 42ms | 1.45x |
| 5 | Go | 50ms | 1.72x |
| 6 | C# | 57ms | 1.97x |
| 7 | Node.js | 546ms | 18.8x |
| 8 | Python | 1470ms | 50.7x |

---

### 2. Prime Sieve (Sieve of Eratosthenes to 1,000,000)

*Tests memory allocation and iteration performance.*

```
sindarin  █████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   18ms
c         ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
go        ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    3ms
rust      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms  (fastest)
java      ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   16ms
csharp    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
python    ████████████████████████████████████████  141ms
nodejs    ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   25ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 2ms | 0.11x |
| 2 | C | 3ms | 0.17x |
| 3 | Go | 3ms | 0.17x |
| 4 | C# | 4ms | 0.22x |
| 5 | Java | 16ms | 0.89x |
| **6** | **Sindarin** | **18ms** | **1.00x** |
| 7 | Node.js | 25ms | 1.39x |
| 8 | Python | 141ms | 7.83x |

---

### 3. String Operations (100,000 concatenations)

*Tests string manipulation and memory management.*

```
sindarin  █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    1ms  (fastest)
c         ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
go        ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    2ms
rust      ████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms
java      █████████████████████░░░░░░░░░░░░░░░░░░░   18ms
csharp    ████████████████████████████████████████   34ms
python    █████████████████████████░░░░░░░░░░░░░░░   22ms
nodejs    ████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    7ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| **1** | **Sindarin** | **1ms** | **1.00x** |
| 2 | C | 2ms | 2.00x |
| 3 | Go | 2ms | 2.00x |
| 4 | Rust | 4ms | 4.00x |
| 5 | Node.js | 7ms | 7.00x |
| 6 | Java | 18ms | 18.0x |
| 7 | Python | 22ms | 22.0x |
| 8 | C# | 34ms | 34.0x |

---

### 4. Array Operations (1,000,000 integers)

*Tests array creation, iteration, and in-place reversal.*

```
sindarin  ██████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   11ms
c         ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    5ms
go        ███░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    7ms
rust      ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░    4ms  (fastest)
java      ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   14ms
csharp    ███████░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   14ms
python    ████████████████████░░░░░░░░░░░░░░░░░░░░   37ms
nodejs    ████████████████████████████████████████   72ms
```

| Rank | Language | Time | vs Sindarin |
|:----:|----------|-----:|:-----------:|
| 1 | Rust | 4ms | 0.36x |
| 2 | C | 5ms | 0.45x |
| 3 | Go | 7ms | 0.64x |
| **4** | **Sindarin** | **11ms** | **1.00x** |
| 5 | Java | 14ms | 1.27x |
| 6 | C# | 14ms | 1.27x |
| 7 | Python | 37ms | 3.36x |
| 8 | Node.js | 72ms | 6.55x |

---

## Overall Rankings

Average rank across all benchmarks:

| Rank | Language | Avg Position | Strengths |
|:----:|----------|:------------:|-----------|
| 1 | C | 1.8 | Fastest baseline, low-level control |
| 2 | Rust | 2.2 | Memory safety + speed |
| **3** | **Sindarin** | **3.2** | **String ops champion, high-level syntax** |
| 4 | Go | 3.5 | Fast compilation, good concurrency |
| 5 | Java | 5.0 | JIT warmup needed |
| 6 | C# | 6.0 | Good all-rounder |
| 7 | Node.js | 6.8 | V8 optimization |
| 8 | Python | 7.5 | Interpreted overhead |

---

## Compilation Notes

| Language | Compiler/Runtime | Flags |
|----------|------------------|-------|
| Sindarin | GCC (via C) | `-O2` |
| C | GCC | `-O2` |
| Rust | rustc | `-O` (release) |
| Go | go build | default |
| Java | javac | default |
| C# | dotnet | `-c Release` |
| Python | CPython 3.x | interpreter |
| Node.js | V8 | default |

---

## Validation

All implementations produce identical results:

- Fibonacci(35) = 9,227,465
- Primes up to 1,000,000 = 78,498
- String length = 500,000; Occurrences = 100,000
- Array sum = 499,999,500,000
