# Rust pipe block strings

The tagged `v0.0.83` pipe block syntax already reaches the ordinary string
literal and interpolated-string model. The Rust gap for
`tests/integration/test_pipe_string.sn` was the adjacent `str.splitLines()`
call, followed by the fixture's built-in assertions.

The Rust backend now admits `splitLines()` for its current `String`
representation. Lowering:

- borrows and binds the receiver once, keeping an owned temporary alive until
  every line has been copied;
- recognizes LF, CRLF, and CR line endings exactly as the tagged runtime does;
- returns independently owned `String` elements and does not consume a named
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

The separate byte-string work replaces Rust `String` with `SnString` and also
touches string-call validation and rendering. When that representation lands,
this lowering must be composed by retaining the once-only receiver/index
contract and substituting its byte-preserving line-copy operation. This slice
does not depend on or edit `SnString` internals.
