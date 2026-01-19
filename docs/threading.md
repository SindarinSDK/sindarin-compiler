# Threading in Sindarin

Sindarin provides OS-level threading with minimal syntax for concurrent execution. The `&` operator spawns threads and the `!` operator synchronizes them. Thread safety is enforced at compile time through pending and frozen state tracking.

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

Thread arguments follow the same `as val` and `as ref` semantics as regular function calls, with one addition: references become **frozen** to the parent thread until synchronization.

### Default Behavior

| Type | Default | Thread Behavior |
|------|---------|-----------------|
| Primitives | Copy (value) | Thread gets copy, no restrictions |
| Arrays | Reference | Parent frozen until sync |
| Strings | Reference | Safe (immutable anyway) |

### Arrays: Frozen Reference

By default, arrays are passed by reference. The parent thread is frozen from writes until sync:

```sindarin
fn sum(data: int[]): int =>
    var total: int = 0
    for n in data => total = total + n
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &sum(numbers)     // reference passed, numbers frozen

// Parent thread restrictions while pending:
numbers[0] = 99                 // COMPILE ERROR: numbers frozen
numbers.push(4)                 // COMPILE ERROR: numbers frozen
print(numbers[0])               // OK - reads allowed
print(numbers.length)           // OK - reads allowed

r!                              // sync releases the freeze

numbers[0] = 99                 // OK - unfrozen
```

### Explicit Copy with `as val`

Use `as val` to pass an independent copy. No freezing occurs:

```sindarin
fn destructive(data: int[] as val): int =>
    var total: int = 0
    while data.length > 0 =>
        total = total + data.pop()  // modifies local copy
    return total

var numbers: int[] = {1, 2, 3}
var r: int = &destructive(numbers)  // thread gets copy

numbers[0] = 99                      // OK - not frozen, thread has own copy
numbers.push(4)                      // OK

r!
print(numbers)                       // {99, 2, 3, 4}
```

### Shared Mutable with `as ref` (Primitives)

Primitives with `as ref` are shared between threads. Parent is frozen until sync:

```sindarin
fn increment(counter: int as ref): void =>
    counter = counter + 1

var count: int as ref = 0
var r1: void = &increment(count)
var r2: void = &increment(count)

count = 5                       // COMPILE ERROR: count frozen by r1 and r2

[r1, r2]!                       // sync both

count = 5                       // OK - unfrozen
print(count)                    // 2 (or 5 after assignment)
```

### Multiple References to Same Array

Multiple threads can share read access to the same frozen array:

```sindarin
var data: int[] = {1, 2, 3}
var r1: int = &sum(data)
var r2: int = &sum(data)       // OK - both read-only

data[0] = 99                    // COMPILE ERROR: frozen by r1 and r2

[r1, r2]!                       // sync releases both freezes
data[0] = 99                    // OK
```

### Summary Table

| Scenario | Parent Read | Parent Write | Thread Read | Thread Write |
|----------|-------------|--------------|-------------|--------------|
| Array (default) | Yes | Frozen | Yes | Yes |
| Array `as val` | Yes | Yes | Yes | Yes (own copy) |
| Primitive | Yes | Yes | Yes | Yes (both have copies) |
| Primitive `as ref` | Yes | Frozen | Yes | Yes |
| String | Yes | N/A | Yes | N/A |

---

## Atomic Variables with `sync`

The `sync` type modifier declares atomic variables that are thread-safe for concurrent access. Operations on `sync` variables use hardware atomic instructions, eliminating race conditions.

### Declaration

```sindarin
var counter: sync int = 0
var total: sync long = 0l
```

The `sync` modifier is allowed on integer types: `int`, `long`, `int32`, `uint`, `uint32`.

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
var counter: sync int = 0

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
var total: sync int = 0

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

### Function Parameters with `sync`

Functions can accept `sync` parameters:

```sindarin
fn safe_increment(counter: sync int as ref): void =>
    counter++

var count: sync int = 0
var t1: void = &safe_increment(count)
var t2: void = &safe_increment(count)
[t1, t2]!

print(count)     // Always 2
```

