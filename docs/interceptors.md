# Function Interceptors in Sindarin

Interceptors provide a way to intercept and modify the behavior of user-defined function calls at runtime. This enables debugging, profiling, mocking, logging, and aspect-oriented programming patterns.

## Overview

When interceptors are registered, all calls to user-defined functions pass through the interceptor chain before (optionally) executing the original function. Interceptors can:

- Inspect function names and arguments
- Modify arguments before the call
- Replace return values
- Skip the original function entirely
- Log or profile function calls

```sindarin
fn myInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    print($"Called: {name}\n")
    var result: any = 42  // Return a fixed value
    return result

fn main =>
    Interceptor.register(myInterceptor)

    var x: int = add(1, 2)  // Intercepted! Returns 42 instead of 3
    print($"Result: {x}\n")  // Prints: Result: 42
```

## The Interceptor Namespace

### Static Methods

| Method | Signature | Description |
|--------|-----------|-------------|
| `register` | `(handler: fn(str, any[], fn(): any): any): void` | Register an interceptor for all functions |
| `registerWhere` | `(handler, pattern: str): void` | Register an interceptor with pattern matching |
| `clearAll` | `(): void` | Remove all registered interceptors |
| `count` | `(): int` | Get the number of registered interceptors |
| `isActive` | `(): bool` | Check if currently inside an interceptor call |

### Interceptor Handler Signature

An interceptor handler receives three arguments:

```sindarin
fn handler(name: str, args: any[], continue_fn: fn(): any): any =>
    // name: The function being called (e.g., "add", "greet")
    // args: Array of boxed arguments
    // continue_fn: Callback to invoke the original function or next interceptor
    // returns: The value to return to the caller
```

## Basic Usage

### Registering an Interceptor

```sindarin
var call_count: int = 0

fn loggingInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    call_count = call_count + 1
    print($"[LOG] Function '{name}' called\n")

    // Call the original function and return its result
    var result: any = continue_fn()
    return result

fn add(a: int, b: int): int =>
    return a + b

fn main =>
    // Direct call - no interception
    var sum: int = add(1, 2)
    print($"Direct: {sum}\n")  // Prints: Direct: 3

    // Register interceptor
    Interceptor.register(loggingInterceptor)

    // Intercepted call
    sum = add(10, 20)
    print($"Intercepted: {sum}\n")  // Prints: Intercepted: 0
    print($"Calls logged: {call_count}\n")  // Prints: Calls logged: 1
```

### Clearing Interceptors

```sindarin
Interceptor.register(myInterceptor)
print($"Count: {Interceptor.count()}\n")  // Prints: Count: 1

Interceptor.clearAll()
print($"Count: {Interceptor.count()}\n")  // Prints: Count: 0

// After clearing, functions execute normally
var result: int = add(1, 2)  // Returns 3, not intercepted
```

### Pattern Matching with registerWhere

Register interceptors that only match specific function name patterns:

```sindarin
fn getUserInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    print($"User function called: {name}\n")
    var result: any = nil
    return result

// Only intercept functions starting with "get"
Interceptor.registerWhere(getUserInterceptor, "get*")

getUser(123)      // Intercepted
setUser(123)      // NOT intercepted - doesn't match pattern
getUserById(456)  // Intercepted
```

Pattern syntax:
- `"*"` or `nil` - Match all functions
- `"prefix*"` - Match functions starting with prefix
- `"*suffix"` - Match functions ending with suffix
- `"pre*suf"` - Match functions with prefix and suffix
- `"exact"` - Exact match only

### Checking Interceptor State

```sindarin
fn myInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    if Interceptor.isActive() =>
        print("We are inside an interceptor!\n")
    var result: any = 0
    return result

fn main =>
    // Outside interceptor
    if !Interceptor.isActive() =>
        print("Not in interceptor\n")

    Interceptor.register(myInterceptor)
    someFunction()  // Inside handler, isActive() returns true
```

## Working with Arguments

### Inspecting Arguments

Arguments are passed as `any[]`, requiring type checking and unboxing:

```sindarin
fn debugInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    print($"Function: {name}\n")
    print($"Arg count: {args.length}\n")

    for var i: int = 0; i < args.length; i++ =>
        var arg: any = args[i]
        if arg is int =>
            var val: int = arg as int
            print($"  arg[{i}]: int = {val}\n")
        else if arg is str =>
            var val: str = arg as str
            print($"  arg[{i}]: str = \"{val}\"\n")
        else if arg is bool =>
            var val: bool = arg as bool
            print($"  arg[{i}]: bool = {val}\n")

    var result: any = 0
    return result
```

### Returning Type-Appropriate Values

The interceptor must return an `any` value that matches what the caller expects:

```sindarin
fn mockingInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    // Return appropriate types based on function name
    if name == "getCount" =>
        var result: any = 42
        return result
    else if name == "getName" =>
        var result: any = "Mocked Name"
        return result
    else if name == "isEnabled" =>
        var result: any = true
        return result

    // Default: return 0 for numeric functions
    var result: any = 0
    return result
```

## Functions with `as ref` Parameters

Interceptors fully support functions with `as ref` (by-reference) parameters. Modified values propagate back to the caller:

