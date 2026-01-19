# Lambdas and Closures

Sindarin supports first-class functions through lambdas (anonymous functions) and closures (lambdas that capture variables from their enclosing scope).

## Lambda Basics

### Syntax

Lambdas are defined using the `fn` keyword followed by parameters, an optional return type, and an arrow `=>` introducing the body:

```sindarin
// Full syntax with explicit types
fn(parameter: type, ...): returnType => expression

// With type inference (when assigned to typed variable)
fn(parameter, ...) => expression
```

### Basic Examples

```sindarin
// Lambda with explicit types
var add: fn(int, int): int = fn(a: int, b: int): int => a + b
print($"add(3, 4) = {add(3, 4)}\n")  // 7

// Lambda with single parameter
var double_it: fn(int): int = fn(x: int): int => x * 2
print($"double_it(5) = {double_it(5)}\n")  // 10

// Lambda with no parameters
var get_value: fn(): int = fn(): int => 42
print($"get_value() = {get_value()}\n")  // 42
```

### Multi-Statement Bodies

Lambdas can have multiple statements with explicit `return`:

```sindarin
var max_of: fn(int, int): int = fn(a: int, b: int): int =>
    if a > b =>
        return a
    return b
```

## Type Inference

When a lambda is assigned to a variable with an explicit function type, parameter types can be inferred:

```sindarin
// Full type on variable, inferred in lambda
var add: fn(int, int): int = fn(a, b) => a + b
var triple: fn(int): int = fn(x) => x * 3
var negate: fn(bool): bool = fn(b) => !b

// Mixed explicit and inferred parameters
var mixed: fn(int, int): int = fn(a: int, b) => a - b
```

### Return Type Inference

Return types are inferred from the expression or explicit `return` statements:

```sindarin
var is_even: fn(int): bool = fn(x) => x % 2 == 0
var greet: fn(str): str = fn(name) => $"Hello, {name}!"
```

## Closures

Closures are lambdas that capture variables from their enclosing scope.

### Capturing Variables

```sindarin
var multiplier: int = 5
var times: fn(int): int = fn(x: int): int => x * multiplier
print($"times(3) = {times(3)}\n")  // 15

// Capture multiple variables
var x: int = 10
var y: int = 20
var sum_with_xy: fn(int): int = fn(z: int): int => x + y + z
print($"sum_with_xy(5) = {sum_with_xy(5)}\n")  // 35
```

### Capturing Different Types

Closures can capture variables of any type:

```sindarin
var prefix: str = "Value: "
var format_int: fn(int): str = fn(n: int): str => $"{prefix}{n}"
print($"{format_int(42)}\n")  // Value: 42

var verbose: bool = true
var maybe_print: fn(str): void = fn(msg: str): void =>
    if verbose =>
        print(msg)
```

### Mutating Captured Variables

Closures can read and modify captured variables:

```sindarin
var count: int = 0
var increment: fn(): int = fn(): int =>
    count = count + 1
    return count

print($"Call 1: {increment()}\n")  // 1
print($"Call 2: {increment()}\n")  // 2
print($"Call 3: {increment()}\n")  // 3
print($"Final count: {count}\n")   // 3
```

### Multiple Closures Sharing State

Multiple closures can share the same captured variable:

```sindarin
var shared_val: int = 0

var inc: fn(): int = fn(): int =>
    shared_val = shared_val + 1
    return shared_val

var dec: fn(): int = fn(): int =>
    shared_val = shared_val - 1
    return shared_val

var get: fn(): int = fn(): int => shared_val

print($"Inc: {inc()}\n")      // 1
print($"Inc: {inc()}\n")      // 2
print($"Dec: {dec()}\n")      // 1
print($"Get: {get()}\n")      // 1
```

### Closures in Loops

When creating closures in loops, each iteration captures the current value:

```sindarin
for var i: int = 0; i < 3; i++ =>
    var capture_i: fn(): int = fn(): int => i
    print($"Captured: {capture_i()}\n")
```

## Higher-Order Functions

Functions can accept lambdas as parameters and return them.

### Lambdas as Parameters

```sindarin
fn apply(f: fn(int): int, x: int): int =>
    return f(x)

fn apply_twice(f: fn(int): int, x: int): int =>
    return f(f(x))

var double_it: fn(int): int = fn(x: int): int => x * 2
var square: fn(int): int = fn(x: int): int => x * x

print($"apply(double, 5) = {apply(double_it, 5)}\n")        // 10
print($"apply_twice(double, 3) = {apply_twice(double_it, 3)}\n")  // 12
```

