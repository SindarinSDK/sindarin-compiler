---
title: "Threading"
description: "Concurrency with spawn, sync, and shared memory"
permalink: /language/threading/
---

Sindarin provides OS-level threading with minimal syntax for concurrent execution. The `&` operator spawns threads and the `!` operator synchronizes them. Thread safety is enforced at compile time through pending state tracking.

## Spawning Threads (`&`)

The `&` operator spawns a new OS thread to execute a function call. The result variable enters a "pending" state until synchronized.

### Basic Spawn

```sindarin
fn compute(n: int): int =>
    // expensive computation
    return n * n

var result: int = &compute(42)   // thread starts, result is pending
// ... do other work while thread runs ...
result!                          // synchronize
print(result)                    // 1764
```

### Fire and Forget

Void function calls with `&` run independently:

```sindarin
fn cleanup(): void =>
    // slow background work
    print("Cleaning up...\n")

&cleanup()   // main continues immediately
             // thread runs in background
```

Fire-and-forget threads are terminated when the process exits. If `main` returns or panics, all threads terminate immediately.

---

## Synchronizing Threads (`!`)

The `!` operator blocks until a pending variable is populated by its thread.

### Basic Synchronization

```sindarin
var r: int = &add(1, 2)
// ... do other work while thread runs ...
r!                          // block until complete
print(r)                    // safe to use
```

### Immediate Synchronization

Combine spawn and sync for blocking calls:

```sindarin
var r: int = &add(1, 2)!    // spawn and wait immediately
print(r)                     // already synchronized

&doWork()!                   // spawn void and wait
```

### Sync in Expressions

The `!` operator syncs and returns the value, allowing inline use:

```sindarin
var x: int = &add(1, 2)
var y: int = &add(3, 4)

// Sync inline and use values
var z: int = x! + y!        // z = 3 + 7 = 10
```

After `!` is used, the variable is synchronized and can be accessed normally:

```sindarin
var x: int = &add(1, 2)
var sum: int = x! + x + x   // first x! syncs, subsequent x reads value
                            // sum = 3 + 3 + 3 = 9
```

### Multiple Thread Synchronization

Sync multiple threads at once with array syntax:

```sindarin
var r1: int = &add(1, 2)
var r2: int = &add(3, 4)
var r3: int = &multiply(5, 6)

// Wait for all to complete
[r1, r2, r3]!

// Now all are synchronized
print(r1 + r2 + r3)
```

Individual synchronization is also valid:

```sindarin
r1!
r2!
r3!
```

---

## Detaching Threads (`~`)

The `~` operator detaches a pending thread, releasing scope ownership. The thread continues running independently — it is not joined when the variable goes out of scope.

### Basic Detach

```sindarin
var handle: int = &startServer(config)
handle~                         // detach: thread runs independently
// handle's scope exits without blocking
```

### Detach in Loops

Without `~`, threads spawned in loops are auto-joined at the end of each iteration, serializing execution. Detach solves this:

```sindarin
fn handleClient(conn: Connection): int =>
    // long-running connection handler
    return 0

fn serve(listener: Listener): void =>
    while true =>
        var conn: Connection = listener.accept()
        var handler: int = &handleClient(conn)
        handler~    // detach: loop continues immediately
        // without ~, auto-join would block until handleClient returns
```

### Detach vs Fire-and-Forget

| Pattern | Syntax | Use Case |
|---------|--------|----------|
| Fire-and-forget | `&fn()` | Void functions, no handle needed |
| Detach | `var h = &fn(); h~` | Need handle temporarily, then release |

Fire-and-forget is shorthand for spawn + immediate detach. Use `~` when you need the handle before detaching (e.g., logging, conditional detach).

### After Detach

A detached variable cannot be synchronized:

```sindarin
var r: int = &compute()
r~
r!              // COMPILE ERROR: cannot sync detached thread
```

The thread's return value is discarded. If you need the result, use `!` (sync) instead of `~` (detach).

---

## Compiler Enforcement

The compiler tracks pending state and enforces synchronization before use.

### Access Before Sync

```sindarin
var r: int = &add(1, 2)
print(r)                    // COMPILE ERROR: r is unsynchronized
r!
print(r)                    // OK
```

### Reassignment Before Sync

```sindarin
var r: int = &add(1, 2)
r = &add(3, 4)              // COMPILE ERROR: r is unsynchronized
r!
r = &add(3, 4)              // OK - can reassign after sync
```

