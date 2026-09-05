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

Owned string arguments to ordinary direct, static, and instance calls are
cloned when a stable source place must remain independently owned. Composite
value-struct method arguments use the model's `is_ref_arg` or `is_borrow_tmp`
decision. A borrow temporary is held for the whole call and is rendered as a
mutable reference, matching the generated method signature. Resolved by-value
string and value-struct arguments similarly clone a stable place instead of
moving it.

Default array parameters use shared pointer identity in C, while the current
Rust representation is an owned `Vec`. Cloning hides mutations and moving a
stable source place destroys its continued ownership. Calls from non-owning
array expressions are therefore rejected precisely until arrays have a shared Rust
representation. Admission requires positive proof of fresh ownership rather
than merely the absence of a stable-place shape: an array-valued member below
an effectful index still aliases its source in C. Fresh array constructors and
owned call results remain supported because no source alias survives the call.

## Evaluation and lifetime contract

Ordinary resolved instance calls evaluate the receiver once and then arguments
once in source order. Swapped comparisons are different: `a > b` is already
resolved as `b.op_lt(a)`, while the language still requires `a` before `b`.
The model therefore records `source_arg_before_object`; both C and Rust hold
the source-left argument before evaluating the resolved receiver. A second
model bit distinguishes a stable receiver place from an owned receiver
temporary. Nested receiver producers are stabilized inside that ordered call
scope, after the source-left argument, so they run once and retain their normal
cleanup/drop lifetime. Generated Rust local names are allocated against all
strings in the model to avoid capture by source identifiers.

The same ordering applies to target validation diagnostics: swapped-call
children are validated argument-first, while normal instance calls remain
receiver-first. Static calls validate arguments in order.

Ordinary function values remain supported inside resolved receiver-producing
expressions; receiver traversal and alias analysis ignore function-type
metadata while still validating and evaluating the nested call normally.

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
Ordinary function values nested inside otherwise supported resolved
expressions do not imply support for closure-typed resolved method parameters.

Rust also rejects a resolved method receiver and mutable borrowed operand when
their stable place paths are identical or are elements of the same array.
Sindarin/C permits the operand to alias and observe mutation, but Rust cannot
form the required overlapping references safely. Full parity requires a
shared/interior-mutability value-struct representation; unsafe aliasing or a
defensive clone would change the language contract.

When a swapped comparison's resolved receiver reads a mutable source operand,
Rust delays forming the mutable borrow until receiver evaluation completes for
side-effect-free variable/member places. This preserves the C-visible read
before the operator mutates the operand without overlapping Rust references.
Indexed operands remain rejected in this shape: delaying their selection can
change source order, bounds checks, or the selected element if receiver
evaluation mutates the array owner. This remains an explicit parity boundary
until the representation can preserve C aliasing without unsafe references.

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
