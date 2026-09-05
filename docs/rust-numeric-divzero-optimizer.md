# Rust numeric optimizer and unsigned-literal parity

## Scope

This branch is based on `feature/rust-tagged-byte-semantics` commit
`52f4e152681d6211b107ef791d376dca1a3c812f`. It resolves the two inherited
default-optimizer division-by-zero fixture mismatches and the next measured
fixed-width numeric gap without changing C generation, the shared model, the
frontend, compiler configuration, or any v0.0.83 source.

The `uint_checked_div_zero.sn` and `uint32_checked_div_zero.sn` source files and
their `default` optimizer sidecars are unchanged. At that optimizer selection,
the unused division is removed. Their Rust snapshots now describe the actual
optimized program and their expected behavior is successful execution with no
output.

Rust-private lowering also distinguishes unary `-` and `~` applied directly to
tagged unsigned literals when the expression is observed by `print` or
`println`. The tag emits these literal operands as signed `long long` values,
so direct observation preserves the promoted signed result. Storage and other
typed contexts still narrow to `uint32` or `uint`, preserving wrapping variable
semantics. No signed checked-arithmetic route is changed.

## Tagged division boundary

The pinned oracle is v0.0.83 commit
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`, using the verified compiler and
worktree from `/tmp/sindarin-tagged-control-reference.md`. The required fresh
smoke passed at `/tmp/sindarin-s2-tag-smoke-divzero-AJiT7U` before the division
harness.

The raw 18-case tag matrix is
`/tmp/sindarin-s2-tag-divzero-XvSFWl/results.tsv`: both unchanged fixtures,
default/checked/unchecked, and O0/O1/O2. Default and checked O0 are tag compile
failures because generated C calls the absent `sn_div_uint` or
`sn_div_uint32`; no executable was produced, so these are an oracle acceptance
boundary and are not runtime parity evidence. Default and checked O1/O2 compile
and run successfully with empty stdout and stderr because the optimizer removes
the unused division. Unchecked executions are not parity evidence because C
division by zero is undefined.

Current Rust was compared with the valid tag cases in
`/tmp/sindarin-s2-rust-divzero-nkav4x/results.tsv`. Both fixtures pass in
default and checked O1/O2, and in the CLI-default selection used by their
sidecars: 10 Rust executions, all successful, with every fresh stdout/stderr
comparison equal to the corresponding tag O1/O2 capture.

## Unsigned literal evidence

The existing backlog source `tagged_wrapping_literal_unary.sn` is committed
unchanged from the preserved raw probe (SHA-256
`33be63935f3fbb936b1400ce700acb8a92c40d3c0f30a3cbfd5f9de4e6a8e03b`). A
fresh required smoke passed at `/tmp/sindarin-s2-tag-smoke-unary-oMhyAD`.
Tagged checked O0/O1/O2 all compile and run with identical raw output at
`/tmp/sindarin-s2-tag-unary-E9Nk9Z`.

The additional `tagged_unsigned_literal_contexts.sn` regression separates
direct observation from narrowing storage for both unary operators and both
unsigned widths. Its source SHA-256 is
`aea8ea5cc20b8a934a8036ab22fcfcfa430baadca2a894572285f7729dc400bb`.
The required smoke passed at
`/tmp/sindarin-s2-tag-smoke-unary-context-uwCIRN`; tagged checked O0/O1/O2
results are `/tmp/sindarin-s2-tag-unary-context-CaXX4U`.

Rust checked O0/O1/O2 results for both sources are
`/tmp/sindarin-s2-rust-unary-RPE0AH/results.tsv`: six successful compile/run
pairs, with raw stdout and stderr equal to the fresh tag captures. No output,
diagnostic, path, ANSI, or newline normalization was applied.

Remaining literal-expression work includes other observation contexts not
covered by these measured failures. C-rejected and undefined probes remain
outside parity claims.
