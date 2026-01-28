# Urgent Fixes

Issues discovered while converting Python scripts to Sindarin.

---

## 1. UUID Module Causes Code Generation Hang

**Severity:** Critical

**Description:** Importing and using `sdk/core/uuid` causes the compiler to hang indefinitely during code generation.

**Reproduction:**
```sindarin
import "sdk/core/uuid"

fn main(): int =>
    var id: UUID = UUID.create()
    print($"ID: {id}\n")
    return 0
```

**Expected:** Compiles successfully.

**Actual:** Compiler hangs at "Code generation..." and never completes.

**Workaround:** Use `Time.now().millis()` for unique identifiers instead.

---

## 2. Struct Arrays Do Not Support push()

**Severity:** High

**Description:** Calling `push()` on an array of structs causes a code generation error: "Unsupported array element type for push".

**Reproduction:**
```sindarin
struct Point =>
    x: int
    y: int

fn main(): int =>
    var points: Point[] = {}
    var p: Point = Point { x: 1, y: 2 }
    points.push(p)  // ERROR
    print($"Point: {points[0].x}\n")
    return 0
```

**Expected:** Struct can be pushed to array like primitives.

**Actual:** Code generation fails with "Unsupported array element type for push".

**Workaround:** Use fixed-size arrays or avoid dynamic struct collections.

---

## 3. Missing String to Integer Conversion

**Severity:** Medium

**Description:** There is no built-in method to parse a string as an integer (e.g., `str.toInt()` or `parseInt(str)`).

**Reproduction:**
```sindarin
fn main(): int =>
    var s: str = "42"
    var n: int = s.toInt()  // ERROR: no such method
    return n
```

**Expected:** Built-in method to parse integers from strings.

**Actual:** No such method exists.

**Workaround:** Implement manual parsing:
```sindarin
fn parseInt(s: str): int =>
    var result: int = 0
    var negative: bool = false
    var i: int = 0
    if s.length > 0 && s.charAt(0) == '-' =>
        negative = true
        i = 1
    while i < s.length =>
        var c: char = s.charAt(i)
        if c >= '0' && c <= '9' =>
            result = result * 10 + (c - '0') as int
        i = i + 1
    if negative =>
        return -result
    return result
```

---

## 4. No Hex Escape Sequences in Strings

**Severity:** Low

**Description:** Hex escape sequences like `\x1b` are not supported in string literals. Only `\n`, `\t`, `\r`, `\\`, `\"`, `\'`, `\0` are valid.

**Reproduction:**
```sindarin
var red: str = "\x1b[0;31m"  // ERROR: Invalid escape sequence
```

**Expected:** Hex escapes work like in C.

**Actual:** Parse error on `\x`.

**Workaround:** Compute escape character at runtime:
```sindarin
var esc: char = 27 as char
var red: str = $"{esc}[0;31m"
```

---

## Summary

| Issue | Severity | Blocking? |
|-------|----------|-----------|
| UUID code gen hang | Critical | Yes - cannot use UUID module |
| Struct array push | High | Yes - limits dynamic collections |
| No parseInt | Medium | No - can implement manually |
| No hex escapes | Low | No - runtime workaround exists |
