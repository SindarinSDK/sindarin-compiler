# Rust resolved calls

This document records the implemented contract for resolved calls in the Rust
target. The checked AST remains authoritative for the selected struct, method,
static/instance form, and source operands. The Rust-private model projection
adds target-only argument and source-order annotations. It does not repeat
overload lookup or lower a resolved call back to a Rust operator.

## Supported envelope

`method_call` supports non-native, non-packed value structs whose receiver,
arguments, and result already have Rust representations. Static and instance
forms validate the resolved declaration, exact argument and result types,
arity, and borrow/copy qualifiers before recursively validating children.
Operator-call qualifiers are projected privately from the resolved method
declaration so they cannot alter C template behavior.

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
array expressions are therefore rejected precisely until arrays have a shared
Rust representation. Admission requires positive proof of fresh ownership rather
than merely the absence of a stable-place shape: an array-valued member below
an effectful index still aliases its source in C. Fresh array constructors and
owned call results remain supported because no source alias survives the call.

## Evaluation and lifetime contract

Ordinary resolved instance calls evaluate the receiver once and then arguments
once in source order. Swapped comparisons are different: `a > b` is already
resolved as `b.op_lt(a)`, while the language still requires `a` before `b`.
The Rust-private projection therefore records `source_arg_before_object`; Rust
holds the source-left argument before evaluating the resolved receiver. The
Rust validator privately derives whether the receiver is a stable place or an
owned temporary. Nested receiver producers are stabilized inside that ordered
Rust call scope, after the source-left argument, so they run once and retain
their normal drop lifetime. Generated Rust local names are allocated against
all strings in the Rust model to avoid capture by source identifiers. The C
model, C target, and shared chain-flattening pass are unchanged by this
feature.

The same ordering applies to target validation diagnostics: swapped-call
children are validated argument-first, while normal instance calls remain
receiver-first. Static calls validate arguments in order.

Ordinary function values remain supported inside resolved receiver-producing
expressions; receiver traversal and alias analysis ignore function-type
metadata while still validating and evaluating the nested call normally.

Immutable function values are also supported as value parameters and results
of non-native static and instance methods. The Rust target uses the closure
backend's reference-counted callable handle, so passing a stable callable
preserves handle identity, returning a capturing callable transfers an owned
handle, and a later reassignment of the source binding does not change an
already returned composition. Callable signatures are compared recursively
when resolved-call metadata is validated; unsupported callable component types
remain outside the same closure representation predicate. Borrowed mutable
callable parameters remain outside this immutable slice.

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
borrowed mutable callable parameters remain separate representation work.
These are targeted diagnostics, not fallback lowering paths. The resolved-call
slice reuses the closure feature's immutable callable representation and does
not modify closure validation, lowering, rendering, or capture support.

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

The Rust `resolved_calls` fixtures cover every currently emitted comparison
form, resolved direct-call snapshots, static and instance metadata, stable
member and index receivers, expression statements, initializers, returns,
arguments, match prefixes and tails, source-order counters, generated-name
collision, heap-bearing value structs, borrow temporaries, and independent
ownership of returned strings/arrays and copied source values. The existing C
snapshot suite is retained unchanged as a backend-freeze check.

The `resolved_callable_methods` fixture additionally covers immutable callable
parameters and results on static and instance methods, reference-counted handle
identity, captured-result ownership after source reassignment, a returned
method result crossing another function boundary, receiver/argument source
order, and execution at each optimization level. Its C execution is an
unchanged-language behavior control rather than a C snapshot change.

The final feature gate records Rust generation snapshots and execution at
`-O0`, `-O1`, and `-O2`, the focused Rust generator suite, unchanged C
snapshots, and the repository's build/test commands.
