# Match Expressions

Sindarin supports pattern matching with the `match` expression. Match compares a subject expression against a series of arms using equality, executing the body of the first matching arm.

## Basic Syntax

```sindarin
match subject =>
    pattern1 => body1
    pattern2 => body2
    else => default_body
```

The subject is evaluated once, then compared top-to-bottom against each arm's pattern. The first matching arm's body is executed.

## Statement Context

When used as a statement, match arms can have any body type:

```sindarin
fn describe(n: int): void =>
    match n =>
        1 => print("one\n")
        2 => print("two\n")
        3 => print("three\n")
        else => print("other\n")

fn main(): void =>
    describe(1)   // one
    describe(3)   // three
    describe(99)  // other
```

## Expression Context

Match can be used as an expression to produce a value. In this context, all arms must have the same result type, and an `else` arm is required:

```sindarin
fn status_message(code: int): str =>
    var msg: str = match code =>
        200 => "OK"
        404 => "Not Found"
        500 => "Internal Server Error"
        else => "Unknown"
    return msg

fn main(): void =>
    print($"{status_message(200)}\n")   // OK
    print($"{status_message(404)}\n")   // Not Found
    print($"{status_message(999)}\n")   // Unknown
```

The last expression in each arm's body is the result value.

## Multi-Value Arms

Arms can match against multiple values separated by commas:

```sindarin
fn category(code: int): str =>
    var result: str = match code =>
        200, 201, 202 => "Success"
        301, 302 => "Redirect"
        400, 401, 403 => "Client Error"
        500, 502, 503 => "Server Error"
        else => "Other"
    return result
```

This is equivalent to matching any of the listed values (logical OR).

## Multi-Statement Arms

Arm bodies can contain multiple statements. Use an indented block:

```sindarin
fn process(code: int): str =>
    var result: str = match code =>
        200 =>
            print("Processing success...\n")
            "OK"
        404 =>
            print("Processing not found...\n")
            "Not Found"
        else =>
            print("Processing unknown...\n")
            "Unknown"
    return result
```

In expression context, the last expression in the block is the arm's result value.

## The `else` Arm

The `else` arm matches when no other arm does:

```sindarin
fn day_type(day: int): void =>
    match day =>
        1, 7 => print("Weekend\n")
        else => print("Weekday\n")
```

- In **expression context**, the `else` arm is required (ensures a value is always produced).
- In **statement context**, the `else` arm is optional. If omitted and no arm matches, execution continues past the match.

## String Matching

Match works with string subjects using string equality:

```sindarin
fn greet(lang: str): void =>
    match lang =>
        "en" => print("Hello\n")
        "es" => print("Hola\n")
        "fr" => print("Bonjour\n")
        else => print("Hi\n")
```

## Supported Types

Match subjects can be any type that supports equality comparison:

- `int`, `long`, `byte` — numeric equality
- `str` — string equality (uses runtime string comparison)
- `char` — character equality
- `bool` — boolean equality

## Design Notes

- **No fallthrough** — each arm is independent; only the first matching arm executes.
- **Evaluation order** — the subject is evaluated once, arms are compared top-to-bottom.
- **Single expression patterns** — arm patterns are expressions (not destructuring patterns).