Preventing reassignment before sync avoids accidental thread orphaning and race conditions. Use separate variables for concurrent operations:

```sindarin
// Correct: separate variables
var r1: int = &add(1, 2)
var r2: int = &add(3, 4)
[r1, r2]!
```

---

## Memory Semantics

Thread arguments follow the same `as val` and `as ref` semantics as regular function calls.

### Default Behavior

| Type | Default | Thread Behavior |
|------|---------|-----------------|
| Primitives | Copy (value) | Thread gets copy |
| Arrays | Reference | Thread gets reference to same array |
| Strings | Reference | Safe (immutable anyway) |

### Arrays: Reference by Default

By default, arrays are passed by reference:

```sindarin
fn sum(data: int[]): int =>
    var total: int = 0
    for n in data => total = total + n
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &sum(numbers)     // reference passed

r!                              // sync

print(numbers[0])               // OK
```

### Explicit Copy with `as val`

Use `as val` to pass an independent copy:

```sindarin
fn destructive(data: int[] as val): int =>
    var total: int = 0
    while data.length > 0 =>
        total = total + data.pop()  // modifies local copy
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &destructive(numbers)  // thread gets copy

numbers[0] = 99                      // OK - thread has own copy
numbers.push(4)                      // OK

r!
print(numbers)                       // {99, 2, 3, 4}
```

### Shared Mutable with `as ref` (Primitives)

Primitives with `as ref` are shared between threads:

```sindarin
fn increment(counter: int as ref): void =>
    counter = counter + 1

var count: int as ref = 0
var r1: void = &increment(count)
var r2: void = &increment(count)

[r1, r2]!                       // sync both

print(count)                    // Result depends on execution order (race condition)
```

**Note:** For thread-safe modifications, use `sync` variables instead:

```sindarin
sync var count: int = 0
var r1: void = &increment(count)
var r2: void = &increment(count)
[r1, r2]!
print(count)                    // Always 2
```

### Multiple References to Same Array

Multiple threads can share access to the same array:

```sindarin
var data: int[] = {1, 2, 3}
var r1: int = &sum(data)
var r2: int = &sum(data)       // both read same array

[r1, r2]!
```

**Warning:** Sharing an array across threads is safe only when all threads read. If any thread modifies the shared array, use `lock` blocks to prevent data races.

### Summary Table

| Scenario | Parent Access | Thread Access |
|----------|--------------|---------------|
| Array (default) | Yes | Yes (shared reference) |
| Array `as val` | Yes | Yes (own copy) |
| Primitive | Yes | Yes (both have copies) |
| Primitive `as ref` | Yes | Yes (shared) |
| String | Yes | Yes (immutable) |

---

## Atomic Variables with `sync`

The `sync` modifier declares atomic variables that are thread-safe for concurrent access. Operations on `sync` variables use hardware atomic instructions, eliminating race conditions.

### Declaration

```sindarin
sync var counter: int = 0
sync var total: long = 0l
```

The `sync` modifier is placed before the `var` keyword. It is allowed on integer types: `int`, `long`, `int32`, `uint`, `uint32`, `byte`, `char`.

You can also combine `sync` with `static` for module-level atomic variables:

```sindarin
static sync var globalCounter: int = 0
sync static var anotherCounter: int = 0  // Order doesn't matter
```

### Atomic Operations

The following operations on `sync` variables are atomic:

| Operation | Example | Generated Code |
|-----------|---------|----------------|
| Increment | `counter++` | `__atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST)` |
| Decrement | `counter--` | `__atomic_fetch_sub(&counter, 1, __ATOMIC_SEQ_CST)` |
| Add-assign | `counter += 5` | `__atomic_fetch_add(&counter, 5, __ATOMIC_SEQ_CST)` |
| Sub-assign | `counter -= 3` | `__atomic_fetch_sub(&counter, 3, __ATOMIC_SEQ_CST)` |

### Thread-Safe Counter Example

Without `sync`, concurrent increments can lose updates:

```sindarin
// UNSAFE: Race condition
var counter: int = 0

fn increment(): void =>
    counter++    // Not atomic - can lose updates

var t1: void = &increment()
var t2: void = &increment()
[t1, t2]!

print(counter)   // Could be 1 or 2 (race condition)
```

With `sync`, all updates are atomic:

```sindarin
// SAFE: Atomic operations
sync var counter: int = 0

fn increment(): void =>
    counter++    // Atomic increment

var t1: void = &increment()
var t2: void = &increment()
[t1, t2]!

print(counter)   // Always 2
```

