---
title: "Namespaces"
description: "Module namespaces and code organization"
permalink: /language/namespaces/
---

Sindarin's import system supports an optional `as` clause that scopes all exported symbols from a module under a namespace prefix.

## Basic Usage

Without `as`, imported symbols are available directly in the importing file's scope:

```sindarin
import "lib/math_ops"

fn main(): void =>
    var result: int = add(5, 3)
    var product: int = multiply(2, 4)
```

With `as`, all symbols must be accessed through the namespace prefix:

```sindarin
import "lib/math_ops" as math

fn main(): void =>
    var result: int = math.add(5, 3)
    var product: int = math.multiply(2, 4)
```

## Multiple Namespaces

```sindarin
import "lib/math_basic" as basic
import "lib/math_advanced" as adv

fn main(): void =>
    print($"basic.sum(2, 3) = {basic.sum(2, 3)}\n")
    print($"adv.power(2, 4) = {adv.power(2, 4)}\n")

    var combined: int = basic.sum(adv.square(3), adv.power(2, 3))
```

## Mixing Import Styles

Direct and namespaced imports can coexist in the same file:

```sindarin
import "lib/math_ops"             // Direct access
import "lib/string_ops" as strings // Namespaced access

fn main(): void =>
    var total: int = add(multiply(2, 3), subtract(10, 5))
    print($"strings.greet(\"Alice\") = {strings.greet("Alice")}\n")
```

## Name Collision Resolution

When two modules export the same symbol, importing both directly is a compile-time error:

```sindarin
import "lib/calc_v1"
import "lib/calc_v2"

fn main(): void =>
    compute(5)  // ERROR: ambiguous reference to 'compute'
```

Use namespaces to resolve the collision:

```sindarin
import "math_utils" as math
import "string_builder" as sb

fn main(): void =>
    var sum: int = math.add(1, 2)
    var result: str = sb.add("hello", "!")
```

Alternatively, import one module directly and namespace the other:

```sindarin
import "lib/math_basic"
import "lib/math_advanced" as adv

fn main(): void =>
    var x: int = sum(adv.power(2, 4), prod(2, 3))
```

## Namespace Identifier Rules

Namespace identifiers follow the same rules as variable names:

- Must start with a letter or underscore
- Can contain letters, digits, and underscores
- Cannot be a reserved keyword
- Case-sensitive

```sindarin
import "utilities" as util    // valid
import "http_client" as http  // valid
import "math" as 123abc       // ERROR: starts with digit
import "math" as for          // ERROR: reserved keyword
import "math" as my-lib       // ERROR: hyphens not allowed
```

## Grammar

```
import_stmt  ::= "import" STRING_LITERAL ( "as" IDENTIFIER )?
```

Namespaces are compile-time only — they have no runtime overhead. The generated C code uses actual function names directly.