### When to Use `sync`

| Use Case | Recommendation |
|----------|----------------|
| Shared counter across threads | Use `sync int` |
| Accumulator for parallel results | Use `sync long` |
| Flag or status variable | Use `sync int` or `sync byte` |
| Complex data structure | Use frozen references or external locks |
| Read-only shared data | No `sync` needed (reads are safe) |

### Limitations

- `sync` only applies to integer types (`int`, `long`, `int32`, `uint`, `uint32`, `byte`, `char`)
- Complex multi-variable updates still require external synchronization
- `sync` does not help with read-modify-write sequences spanning multiple statements

For complex synchronization needs beyond atomic counters, consider:
- Freezing shared data structures during thread execution
- Using `as val` to give each thread its own copy
- Designing algorithms to minimize shared mutable state
- Using `lock` blocks for compound operations

---

## Lock Blocks

The `lock` statement provides mutual exclusion for compound operations on `sync` variables. While single operations like `counter++` are atomic, multi-statement operations need explicit locking.

### Syntax

```sindarin
lock(sync_variable) =>
    // critical section
    // only one thread executes this at a time
```

### Basic Example

```sindarin
var counter: sync int = 0

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
var value: sync int = 100

fn halve_if_even(): void =>
    lock(value) =>
        if value % 2 == 0 =>
            value = value / 2  // Multiple statements need lock
```

### Thread-Safe Counter with Lock

Without `lock`, compound operations can interleave:

```sindarin
// UNSAFE: read-modify-write can interleave
var counter: sync int = 0

fn unsafe_increment(): void =>
    var temp = counter      // Thread A reads 0
    temp = temp + 1         // Thread B reads 0
    counter = temp          // Thread A writes 1, Thread B writes 1
                            // Result: 1 (lost update)
```

With `lock`, compound operations are atomic:

```sindarin
// SAFE: entire block is atomic
var counter: sync int = 0

fn safe_increment(): void =>
    lock(counter) =>
        var temp = counter
        temp = temp + 1
        counter = temp      // No interleaving possible
```

### Multi-Threaded Example

```sindarin
var counter: sync int = 0

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
var total: sync int = 0

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

## Thread Arenas

Thread arena management follows the same `shared`, `private`, and default semantics as regular functions.

### Default (Own Arena)

Thread gets its own arena. Return value promoted to caller's arena on sync:

```sindarin
fn build(): str[] =>
    var result: str[] = {"a", "b", "c"}  // thread's arena
    return result                         // promoted on sync

var r: str[] = &build()
r!                        // result promoted to caller's arena
print(r)                  // safe - lives in caller's arena
```

### `shared` (Caller's Arena)

Thread allocates directly in caller's arena. Parent's writes frozen until sync:

```sindarin
fn build() shared: str[] =>
    var result: str[] = {"a", "b", "c"}  // caller's arena
    return result                         // no promotion needed

var data: str[] = {}
var r: str[] = &build()

data.push("x")            // COMPILE ERROR: caller's arena frozen
r!
data.push("x")            // OK - unfrozen
```

### `private` (Isolated Arena)

Thread has isolated arena. Only primitives can be returned:

```sindarin
fn count_lines(path: str) private: int =>
    var contents: str = read_file(path)  // thread's private arena
    var lines: str[] = contents.split("\n")
    return lines.length                   // primitive escapes, rest freed

var r: int = &count_lines("big.txt")
r!
print(r)                  // just the count, file contents already freed
```

```sindarin
// COMPILE ERROR: can't return array from private function
fn bad(path: str) private: str[] =>
    return read_file(path).split("\n")
```

### Arena Summary

| Function Type | Thread Arena | Return Behavior | Parent Arena |
|---------------|--------------|-----------------|--------------|
| default | Own arena | Promoted on `!` | Not frozen |
| `shared` | Caller's arena | No promotion | Frozen until `!` |
| `private` | Isolated arena | Primitives only | Not frozen |

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

// Safe: all threads only read the frozen array
[count1, count2, count3]!

print($"1s: {count1}, 2s: {count2}, 3s: {count3}\n")
```

---

