---
title: Quick Start
description: Get started with Sindarin through practical examples
permalink: /language/quick-start/
---

## Hello World

```sindarin
fn main(): void =>
  print("Hello, World!\n")
```

## FizzBuzz

```sindarin
fn main(): void =>
  for var i: int = 1; i <= 100; i++ =>
    if i % 15 == 0 =>
      print("FizzBuzz\n")
    else if i % 3 == 0 =>
      print("Fizz\n")
    else if i % 5 == 0 =>
      print("Buzz\n")
    else =>
      print($"{i}\n")
```

## Prime Finder

```sindarin
fn is_prime(n: int): bool =>
  if n <= 1 =>
    return false
  var i: int = 2
  while i * i <= n =>
    if n % i == 0 =>
      return false
    i = i + 1
  return true

fn find_primes(limit: int): int[] =>
  var primes: int[] = {}
  for var n: int = 2; n <= limit; n++ =>
    if is_prime(n) =>
      primes.push(n)
  return primes

fn main(): void =>
  var primes: int[] = find_primes(50)
  print($"Found {primes.length} primes: {primes.join(\", \")}\n")
```

## File Processing

```sindarin
fn main(): void =>
  // Read file and count lines
  var content: str = TextFile.readAll("data.txt")
  var lines: str[] = content.splitLines()

  var nonEmpty: int = 0
  for line in lines =>
    if !line.isBlank() =>
      nonEmpty = nonEmpty + 1

  print($"Total lines: {lines.length}\n")
  print($"Non-empty lines: {nonEmpty}\n")
```

## String Processing

```sindarin
fn main(): void =>
  var text: str = "  Hello, World!  "

  // Method chaining
  var cleaned: str = text.trim().toLower()
  print($"Cleaned: '{cleaned}'\n")

  // Splitting and joining
  var words: str[] = "apple,banana,cherry".split(",")
  var sentence: str = words.join(" and ")
  print($"Fruits: {sentence}\n")
```

## Next Steps

- [Building](/language/building/) - Compile and run the Sindarin compiler
- [Strings](/language/strings/) - String interpolation and methods
- [Arrays](/language/arrays/) - Array operations and slicing
- [SDK Overview](/sdk/overview/) - Built-in modules for I/O, networking, and more
