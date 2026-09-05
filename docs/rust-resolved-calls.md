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

Resolved instance calls evaluate the already-selected receiver once and then
the arguments once in model order. This includes swapped comparisons: `a > b`
is resolved as `b.op_lt(a)`, and tagged C evaluates `b` before `a`. Rust method
call evaluation has the same order. Rust-private lowering prefixes stabilize
owning array producers and computed indices below receivers and borrowed
arguments so bounds-checked indexing never duplicates either expression. When
an argument needs stabilization, the receiver and every argument are bound
once in source order; a borrowed array producer remains live through the
method invocation. These prefixes preserve tagged C order and normal drop
lifetime.
Generated Rust local names are allocated against all strings in the Rust model
to avoid capture by source identifiers. The C model, C target, and shared
chain-flattening pass are unchanged by this feature.

Target validation diagnostics follow the same order: instance receivers are
validated before arguments, and static calls validate arguments in order.

Ordinary function values remain supported inside resolved receiver-producing
expressions; receiver traversal and alias analysis ignore function-type
metadata while still validating and evaluating the nested call normally.

Function values are also supported as value parameters and results of
non-native static and instance methods. The Rust target uses the closure
backend's reference-counted callable handle, so passing a stable callable
preserves handle identity, returning a capturing callable transfers an owned
handle, and a later reassignment of the source binding does not change an
already returned composition. A callable `as ref` parameter borrows the
caller's handle slot, so reassignment is visible to the caller while ordinary
reads retain the selected handle independently. Callable signatures are
compared recursively when resolved-call metadata is validated; unsupported
callable component types remain outside the same closure representation
predicate. Returning an owned callable value retains the existing callable
result behavior.

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

Native and pointer receivers, reference structs, and packed structs remain
separate representation work. These are targeted diagnostics, not fallback
lowering paths. The resolved-call slice reuses the closure feature's callable
handle representation and adds only borrowed handle-slot reads and writes; it
does not change capture representation or capture support.

Rust also rejects a resolved method receiver and mutable borrowed operand when
their stable place paths are identical or are elements of the same array.
Sindarin/C permits the operand to alias and observe mutation, but Rust cannot
form the required overlapping references safely. Full parity requires a
shared/interior-mutability value-struct representation; unsafe aliasing or a
defensive clone would change the language contract.

When a swapped comparison's resolved receiver reads a mutable source operand,
receiver-first evaluation completes that read before Rust forms the argument's
mutable borrow. Variable, member, and indexed operands are therefore supported
for this non-overlapping lifetime shape without unsafe references or clones.

## Regression evidence

The Rust `resolved_calls` fixtures cover every currently emitted comparison
form, resolved direct-call snapshots, static and instance metadata, stable
member and index receivers, expression statements, initializers, returns,
arguments, match prefixes and tails, source-order counters, generated-name
collision, heap-bearing value structs, borrow temporaries, and independent
ownership of returned strings/arrays and copied source values. Heap-bearing
and `as ref` operator cases are Rust generation extras, not tagged-C parity
claims; this includes the computed-index control that pins once-only place
evaluation while borrowing its resolved argument and the call/member/nested
producer control that pins owner lifetime and receiver-before-argument order.
The small plain-value
`resolved_operator_tagged_eval_order` control is valid on both targets and
pins tagged C's receiver-before-argument swapped order. The existing C
snapshot suite is retained unchanged as a backend-freeze check.

The `resolved_callable_methods` fixture additionally covers immutable callable
parameters and results on static and instance methods, reference-counted handle
identity, captured-result ownership after source reassignment, a returned
method result crossing another function boundary, receiver/argument source
order, and execution at each optimization level. Its C execution is an
unchanged-language behavior control rather than a C snapshot change.

`resolved_callable_method_qualified_signature` promotes the previously
rejected tagged-valid callable `as ref` method parameter without changing its
source. `resolved_callable_borrowed_parameters` proves caller-visible handle
replacement with a fresh closure and once-only argument evaluation. Existing
owned-callable controls continue to cover returned captures, escaping owners,
handle identity, and instance receiver order.

The final feature gate records Rust generation snapshots and execution at
`-O0`, `-O1`, and `-O2`, the focused Rust generator suite, unchanged C
snapshots, and the repository's build/test commands.