## Thread Safety Model

Sindarin's threading model prevents data races through compile-time enforcement.

### Safety Guarantees

| Protection | Mechanism |
|------------|-----------|
| Write-write races on arrays | Frozen while pending |
| Read-write races on arrays | Caller reads allowed, writes frozen |
| Use-before-ready on thread results | Compile error on pending access |
| Lost updates | Sync required before reassignment |

### User Responsibilities

The following scenarios are not automatically prevented:

- Multiple threads reading shared data while another writes via `as ref`
- External effects (file I/O, network) are not synchronized
- Race conditions in fire-and-forget threads without sync

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
| `&fn()` | Fire and forget (void only) |
| `&fn()!` | Spawn and wait (void) |
| `var x: sync int = 0` | Atomic integer variable |
| `x++`, `x--` | Atomic increment/decrement (on sync) |
| `x += n`, `x -= n` | Atomic add/subtract (on sync) |
| `x *= n`, `x /= n`, `x %= n` | Atomic mul/div/mod via CAS (on sync) |
| `lock(sync_var) => ...` | Mutual exclusion block for sync variable |

### Compiler Rules

| Rule | |
|------|---|
| Access unsynchronized variable | Compile error |
| Reassign unsynchronized variable | Compile error |
| Write to frozen array | Compile error |
| Write to frozen `as ref` primitive | Compile error |
| After `!` | Variable is normal, can access/reassign |
| `sync` on non-integer type | Compile error |
| `lock` on non-sync variable | Compile error |

---

## Implementation Notes

### Code Generation

The `&` operator generates pthread creation:

```sindarin
var r: int = &add(1, 2)
```

```c
// Generated C
typedef struct {
    int arg_a;
    int arg_b;
    int* result;
    bool done;
    bool has_panic;
    char* panic_message;
} add_thread_args;

void* add_thread_wrapper(void* arg) {
    add_thread_args* args = (add_thread_args*)arg;
    *args->result = add(args->arg_a, args->arg_b);
    args->done = true;
    return NULL;
}

// At spawn site
add_thread_args args = {1, 2, &r, false, false, NULL};
pthread_t thread;
pthread_create(&thread, NULL, add_thread_wrapper, &args);
```

### Synchronization Implementation

The `!` operator generates pthread_join:

```sindarin
r!
```

```c
// Generated C
pthread_join(thread, NULL);
if (args.has_panic) {
    rt_panic(args.panic_message);
}
```

### C Runtime Structures

```c
typedef struct RtThreadHandle {
    pthread_t thread;
    void* args;
    bool synced;
    bool has_panic;
    char* panic_message;
} RtThreadHandle;

// Thread-local arena for each spawned thread
__thread RtArena* rt_thread_arena = NULL;
```

### Arena Integration

Thread arenas follow the function arena model:

```c
void* thread_wrapper(void* arg) {
    // Create thread-local arena (default mode)
    RtArena* arena = rt_arena_create();
    rt_thread_arena = arena;

    // Execute function
    thread_args* args = (thread_args*)arg;
    args->result = target_function(args->params);

    // Promote return value to parent arena
    if (needs_promotion) {
        args->result = rt_arena_promote(args->parent_arena, args->result);
    }

    // Destroy thread arena
    rt_arena_destroy(arena);
    return NULL;
}
```

---

## Threading Notes

The following features are fully supported:

1. **Nested thread spawns** - Spawning threads from within spawned threads works correctly
2. **Function parameters in threads** - Passing function types (including lambdas with captured state) as arguments to thread-spawned functions is supported
3. **Closures with mutable state** - Lambda expressions capturing and modifying mutable state (including arrays and primitives) work correctly across thread boundaries

**Race conditions:** When multiple threads modify the same mutable state without synchronization, the results are non-deterministic. Use `sync` variables and `lock` blocks for thread-safe access to shared mutable state.

---

## See Also

- [Memory](memory.md) - Arena memory management, `as ref`, `as val`, `shared`, `private`
- [Arrays](arrays.md) - Array operations and frozen semantics
- [SDK I/O documentation](sdk/io/readme.md) - File I/O operations
