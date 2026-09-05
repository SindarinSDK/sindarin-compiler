# Rust resolved calls

This document records the implemented contract for resolved calls in the Rust
target. The shared checked model remains authoritative: Rust consumes the
selected struct, method name, static/instance form, argument annotations, and
source-order metadata. It does not repeat overload lookup or lower a resolved
call back to a Rust operator.

## Supported envelope

`method_call` supports non-native, non-packed value structs whose receiver,
arguments, and result already have Rust representations. Static and instance
forms validate the resolved declaration, exact argument and result types,
arity, and the model's borrow/copy annotations before recursively validating
children.

For the source language implemented by the current checker, resolved operator
calls are the comparison family:

- explicitly declared `==`, `!=`, and `<` methods;
- derived `!=` through a resolved `==` method;
- derived `>`, `>=`, and `<=` through a resolved `<` method.

The generated Rust calls the resolved method directly (`op_eq`, `op_ne`, or
`op_lt`). Derived negation remains a normal model `unary` node. The current
checker has no representation-supported resolved unary-operator declarations
and no arithmetic operator-overload declarations, so there is no additional
unary or arithmetic resolved-call form for this target to render.

Owned string and array arguments to ordinary direct, static, and instance
calls are cloned when a stable source place must remain independently owned.
Composite value-struct method arguments use the model's `is_ref_arg` or
`is_borrow_tmp` decision. A borrow temporary is held for the whole call and is
rendered as a mutable reference, matching the generated method signature.
Resolved by-value string, array, and value-struct arguments similarly clone a
stable place instead of moving it.

## Evaluation and lifetime contract

Ordinary resolved instance calls evaluate the receiver once and then arguments
once in source order. Swapped comparisons are different: `a > b` is already
resolved as `b.op_lt(a)`, while the language still requires `a` before `b`.
The model therefore records `source_arg_before_object`; both C and Rust hold
the source-left argument before evaluating the resolved receiver. A second
model bit distinguishes a stable receiver place from an owned receiver
temporary. This preserves mutation targeting and gives owned temporaries the
correct cleanup/drop scope. Generated Rust local names are allocated against
all strings in the model to avoid capture by source identifiers.

The same ordering applies to target validation diagnostics: swapped-call
children are validated argument-first, while normal instance calls remain
receiver-first. Static calls validate arguments in order.

Resolved results are ordinary Rust expressions and retain ownership correctly
in expression statements, initializers, returns, call arguments, match-arm
prefix statements, and match tails. Stable member and index receiver places
are admitted; unsupported nested children keep their earlier, more specific
target diagnostic.

## Borrow-inferred calls

The model currently creates `borrow_inferred_call` only around native calls
that return a reference struct and may alias a same-type reference-struct
argument. Native calls and reference structs are both outside this slice's
representation envelope. The validator recognizes the complete wrapper shape
and emits the precise ownership diagnostic rather than silently accepting the
inner call. The Rust partial is intentionally a pass-through for the future
point at which validation can prove and annotate the retain/alias decision.

There is therefore no non-native, non-pointer, value-struct positive
`borrow_inferred_call` fixture in the current shared checker/model. Adding a
fake positive model would test a state the compiler cannot produce and could
mask the missing native reference-ownership implementation.

## Explicit follow-ups

Native and pointer receivers, reference structs, packed structs, and
closure-typed arguments/results remain separate representation work. These are
targeted diagnostics, not fallback lowering paths. Closure validation,
lowering, rendering, and support remain owned by the closure feature branch.

## Regression evidence

`resolved_calls` is shared by the model, C, Rust-generation, and runtime
integration suites. It covers every currently emitted comparison form,
resolved direct-call snapshots, static and instance metadata, stable member
and index receivers, expression statements, initializers, returns, arguments,
match prefixes and tails, source-order counters, generated-name collision,
heap-bearing value structs, borrow temporaries, and independent ownership of
returned strings/arrays and copied source values.

The final feature gate records generation snapshots, C/Rust output comparison,
Rust execution at `-O0`, `-O1`, and `-O2`, the focused Rust generator suite,
and the repository's sequential default-20 build/test commands.