### Compound Assignment with `sync`

All compound assignments are atomic on `sync` variables:

```sindarin
sync var total: int = 0

fn add_value(n: int): void =>
    total += n   // Atomic add

var t1: void = &add_value(10)
var t2: void = &add_value(20)
var t3: void = &add_value(30)
[t1, t2, t3]!

print(total)     // Always 60
```

| Operation | Example | Implementation |
|-----------|---------|----------------|
| `+=` | `counter += 5` | `__atomic_fetch_add` |
| `-=` | `counter -= 3` | `__atomic_fetch_sub` |
| `*=` | `counter *= 2` | Compare-and-swap loop |
| `/=` | `counter /= 4` | Compare-and-swap loop |
| `%=` | `counter %= 3` | Compare-and-swap loop |

For `*=`, `/=`, and `%=`, a CAS (compare-and-swap) loop is used since there are no direct atomic builtins for these operations. The CAS loop ensures atomicity by retrying if another thread modified the value.

### Module-Level Sync Variables

For shared counters accessed by multiple functions, declare sync variables at module level:

```sindarin
sync var count: int = 0

fn safe_increment(): void =>
    count++

var t1: void = &safe_increment()
var t2: void = &safe_increment()
[t1, t2]!

print(count)     // Always 2
```

**Note:** The `sync` modifier is only valid on variable declarations, not on function parameters.

### When to Use `sync`

| Use Case | Recommendation |
|----------|----------------|
| Shared counter across threads | Use `sync var x: int` |
| Accumulator for parallel results | Use `sync var x: long` |
| Flag or status variable | Use `sync var x: int` or `sync var x: byte` |
| Complex data structure | Use `lock` blocks or external synchronization |
| Read-only shared data | No `sync` needed (reads are safe) |

### Limitations

- `sync` only applies to integer types (`int`, `long`, `int32`, `uint`, `uint32`, `byte`, `char`)
- Complex multi-variable updates still require external synchronization
- `sync` does not help with read-modify-write sequences spanning multiple statements

For complex synchronization needs beyond atomic counters, consider:
- Using `lock` blocks for compound operations
- Using `as val` to give each thread its own copy
- Designing algorithms to minimize shared mutable state
- Using `lock` blocks for compound operations

---

## Lock Blocks

The `lock` statement provides mutual exclusion for compound operations on `sync` variables. The lock target must be a `sync var` — locking a non-sync variable is a compile error. While single operations like `counter++` are atomic, multi-statement operations need explicit locking.

### Syntax

```sindarin
lock(sync_variable) =>
    // critical section
    // only one thread executes this at a time
```

### Basic Example

```sindarin
sync var counter: int = 0

fn increment_twice(): void =>
    lock(counter) =>
        counter = counter + 1
        counter = counter + 1  // Both updates are atomic together
```

### When to Use Lock Blocks

Use `lock` when you need to:
- Perform multiple operations atomically together
- Read-modify-write with complex logic involving multiple statements

```sindarin
sync var value: int = 100

fn halve_if_even(): void =>
    lock(value) =>
        if value % 2 == 0 =>
            value = value / 2  // Multiple statements need lock
```

### Thread-Safe Counter with Lock

Without `lock`, compound operations can interleave:

```sindarin
// UNSAFE: read-modify-write can interleave
sync var counter: int = 0

fn unsafe_increment(): void =>
    var temp = counter      // Thread A reads 0
    temp = temp + 1         // Thread B reads 0
    counter = temp          // Thread A writes 1, Thread B writes 1
                            // Result: 1 (lost update)
```

With `lock`, compound operations are atomic:

```sindarin
// SAFE: entire block is atomic
sync var counter: int = 0

fn safe_increment(): void =>
    lock(counter) =>
        var temp = counter
        temp = temp + 1
        counter = temp      // No interleaving possible
```

### Multi-Threaded Example

```sindarin
sync var counter: int = 0

fn increment_100_times(): int =>
    for i in 1..101 =>
        lock(counter) =>
            counter = counter + 1
    return 1

fn main(): void =>
    var t1: int = &increment_100_times()
    var t2: int = &increment_100_times()
    var t3: int = &increment_100_times()
    var t4: int = &increment_100_times()

    var r1 = t1!
    var r2 = t2!
    var r3 = t3!
    var r4 = t4!

    print($"Final counter: {counter}\n")  // Always 400
```

### Nested Operations

`lock` blocks can contain any statements:

