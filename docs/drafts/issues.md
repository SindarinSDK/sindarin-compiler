# Compiler & Language Issues

Issues discovered while implementing `sdk/core/version.sn` (a pure Sindarin SDK type).

## Missing Language Features

### 1. No enum type

Operator constants must be modeled as functions returning integers. A proper enum would provide type safety and self-documentation.

```
# Current: magic integers, no type safety
fn OP_EQ(): int => 0
fn OP_GT(): int => 1
fn OP_GTE(): int => 2

var op: int = OP_CARET()  # just an int, any value accepted

# Desired:
enum Operator => EQ, GT, GTE, LT, LTE, CARET, TILDE
var op: Operator = Operator.CARET
```

### 2. No match/switch statement

Multi-way branching requires chains of `if`/`else if`. Error-prone and verbose, especially combined with the lack of enums.

```
# Current: repetitive if-chains
if self.op == OP_EQ() =>
    return cmp == 0
if self.op == OP_GT() =>
    return cmp == 1
if self.op == OP_GTE() =>
    return cmp >= 0
...

# Desired:
match self.op =>
    Operator.EQ => return cmp == 0
    Operator.GT => return cmp == 1
    Operator.GTE => return cmp >= 0
    ...
```

### 3. No error/result type

Functions like `Version.parse` cannot signal failure. A malformed input silently returns a zero-value struct.

```
# Current: silent failure
var v: Version = Version.parse("not-a-version")  # returns 0.0.0, no error

# Desired: explicit error handling
var result: Result<Version, str> = Version.parse("not-a-version")
if result.isErr() =>
    print($"Parse error: {result.error()}\n")
```

### 4. No operator overloading

Comparison must use named methods rather than natural operators.

```
# Current:
if v1.lt(v2) => ...
if v1.compare(v2) == 0 => ...

# Desired:
if v1 < v2 => ...
if v1 == v2 => ...
```

### 5. Missing string utilities

Common operations require manual implementation. The following had to be written from scratch for the version module:

- `_strToInt(s: str): int` — parse numeric string to integer
- `_isNumeric(s: str): bool` — check if string is all digits
- `_strcmp(a: str, b: str): int` — lexicographic comparison returning -1/0/1

Standard library additions that would help:
- `str.toInt(): int` or `int.parse(s: str): int`
- `str.isDigit(): bool` (per-character) or `str.isNumeric(): bool`
- `str.compareTo(other: str): int`
