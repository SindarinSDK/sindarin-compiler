# Rust pipe block strings

The tagged `v0.0.83` pipe block syntax already reaches the ordinary string
literal and interpolated-string model. The Rust gap for
`tests/integration/test_pipe_string.sn` was the adjacent `str.splitLines()`
call, followed by the fixture's built-in assertions.

The Rust backend now admits `splitLines()` for its byte-backed `SnString`
representation. Lowering:

- borrows and binds the receiver once, keeping an owned temporary alive until
  every line has been copied;
- recognizes LF, CRLF, and CR line endings exactly as the tagged runtime does;
- copies line byte slices directly into independently owned `SnString`
  elements, without UTF-8 validation or lossy conversion, and does not consume a named
  source string;
- binds an immediately indexed split result before evaluating the index, so an
  effectful receiver is not rendered twice; and
- evaluates an assertion condition and message once in source order before
  either continuing or emitting the tagged failure message and exiting with
  status 1.

All annotations and rendering changes are under `src/target/rust` and
`templates/rust`. The C model, templates, runtime, tagged sources, and existing
oracles are unchanged.

The focused `tests/rgen/pipe_string.sn` regression covers plain and
interpolated pipe strings, an effectful temporary receiver, named-source reuse,
immediate indexing, CR/LF variants, empty lines, owned indexed results, and
collisions with the private lowering names. Its emitted Rust is pinned by the
adjacent snapshot.

`tests/rgen/pipe_string_bytes.sn` is adjacent Rust coverage, not a tagged-corpus
parity count. The same source is accepted by the tagged compiler and checks
that invalid UTF-8 bytes survive CRLF/LF splitting, that its producer runs
once, and that returned line owners remain independent and readable.

This slice composes on the byte-string implementation without changing
`SnString` internals. It retains that implementation's byte-preserving
`splitLines` helper and adds only the pipe fixture's once-only receiver/index
contract and byte-exact assertion failure output.
