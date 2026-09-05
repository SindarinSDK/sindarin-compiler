# Rust tagged byte semantics

## Scope

This change restores the `v0.0.83` byte-storage arithmetic contract in the
Rust backend. The oracle is peeled tag commit
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62`; the implementation branch starts
at restoration checkpoint `48975cbe54c034c79c49642227a2e28eb4a8ba7b`.

Rust now uses target-local byte lowering for:

- binary `+`, `-`, `*`, `&`, `|`, `^`, `<<`, and `>>`;
- unary `-` and `~`;
- byte compound assignment, including explicit unchecked `/` and `%`;
- postfix `++` and `--` on supported locals, direct fields, iterator bindings,
  `as ref` parameters, and prepared by-value parameters.

Add, subtract, multiply, negate, increment, and decrement use Rust wrapping
operations. Shifts promote the byte to `u32`, perform the shift, and narrow to
`u8`, matching defined C promotion followed by byte storage. Binary operands
are evaluated once from left to right. Compound assignment evaluates its RHS
before borrowing the stable place, and postfix mutation borrows its place once.
Tuple-pattern temporaries keep compiler bindings out of scope while source
operands and places are evaluated, including when source names match the
generated spellings.

Signed `int` and `long` keep checked overflow behavior. This commit does not
change C generation, the shared model, or frontend type checking.

## Tagged oracle evidence

The committed comparison sources are:

- `tests/rgen/tagged_byte_wrapping.sn`, SHA-256
  `7fd22d6bf17760bf5b6ca4f5d929c33c1ba559376868cc4e188b074dba1ef9c1`;
- `tests/rgen/tagged_byte_by_value_mutation.sn`, SHA-256
  `08069f2d3b34fbc746a69d1564b041c21cd0892d53f53835e0a4cc7e54e28b85`.

Each unchanged source was compiled with the peeled tag C compiler and the
restoration-checkpoint Rust compiler for all 18 combinations of two fixtures,
three arithmetic selections (`default`, `--checked`, `--unchecked`), and three
optimizer selections (`-O0`, `-O1`, `-O2`). All 36 compilations and executions
returned zero. Corresponding C and Rust stdout and stderr were compared as raw
bytes with `cmp`; all 18 pairs matched. No diagnostic, path, ANSI, newline, or
numeric-output normalization was applied. The by-value matrix is under
`/tmp/sn-s2-byte-work/differential-20260905b`; the final wrapping matrix is
under `/tmp/sn-s2-byte-work/differential-20260905c`.

The earlier broad oracle probe is preserved at
`/tmp/sn-s2-byte-work/oracle/tagged_byte_core.sn` with results under
`/tmp/sn-s2-byte-work/oracle/results-20260905b`. It records the tagged C
temporary-expression behavior as well as stored byte behavior.

## Deliberately separate evidence

The tagged checked C templates call nonexistent `sn_div_byte` and
`sn_mod_byte` helpers for binary byte division and remainder. The exact
checked-division source and compiler streams are preserved under
`/tmp/sn-s2-byte-work/oracle/tagged_byte_checked_div.sn` and
`/tmp/sn-s2-byte-work/oracle/results-20260905b`. Those failing forms are not
counted as tagged-valid parity evidence. Rust checked division and remainder
remain on the diagnostic branch's logical checked-helper path.

A compound assignment nested directly in a comparison exposes the tagged C
template's missing grouping. The minimal source is
`/tmp/sn-s2-byte-work/oracle/tagged_byte_compound_context_raw.sn` (SHA-256
`b8bf3c3f4b32bd06ea91c54af0399335b87eb0d67d54d552e5e2920a0457efa9`).
Its generated C, streams, and statuses are under
`/tmp/sn-s2-byte-work/oracle/raw-compound-context`; generated C SHA-256 is
`4fa2dad56cfae99783bade251fc88e2ae81332debbe48f58d32b18dc76403146`.
The comparison fixture therefore sequences compound results into byte
variables before comparing them. The raw form remains evidence for separate C
work and is not claimed as C/Rust parity.

Tagged unchecked byte arithmetic and all tagged byte unary expressions use C
integer-promotion temporaries until a byte storage boundary. For example, the
preserved core probe prints `0x100` for unchecked `255b + 1b` and
`0xFFFFFFFF` for direct `-1b`, while storing either expression in a byte
narrows it. This Rust slice implements the byte-typed/storage result used by
assignments, returns, arguments, fields, and arrays. Exact observation of the
promoted temporary in a context such as direct printing would require
context-sensitive narrowing at every byte consumption boundary; it remains
explicit required parity work and is not covered by the byte-equality claim
above.

Shift counts from 0 through 31 are the defined C oracle range. Counts at least
32 and division or remainder by zero are excluded from valid tagged behavior
evidence.

## Composition boundary and follow-ups

The diagnostic branch owns checked-helper names and messages. This branch does
not name or emit those helpers. Its composition patch adds byte routing around
the existing Rust `binary`, `unary`, `compound_assign`, `increment`, and
`decrement` partials; the operation bodies live in new byte-only partials and
type selection lives in `rust_lower_byte.c`.

The tag also wraps `int32`, `uint32`, and `uint` helper arithmetic. Their Rust
restoration remains a required numeric slice because it needs type-width and
promotion-specific templates rather than reusing the byte-only `u8` lowering.
Signed 64-bit checked behavior must remain unchanged when that slice composes.