### Inline Lambdas

Lambdas can be passed inline without assigning to a variable:

```sindarin
var result: int = apply(fn(x: int): int => x + 10, 5)
print($"result = {result}\n")  // 15
```

### Returning Lambdas

Functions can return lambdas, enabling factory patterns:

```sindarin
fn make_adder(n: int): fn(int): int =>
    return fn(x: int): int => x + n

fn make_multiplier(n: int): fn(int): int =>
    return fn(x: int): int => x * n

var add5: fn(int): int = make_adder(5)
var triple: fn(int): int = make_multiplier(3)

print($"add5(10) = {add5(10)}\n")      // 15
print($"triple(7) = {triple(7)}\n")    // 21
```

### Function Composition

```sindarin
fn compose(f: fn(int): int, g: fn(int): int): fn(int): int =>
    return fn(x: int): int => f(g(x))

var double_it: fn(int): int = fn(x: int): int => x * 2
var square: fn(int): int = fn(x: int): int => x * x

var double_then_square: fn(int): int = compose(square, double_it)
print($"double_then_square(3) = {double_then_square(3)}\n")  // 36
```

### Currying

```sindarin
fn curried_add(a: int): fn(int): int =>
    return fn(b: int): int => a + b

var add3: fn(int): int = curried_add(3)
print($"add3(5) = {add3(5)}\n")  // 8
```

## The `shared` Modifier

The `shared` modifier affects memory allocation. A `shared` lambda uses the caller's arena instead of creating its own:

```sindarin
// Shared lambda - uses caller's arena
var add_shared: fn(int, int): int = fn(a: int, b: int) shared: int => a + b

// Shared with type inference
var triple_shared: fn(int): int = fn(x) shared => x * 3

// Shared closure with capture
var factor: int = 10
var multiply_shared: fn(int): int = fn(x: int) shared: int => x * factor
```

See [Memory](memory.md) for details on arena memory management.

## Nested Lambdas

Lambdas can be nested, with inner lambdas capturing from outer scopes:

```sindarin
var outer: int = 100
var middle: fn(int): fn(int): int = fn(a: int): fn(int): int =>
    var inner: fn(int): int = fn(b: int): int => outer + a + b
    return inner

var add_100_50: fn(int): int = middle(50)
print($"add_100_50(25) = {add_100_50(25)}\n")  // 175
```

## Arrays of Lambdas

Arrays can hold function types. Use parentheses to disambiguate:

```sindarin
// Array of lambdas (parentheses required)
var operations: (fn(int, int): int)[] = {}

var add: fn(int, int): int = fn(a: int, b: int): int => a + b
var mul: fn(int, int): int = fn(a: int, b: int): int => a * b

operations.push(add)
operations.push(mul)

print($"operations[0](10, 5) = {operations[0](10, 5)}\n")  // 15
print($"operations[1](10, 5) = {operations[1](10, 5)}\n")  // 50
```

See [Arrays](arrays.md) for more on arrays of function types.

## Function Type Syntax

Function types use the `fn` keyword:

| Syntax | Description |
|--------|-------------|
| `fn(): void` | No parameters, returns nothing |
| `fn(): int` | No parameters, returns int |
| `fn(int): int` | One int parameter, returns int |
| `fn(int, int): int` | Two int parameters, returns int |
| `fn(str): str` | String parameter, returns string |
| `fn(int): fn(int): int` | Returns a function |
| `(fn(int): int)[]` | Array of functions |

## Complete Example

```sindarin
// A simple callback system
fn main(): void =>
    // Create operations map using closures
    var operations: (fn(int, int): int)[] = {}

    operations.push(fn(a: int, b: int): int => a + b)  // add
    operations.push(fn(a: int, b: int): int => a - b)  // subtract
    operations.push(fn(a: int, b: int): int => a * b)  // multiply

    var names: str[] = {"add", "subtract", "multiply"}
    var a: int = 10
    var b: int = 3

    for var i: int = 0; i < operations.length; i++ =>
        var result: int = operations[i](a, b)
        print($"{names[i]}({a}, {b}) = {result}\n")
```

Output:
```
add(10, 3) = 13
subtract(10, 3) = 7
multiply(10, 3) = 30
```