```sindarin
sync var total: int = 0

fn add_sum(values: int[]): void =>
    lock(total) =>
        for v in values =>
            total += v
```

### Lock vs Atomic Operations

| Operation | Use | Example |
|-----------|-----|---------|
| Single increment | Atomic | `counter++` |
| Single add/sub/mul/div/mod | Atomic | `counter += 5`, `counter *= 2` |
| Multiple operations | Lock | `lock(x) => x = x * 2; x += 1` |
| Read-modify-write sequence | Lock | `lock(x) => if x > 0 => x--` |

### Restrictions

- Lock expression must be a `sync` variable
- Non-sync variables cannot be locked

```sindarin
var normal: int = 0
lock(normal) =>     // COMPILE ERROR: not a sync variable
    normal++
```

---

## Error Handling

Thread panics propagate on sync. If you don't sync, the panic is lost.

### Panic Propagation

```sindarin
fn might_fail(x: int): int =>
    if x < 0 =>
        panic("negative value")
    return x * 2

var r: int = &might_fail(-1)
// ... thread panics, but we don't know yet ...
r!                            // PANIC propagates here
print(r)                      // never reached
```

### Fire and Forget: Panic Lost

```sindarin
fn risky(): void =>
    panic("something went wrong")

&risky()        // fire and forget
                // panic happens in background
                // no sync = panic lost
print("done")   // still executes
```

### Multiple Thread Panics

If multiple threads panic, the first sync propagates its panic:

```sindarin
var r1: int = &might_fail(-1)
var r2: int = &might_fail(-2)

r1!             // PANIC from r1
r2!             // never reached
```

With array sync, first completed panic propagates:

```sindarin
var r1: int = &might_fail(-1)
var r2: int = &might_fail(-2)

[r1, r2]!       // PANIC from whichever fails first
```

---

## Common Patterns

### Parallel Computation

```sindarin
fn compute_square(x: int): int =>
    return x * x

var r1: int = &compute_square(5)
var r2: int = &compute_square(10)
r1!
r2!
print($"Squared values: {r1}, {r2}\n")
```

### Parallel File Reads

```sindarin
var f1: str = &TextFile.readAll("file1.txt")
var f2: str = &TextFile.readAll("file2.txt")
var f3: str = &TextFile.readAll("file3.txt")

[f1, f2, f3]!

print($"Total: {f1.length + f2.length + f3.length} bytes\n")
```

### Background Write

```sindarin
// Fire and forget - write happens in background
&TextFile.writeAll("backup.txt", data)

// Continue with other work...
```

### Worker Pool Pattern

```sindarin
fn process(item: str): str =>
    // expensive processing
    return $"processed: {item}"

fn main(): void =>
    var items: str[] = {"a", "b", "c", "d", "e"}

    // Spawn workers for each item
    var r1: str = &process(items[0])
    var r2: str = &process(items[1])
    var r3: str = &process(items[2])
    var r4: str = &process(items[3])
    var r5: str = &process(items[4])

    // Wait for all to complete
    [r1, r2, r3, r4, r5]!

    print($"{r1}\n{r2}\n{r3}\n{r4}\n{r5}\n")
```

### Read-Only Shared Data

```sindarin
fn count_matches(data: int[], target: int): int =>
    var count: int = 0
    for n in data =>
        if n == target => count = count + 1
    return count

var data: int[] = {1, 2, 3, 2, 1, 2, 3}
var count1: int = &count_matches(data, 1)
var count2: int = &count_matches(data, 2)
var count3: int = &count_matches(data, 3)

// Safe: all threads only read the shared array
[count1, count2, count3]!

print($"1s: {count1}, 2s: {count2}, 3s: {count3}\n")
```

---

## Thread Safety Model

Sindarin's threading model prevents certain data races through compile-time enforcement.

### Safety Guarantees

| Protection | Mechanism |
|------------|-----------|
| Use-before-ready on thread results | Compile error on pending access |
| Lost updates on sync variables | Atomic operations |
| Reassignment of pending variables | Compile error |

### User Responsibilities

The following scenarios require user attention:

- Race conditions when multiple threads modify shared arrays or `as ref` primitives
- External effects (file I/O, network) are not synchronized
- Race conditions in fire-and-forget threads without sync
- Use `sync` variables and `lock` blocks for thread-safe shared mutable state

---

## Quick Reference

### Syntax

