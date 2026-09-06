# Rust default-array calls

Sindarin default array parameters share the caller's array handle. Element and
method mutations through a parameter are visible through every alias, while a
parameter itself remains a non-owning binding. The Rust backend represents a
stable default-array argument as a mutable borrow of the caller's `Vec`.

Distinct stable arrays can be borrowed independently. When a direct call passes
the same proven stable variable or member place to more than one default-array
parameter, Rust-private lowering emits a collision-free specialized function.
Duplicate formals in that private function are rewritten to the first formal,
and the call passes one mutable borrow for that place. This preserves sequential
mutation visibility without cloning the array and without constructing unsafe
overlapping mutable references. The source function remains available with its
ordinary signature for non-aliasing calls.

Specialization follows the Rust-private binding identities assigned during
validation, so a nested declaration that shadows a formal keeps its independent
binding. Newly specialized bodies are processed recursively. A specialization
is memoized by its source function and formal-alias partition before its body is
visited, which both propagates aliases through forwarders and terminates for
recursive or mutually recursive calls.

Direct array fields of a stable instance receiver are the corresponding
bounded receiver-alias cases. Rust-private lowering clones the method under a
collision-free name, replaces each aliased formal by its `self.field`, and
removes those arguments at the call site. The resulting call takes only the
receiver borrow, so method mutations and formal reads retain their source array
identities without cloning or overlapping mutable references. Binding IDs keep
same-spelled nested locals independent. The complete formal-to-field pattern is
memoized before the clone body is visited, allowing aliases to propagate through
method forwarders and recursive or mutually recursive forwarding calls.

Stable array places have no source-visible evaluation of their own. Other
arguments are evaluated once in their original relative order before any array
borrow when a later argument could read an earlier array. This includes calls
with multiple distinct arrays and calls whose duplicate array formals have been
coalesced.

The `receiver_array_alias_multiple` fixture covers distinct receiver fields,
two parameters bound to the same field, direct calls, and a method forwarder.
The `receiver_array_alias_forward_recursive` fixture covers binding-safe lexical
shadowing and mutually recursive method forwarding. Both compare unchanged
tagged C and Rust execution at `-O0`, `-O1`, and `-O2`.

The focused `default_array_multiple_distinct` and
`default_array_multiple_same` fixtures cover distinct arrays, duplicate local
and member places, mixed duplicate and distinct arguments, mutation visibility,
a side-effectful scalar between array arguments, a later same-array length read,
and private-name collision. The forwarder fixtures cover transitive local and
member aliases, and the shadow fixture proves that specialization does not cross
a nested binding with the same source spelling. Their tagged C and generated
Rust executions are compared at `-O0`, `-O1`, and `-O2`.

This bounded representation does not infer runtime aliasing between different
place expressions. Produced/indexed array places, sibling references derived
from the same aggregate beyond direct receiver fields, and broader
receiver/argument overlap retain their separately owned representation work.
Mutable array parameter rebinding also requires a handle representation rather
than formal coalescing. No C model, template, runtime, source fixture, or
language rule is changed by this Rust-private lowering.