```sindarin
fn increment(x: int as ref) =>
    x = x + 1

fn refInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    print($"Intercepting {name}\n")
    // The ref parameter modification still works through interception
    var result: any = 0
    return result

fn main =>
    var value: int = 10

    // Direct call
    increment(value)
    print($"After direct: {value}\n")  // Prints: 11

    // Intercepted call - ref semantics preserved
    Interceptor.register(refInterceptor)
    increment(value)
    print($"After intercepted: {value}\n")  // Value may be modified by thunk
```

## What Gets Intercepted

### Intercepted

- User-defined functions (declared with `fn`)
- Functions with any parameter types (including `as ref`)
- Functions returning any non-pointer type
- Functions in namespaces (via `import "module" as ns`)

### Not Intercepted

- Native functions (declared with `native fn`)
- Built-in methods (string methods, array methods)
- Static type methods (e.g., `TextFile.readAll()`, `Random.int()`)
- Functions with pointer parameters or return types
- Lambda expressions

## Performance Considerations

### Fast Path

When no interceptors are registered (`Interceptor.count() == 0`), function calls execute with minimal overhead - just a single integer comparison.

```c
// Generated C code (simplified)
if (__rt_interceptor_count > 0) {
    // Boxing, thunk setup, interceptor chain...
} else {
    result = originalFunction(args);  // Direct call
}
```

### Overhead When Active

When interceptors are registered:
1. Arguments are boxed into `RtAny` values
2. A thunk function is prepared for potential continuation
3. The interceptor chain is traversed
4. Results are unboxed back to concrete types

For performance-critical code, use `Interceptor.clearAll()` when interception is no longer needed.

## Use Cases

### Logging and Debugging

```sindarin
var log: str[] = {}

fn logInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    log.push($"Called: {name} with {args.length} args")
    var result: any = 0
    return result

fn main =>
    Interceptor.register(logInterceptor)

    // Run application...
    processData()
    saveResults()

    // Print call log
    for entry in log =>
        print($"{entry}\n")
```

### Mocking for Tests

```sindarin
fn mockDatabase(name: str, args: any[], continue_fn: fn(): any): any =>
    if name == "fetchUser" =>
        var result: any = "MockUser"
        return result
    else if name == "saveUser" =>
        var result: any = true
        return result
    var result: any = nil
    return result

fn runTests =>
    Interceptor.register(mockDatabase)

    // Tests now use mocked database functions
    var user: str = fetchUser(123)
    assert(user == "MockUser", "Should return mock user")

    Interceptor.clearAll()
```

### Profiling

```sindarin
var call_counts: int[] = {}
var function_names: str[] = {}

fn profilerInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    var idx: int = function_names.indexOf(name)
    if idx < 0 =>
        function_names.push(name)
        call_counts.push(1)
    else =>
        call_counts[idx] = call_counts[idx] + 1

    var result: any = 0
    return result

fn printProfile =>
    print("Function Call Counts:\n")
    for var i: int = 0; i < function_names.length; i++ =>
        print($"  {function_names[i]}: {call_counts[i]}\n")
```

## Limitations

1. **Pointer types excluded**: Functions with pointer parameters or return types cannot be intercepted due to boxing limitations.

2. **Native functions excluded**: Functions declared with `native fn` bypass interception.

3. **Single return value**: Interceptors can only return a single value, not multiple return values.

## Thread Safety

The interceptor registry is thread-safe:
- Registration and clearing use mutex locks
- The interceptor count is volatile for visibility
- Per-thread context is used for nested interception

However, interceptor handlers themselves should be thread-safe if used in multi-threaded code.

## Example: Complete Interceptor

```sindarin
var intercepted_calls: int = 0

fn comprehensiveInterceptor(name: str, args: any[], continue_fn: fn(): any): any =>
    intercepted_calls = intercepted_calls + 1

    // Log the call
    print($"[{intercepted_calls}] {name}(")
    for var i: int = 0; i < args.length; i++ =>
        if i > 0 =>
            print(", ")
        var arg: any = args[i]
        if arg is int =>
            print($"{arg as int}")
        else if arg is str =>
            print($"\"{arg as str}\"")
        else =>
            print("?")
    print(")\n")

    // Return based on expected return type
    if name == "getMessage" =>
        var result: any = "Intercepted message"
        return result
    else if name == "getCount" =>
        var result: any = 999
        return result

    // Default numeric return
    var result: any = 0
    return result

fn getMessage(): str =>
    return "Original message"

fn getCount(): int =>
    return 42

fn main =>
    print("=== Without Interceptor ===\n")
    print($"Message: {getMessage()}\n")
    print($"Count: {getCount()}\n")

    print("\n=== With Interceptor ===\n")
    Interceptor.register(comprehensiveInterceptor)
    print($"Message: {getMessage()}\n")
    print($"Count: {getCount()}\n")

    print($"\nTotal intercepted calls: {intercepted_calls}\n")
```

Output:
```
=== Without Interceptor ===
Message: Original message
Count: 42

=== With Interceptor ===
[1] getMessage()
Message: Intercepted message
[2] getCount()
Count: 999

Total intercepted calls: 2
```

## See Also

- [Lambdas](lambdas.md) - Lambda expressions used in interceptor handlers
- [Memory](memory.md) - Memory management in interceptors
