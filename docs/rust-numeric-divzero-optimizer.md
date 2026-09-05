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

Rust-private lowering also distinguishes unary `-` and `~` chains rooted at
tagged unsigned literals. The tag emits these literal operands as signed `long
long` values, so comparison, interpolation, and direct print observation retain
the promoted signed expression. Assignment, declaration, array element, struct
field, parameter, and return boundaries apply one conversion to `uint32` or
`uint`, preserving fixed-width storage and transport semantics. No signed
checked-arithmetic route is changed.

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

The review-blocking observation source is committed as
`tagged_uint32_literal_observation_contexts.sn`. It covers the original
`(-1u32) == 4294967295u32` and interpolation forms plus a nested unary chain.
`tagged_unsigned_literal_transport.sn` separately covers declarations, direct
assignments, normal-array elements, value-struct fields, parameters, returns,
and comparison after transport. Their SHA-256 values are respectively
`c48507a32be4cc61d395aa5f6746b0487aa20b8d7787ccb07a16affb679d1540` and
`0f79e98527c078bac4994f5181a9fb3a10d3efd62363c9a2f1ea3f5ca02e8e17`.

The required fresh tagged smoke is
`/tmp/sindarin-s2-u32-final2-smoke-KhTNt4`. Exact tagged C captures are in
`/tmp/sindarin-s2-u32-final2-tag-TuesOz`, and exact Rust captures are in
`/tmp/sindarin-s2-u32-final2-rust-b2ogyT`. Each feature directory contains 18
successful compile/run/expected comparisons: two unchanged committed sources,
default/checked/unchecked, and O0/O1/O2. Every child status and executable
existence check was asserted. Captured stdout was compared byte for byte to the
committed expectation; stderr was retained separately, with no output, path,
ANSI, or newline normalization. The 18 direct tag/Rust stdout comparisons are
recorded in
`/tmp/sindarin-s2-u32-final2-rust-b2ogyT/tag-rust-comparisons.tsv`.

Independent review then exposed a mixed-width comparison in which the other
operand's rank is significant. The exact source is committed as
`tagged_unsigned_literal_mixed_width_comparisons.sn` (SHA-256
`01d3a34f17a2d15d7145f54836bb7516c79d8c78c86ffab8fcda7d9e528521ce`).
Its first comparison, `(-1u) < uint32(0)`, observes the signed promoted
literal in checked/default O0 and O1 modes, but uses the raw C usual integer
conversions in unchecked mode and default O2. Its second comparison,
`(-1u32) > uint(0)`, is true in every mode: checked `<` and `>` dispatch by
the left operand width, while the raw form promotes against the opposite
`uint` operand. The reviewer's unnormalized nine-mode captures are preserved
at `/tmp/sindarin-numeric-5898-u32-matrix-rTTs5v`; they replace an earlier
withdrawn shorthand description of this result.

`tagged_unsigned_literal_mixed_width_once.sn` is the reviewer's unchanged
side-effect probe (SHA-256
`44c93092a8ac92a7e06060dbe7af52c41b6848f1d1f1a4e0b56064ed45dbf652`).
It prints `true` followed by `1` in the tag, proving that selecting the
comparison width must not evaluate the function operand again. The original
review evidence is `/tmp/sindarin-numeric-5898-once-B0Vnhh`.

A fresh required smoke passed at
`/tmp/sindarin-s2-numeric-rank-smoke-3sEA1l`. The strict replacement matrix is
`/tmp/sindarin-s2-numeric-rank-matrix-3ZZGFq`: the two review probes plus the
existing observation and transport regressions, each under
default/checked/unchecked and O0/O1/O2. All 36 tagged and all 36 Rust compiles
returned zero and produced executables; all 72 executions returned zero; and
all 72 direct stdout/stderr comparisons were byte-identical. Each child
status, executable check, raw stream, and source hash is retained. No output,
diagnostic, path, ANSI, or newline normalization was applied.

Unchecked compound binary expressions retain their separately measured
optimizer boundary. C-rejected and undefined probes remain outside parity
claims.
