# Namespaces in Sindarin

Sindarin extends its import system with optional namespace support. The existing import behavior remains unchanged, but a new `as` clause allows imports to be scoped under a namespace prefix.

---

## Current Import Behavior (Unchanged)

The existing import syntax continues to work exactly as before:

```sindarin
import "math_utils"

fn main(): void =>
    var result: int = add(5, 3)      // Direct access to imported functions
    var product: int = multiply(2, 4)
```

All exported symbols from the imported module are available directly in the importing file's scope.

---

## Namespaced Imports

### Basic Syntax

Use the `as` keyword to import a module under a namespace:

```sindarin
import "math_utils" as math

fn main(): void =>
    var result: int = math.add(5, 3)       // Must use namespace prefix
    var product: int = math.multiply(2, 4)
```

When a namespace is specified, **all** symbols from that module must be accessed through the namespace prefix.

### Namespace Identifier Rules

Namespace identifiers follow the same rules as variable names:
- Must start with a letter or underscore
- Can contain letters, digits, and underscores
- Cannot be a reserved keyword
- Case-sensitive

```sindarin
// Valid namespace identifiers
import "utilities" as util
import "http_client" as http
import "MyLibrary" as myLib
import "v2_api" as api2

// Invalid - these would be compile errors
import "math" as 123abc     // Cannot start with digit
import "math" as for        // Cannot use reserved keyword
import "math" as my-lib     // Cannot contain hyphens
```

---

## Mixing Import Styles

Both import styles can be used in the same file:

```sindarin
import "string_utils"           // Direct access
import "math_utils" as math     // Namespaced access

fn main(): void =>
    var greeting: str = greet("World")    // From string_utils (direct)
    var sum: int = math.add(10, 20)       // From math_utils (namespaced)
```

### Same Module, Different Styles

A module can be imported multiple times with different styles, though this is discouraged:

```sindarin
import "math_utils"             // Direct access
import "math_utils" as math     // Also namespaced

fn main(): void =>
    var a: int = add(1, 2)       // Works (direct)
    var b: int = math.add(1, 2) // Also works (namespaced)
```

---

## Name Collision Resolution

### Without Namespaces (Current Behavior)

If two modules export the same symbol and both are imported directly, it's a compile-time error:

```sindarin
import "math_utils"      // Exports: add, subtract
import "string_builder"  // Also exports: add

fn main(): void =>
    add(1, 2)  // ERROR: Ambiguous reference to 'add'
```

### With Namespaces

Namespaces resolve collisions by qualifying which module's symbol to use:

```sindarin
import "math_utils" as math
import "string_builder" as sb

fn main(): void =>
    var sum: int = math.add(1, 2)           // math_utils.add
    var result: str = sb.add("hello", "!")  // string_builder.add
```

### Hybrid Resolution

You can import one module directly and namespace the other:

```sindarin
import "math_utils"              // Primary module (direct)
import "string_builder" as sb    // Secondary module (namespaced)

fn main(): void =>
    var sum: int = add(1, 2)              // math_utils.add (direct)
    var result: str = sb.add("a", "b")    // string_builder.add (namespaced)
```

---

## Accessing Module Contents

### Functions

```sindarin
import "utils" as u

fn main(): void =>
    u.helper()
    var result: int = u.compute(42)
```

---

## Nested Paths

The namespace only affects how symbols are accessed, not how the module path is specified:

```sindarin
// The path remains a string literal
import "lib/utils/math" as math
import "external/vendor/http" as http

fn main(): void =>
    var x: int = math.add(1, 2)
    http.get("https://example.com")
```

---

## Best Practices

### When to Use Namespaces

1. **Preventing collisions**: When importing modules that may have conflicting names
2. **Clarity**: When it's helpful to see where a function comes from
3. **Large codebases**: When importing many modules
4. **Third-party code**: When using external libraries with generic names

### When Direct Import is Fine

1. **Single import**: When only one module is imported
2. **Well-known utilities**: For common utility functions with unique names
3. **Small files**: When context is obvious

### Naming Conventions

```sindarin
// Good: Short, descriptive abbreviations
import "mathematics" as math
import "string_utilities" as str
import "file_system" as fs
import "network/http" as http

// Avoid: Too short or cryptic
import "mathematics" as m
import "utilities" as u

// Avoid: Redundant suffixes
import "math_utils" as mathUtils
import "string_lib" as stringLib
```

---

## Grammar

The import statement grammar is extended:

```
import_stmt  ::= "import" STRING_LITERAL ( "as" IDENTIFIER )?
```

Where:
- `STRING_LITERAL` is the module path in double quotes
- `IDENTIFIER` is an optional namespace name

---

## Implementation Notes

### Symbol Table

When a namespaced import is processed:
1. A namespace entry is created in the symbol table
2. All symbols from the imported module are registered under that namespace
3. Symbol lookup checks for namespace prefix and resolves accordingly

### Code Generation

For namespaced calls like `math.add(1, 2)`:
1. The parser recognizes `math.add` as a namespaced function call
2. The symbol table resolves `math` to the namespace, then `add` within it
3. Code generation emits the actual function name (namespaces are compile-time only)

### No Runtime Overhead

Namespaces are purely a compile-time feature. The generated C code uses the actual function names directly with no indirection.

---

## Examples

### Complete Example

```sindarin
// math.sn
fn add(a: int, b: int): int => a + b
fn multiply(a: int, b: int): int => a * b

// strings.sn
fn add(a: str, b: str): str => $"{a}{b}"
fn repeat(s: str, n: int): str =>
    var result: str = ""
    for var i: int = 0; i < n; i++ =>
        result = $"{result}{s}"
    return result

// main.sn
import "math" as math
import "strings" as str

fn main(): void =>
    // No ambiguity - each 'add' is clearly qualified
    var sum: int = math.add(10, 20)
    var combined: str = str.add("Hello, ", "World!")

    print($"Sum: {sum}\n")           // Sum: 30
    print($"Combined: {combined}\n") // Combined: Hello, World!

    var product: int = math.multiply(6, 7)
    var stars: str = str.repeat("*", 5)

    print($"Product: {product}\n")   // Product: 42
    print($"Stars: {stars}\n")       // Stars: *****
```