| Syntax | Behavior |
|--------|----------|
| `var r: T = &fn()` | Spawn thread, r is pending |
| `r!` | Block until synced, returns value |
| `x! + y!` | Sync in expressions |
| `[r1, r2, ...]!` | Block until all are synchronized |
| `var r: T = &fn()!` | Spawn and wait immediately |
| `r~` | Detach thread, runs independently |
| `&fn()` | Fire and forget (void only) |
| `&fn()!` | Spawn and wait (void) |
| `sync var x: int = 0` | Atomic integer variable |
| `x++`, `x--` | Atomic increment/decrement (on sync) |
| `x += n`, `x -= n` | Atomic add/subtract (on sync) |
| `x *= n`, `x /= n`, `x %= n` | Atomic mul/div/mod via CAS (on sync) |
| `lock(sync_var) => ...` | Mutual exclusion block for sync variable |

### Compiler Rules

| Rule | |
|------|---|
| Access pending variable | Compile error |
| Reassign pending variable | Compile error |
| After `!` | Variable is synchronized, can access/reassign |
| After `~` | Variable is detached, cannot sync |
| Sync detached variable | Compile error |
| Detach non-pending variable | Compile error |
| `sync` on non-integer type | Compile error |
| `lock` on non-sync variable | Compile error |

---

## Implementation Notes

### Code Generation

The `&` operator generates a `__thread_wrapper_N__` function and a `SnThread` handle:

```sindarin
var r: int = &compute()
```

```c
// Generated C
static void *__thread_wrapper_0__(void *arg) {
    SnThread *__th__ = (SnThread *)arg;

    long long __result__ = __sn__compute();
    if (!__th__->result) __th__->result = calloc(1, sizeof(long long));
    *(long long *)__th__->result = __result__;
    return NULL;
}

// At spawn site
long long __sn__r = 0; sn_auto_thread SnThread * __sn__r__th__ = ({
    SnThread *__th__ = sn_thread_create();
    __th__->result_size = sizeof(long long);
    pthread_create(&__th__->thread, NULL, __thread_wrapper_0__, __th__);
    __th__;
});
```

### Synchronization Implementation

The `!` operator generates `sn_thread_join` and extracts the result:

```sindarin
var result: int = r!
```

```c
// Generated C
long long __sn__result = ({
    sn_auto_thread SnThread *__sync_th__ = __sn__r__th__; __sn__r__th__ = NULL;
    if (__sync_th__) { sn_thread_join(__sync_th__); __sn__r = *(long long *)__sync_th__->result; }
    __sn__r; });
```

### C Runtime Structures

The `SnThread` struct (from `sn_minimal.h`) holds the thread handle and result:

```c
typedef struct {
    pthread_t thread;
    void *result;
    size_t result_size;
    int joined;
    int detached;
} SnThread;

// Cleanup attribute — automatically joins and frees when the variable goes out of scope
#define sn_auto_thread __attribute__((cleanup(sn_cleanup_thread)))
```

### Thread Cleanup

Thread wrappers use standard C cleanup via the `sn_auto_thread` attribute macro from the minimal runtime. For fire-and-forget threads, the wrapper calls `free()` directly on the `SnThread` struct after the function returns. For joinable threads, the joining side handles cleanup. No arena-level teardown is needed — only `free()` and the `sn_auto_*` cleanup macros from `sn_minimal.h`.

### Detach Implementation

The `~` operator generates code that:
1. Moves the `SnThread*` from the companion variable to a local
2. NULLs the companion variable (prevents `sn_auto_thread` cleanup from joining)
3. Sets `__th__->detached = 1` on the thread struct
4. Calls `pthread_detach()` to allow OS cleanup when the thread exits

All thread wrappers include a self-cleanup check at the end: `if (__th__->detached) { free(__th__->result); free(__th__); }`. This ensures the `SnThread` struct is freed by the worker thread itself when nobody else will join it.

---

## Threading Notes

The following features are fully supported:

1. **Nested thread spawns** - Spawning threads from within spawned threads works correctly
2. **Function parameters in threads** - Passing function types (including lambdas with captured state) as arguments to thread-spawned functions is supported
3. **Closures with mutable state** - Lambda expressions capturing and modifying mutable state (including arrays and primitives) work correctly across thread boundaries

**Race conditions:** When multiple threads modify the same mutable state without synchronization, the results are non-deterministic. Use `sync` variables and `lock` blocks for thread-safe access to shared mutable state.

---

## See Also

- [Memory](memory.md) - `as ref`, `as val` ownership semantics
- [Arrays](arrays.md) - Array operations
- SDK I/O documentation is available in the sindarin-pkg-sdk repository