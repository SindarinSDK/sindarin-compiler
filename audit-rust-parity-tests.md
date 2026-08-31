# Rust Parity Tests — Independent Audit (spark2, worktree `sindarin-rust-parity-tests-v2`, branch `audit/rust-parity-tests-v2`)

Audit of the Rust target vs the C default using actual evidence only (no README/PR-description reliance).
All PRs #9–#56 were inspected via git merge commits (branch tip = PR #56 merge `940d559`).
Artifact only — not committed, not pushed.

---

## 1. Commands executed in this worktree and results

| Command | Result |
|---|---|
| `make setup` | PASS — downloaded `sindarin-0.0.83-linux-arm64.tar.gz`, installed to `~/.sn`, then `sn --install` pulled `sindarin-pkg-libs` (v0.0.18, linux-arm64), `sindarin-pkg-sdk`, `sindarin-pkg-test` from cache. |
| `make build` | PASS — CMake+Ninja Release build of `bin/sn` + `bin/tests`, staged runtime headers (`bin/include/runtime/*.h`, `bin/include/platform/*.h`) and prebuilt deps (`zlib`, `yyjson`, `json-c`). |
| `make test` (all suites) | PASS — 0 failures: unit **1609**, cgen **107**, rgen **45**, rgen-errors **8**, mgen **79**, integration **1142**, integration-errors **58**, explore **224**, explore-errors **11**. Total 18.77s. (Full log: `/tmp/opencode/make_test.log`.) |
| `make test-rgen` | PASS — 45/45 in 620ms. |
| `rustc --version` (host toolchain) | `rustc 1.93.1` present on spark2. |

## 2. Test-harness evidence (scripts/run_tests.py)

- `_run_rgen_test_internal` (run_tests.py:755): rgen tests are **not snapshot-only** — they (a) run `sn <t>.sn --emit-rust -o t.expected.rs` and diff against the checked-in `.expected.rs` snapshot; (b) run `sn <t>.sn --target rust -o <exe>` (invokes `rustc --edition=2021`), (c) execute the generated binary and diff stdout against `<t>.expected`. So every one of the 45 rgen tests is behavior-tested end-to-end (emit → rustc build → run → output compare).
- `_run_rgen_error_test_internal` (run_tests.py:818): rgen-errors (8 tests) verify that unsupported constructs fail **at emission** and that stderr's first line matches the single-line `.expected` file (e.g. `Error: Rust target does not support a parameter of function 'rename'`).
- cgen (107): compiles with default C target, runs binary, compares output.
- mgen (79): `--emit-model` JSON comparison.
- integration (1142) / integration-errors (58) / explore (224) / explore-errors (11): **C target only** — no `--target rust` equivalent exists for any of these 1434 C-side behavioral tests.

## 3. Coverage inventory

Classification legend: **IBT** = implemented-and-behavior-tested (rgen: snapshot + rustc build + run + output diff), **SNAP** = snapshot-only, **NEG** = negative-only, **UT** = under-tested, **REJ** = explicitly rejected (dedicated error message), **SIL** = silently unsupported (falls to generic "unsupported construct"), **ABSENT** = absent.

### 3.1 Source language (declarations, expressions, operators, conversions, control flow)

| Area | Evidence | Class |
|---|---|---|
| `fn` declarations + plain calls | `tests/rgen/basic_functions.sn/.expected/.expected.rs` (PR #9); validated in `rust_target.c` `rust_validate_model_impl` function loop (is_native rejected, return type whitelist) | IBT |
| Struct value types, struct literals, field access | `tests/rgen/struct_values.*` (PR #10); `rust_validate_structs` rejects native/packed/serializable structs and non-val mem_mode | IBT |
| Arrays: literals, access, concat, push/pop/insert/remove/reverse/clear/clone/contains/indexOf | `tests/rgen/array_values.sn`, `array_concat.sn`, `array_search_methods.sn`; method whitelist `rust_array_method_supported` (rust_target.c:133) | IBT |
| Array iteration `for_each` | `tests/rgen/array_iteration.sn` (PR #12) | IBT |
| C-style `for` + `continue` semantics (increment runs on continue) | `tests/rgen/c_style_for_continue.sn` (PR #18); lowering `rust_mark_for_continues` (rust_target.c:1770) | IBT |
| `if`/`while` partials exist (`templates/rust/partials/stmt/if.hbs`, `while.hbs`) | No dedicated rgen test for `while` or `if/else` behavior | **SNAP** (partial exists, no behavioral test) |
| `match` | No Rust stmt partial (`templates/rust/partials/stmt/` has no match.hbs; C has `templates/c/partials/stmt/`? — match handled via C's expr `match.hbs`); `rust_validate_stmt` has no `match` case → generic rejection | **SIL** |
| String interpolation `$"..."` | `tests/rgen/string_interpolation.sn` (PR #13) | IBT |
| String ops: `+`, `+=`, methods (contains/startsWith/endsWith/trim/toUpper/toLower/substring/replace/charAt/indexOf) | `tests/rgen/string_operations.sn` (PR #14); `rust_string_method_supported` (rust_target.c:144) | IBT |
| String format specifiers `%05d`, hex/octal, fixed/scientific/precision/space-sign/alternate | `tests/rgen/string_format_specifiers.sn`, `integer_alternate_formatting.sn`, `float_alternate_formatting.sn`, `scientific_formatting.sn`, `space_sign_formatting.sn`, `string_precision_formatting.sn` (PR #15–#28); parser in `rust_parse_format_spec` (rust_target.c:248) | IBT |
| Array slicing (no step, no pointer slices) | `tests/rgen/array_slicing.sn` (PR #22); pointer/stepped slices rejected with explicit errors (rust_target.c:585-595) | IBT + REJ |
| Array literal flattening (spread/range elements) | `tests/rgen/array_literal_flattening.sn` (PR #23); `rust_flatten` annotation (rust_target.c:514) | IBT |
| `copyOf` per element type | `tests/rgen/{integer,string,bool,char,double,float,struct}_array_copy_of.sn` (PR #42–#48) | IBT |
| Struct methods: static (PR #29), readonly (PR #30), mutating `&mut self` (PR #31), field increment/decrement (PR #34) | `tests/rgen/static_struct_methods.sn`, `readonly_struct_methods.sn`, `mutating_struct_methods.sn`, `struct_field_increment.sn` | IBT |
| Struct array mutation / index assignment | `tests/rgen/struct_array_mutation.sn` (PR #32), `struct_array_index_assignment.sn` (PR #33) | IBT |
| Instance function/static calls, `return self` | `tests/rgen/instance_function_calls.sn` (PR #36), `instance_static_calls.sn` (PR #35), `return_self.sn` (PR #40) | IBT |
| as-ref / as-val struct params (function + static + instance + readonly instance) | `tests/rgen/{as_ref,as_val,static_as_ref,static_as_val,readonly_instance_as_val}_struct_parameter.sn` (PR #50–#54) | IBT |
| int/long as-ref params (deref marking) | `tests/rgen/int_as_ref_parameter.sn` (PR #55), `long_as_ref_parameter.sn` (PR #56); lowering `rust_lower_integer_ref_parameters` (rust_target.c:1906) | IBT |
| Type conversions (`int`→`str` etc.) | C-side cgen/mgen cover conversions; **no rgen test** | **ABSENT** (Rust) |
| Operator overloading on structs (`method_call`, `borrow_inferred_call`) | `gen_model_expr.c:1046,1908` emits `method_call` kind; `rust_validate_expr` only whitelists `call` → generic rejection | **SIL** |
| `sizeof`/`typeof`/`address_of`/`value_of`/`spread` | C expr partials exist (`templates/c/partials/expr/*.hbs`); no Rust partials, no validator case | **SIL** |

### 3.2 Functions / methods / structs / namespaces / imports / packages / generics

| Area | Evidence | Class |
|---|---|---|
| Namespaces / imports (`using`, `import`) | C integration suite covers (`tests/integration/imports/*`); Rust target: `using`/`import` statement kinds exist in model (`gen_model_stmt.c`), no Rust partials, no validator case; PR #8 fixed self-package resolution on C only | **SIL** (no rgen import test) |
| Packages / SDK usage in Rust target | Rust codegen is std-only; SDK (sindarin-pkg-sdk) is C runtime-based (`src/runtime/sn_*.c/h`) — **ABSENT** in Rust |
| Generics / type declarations | `rust_validate_model_impl` rejects non-empty `type_decls` with "Rust target does not support type declarations yet" | **REJ** |
| Native functions (`is_native`) | `rust_validate_model_impl` function loop rejects: "Rust target does not support function 'X' yet" (rust_target.c:1242) | **REJ** |

### 3.3 Strings / arrays / ownership / references / copy / cleanup

| Area | Evidence | Class |
|---|---|---|
| Owned string move/clone semantics at call sites | `rust_lower_strings` marks `rust_needs_clone` on string args passed to functions/struct methods (rust_target.c:1410-1475) | **SNAP/IBT** (verified only via `.expected.rs` snapshots; no dedicated behavioral test asserting clone vs move outcomes) |
| `copyOf` deep-copy of heap-free structs | `tests/rgen/struct_array_copy_of.sn` (PR #48) | IBT |
| Cleanup/finalization (C `val_cleanup`, encoder finalization — PR #5 fixed C-side `sn_serial.h`) | C-only: `tests/integration/test_serializable_encoder_cleanup.*`; **no Rust equivalent** | **ABSENT** |
| `self` ownership in mutating methods (`has_heap_fields`) | `rust_mark_instance_method_clones` (rust_target.c:1526); readonly-instance-as-val restricted to heap-free structs (PR #54 negative: `heap_owning_instance_as_val_struct_parameter`) | IBT + NEG |

### 3.4 Lambdas / closures / callbacks / threads / concurrency

| Area | Evidence | Class |
|---|---|---|
| Lambdas/closures | Model key `lambdas` non-empty → "Rust target does not support closures yet" (rust_target.c:1198) | **REJ** |
| Threads | Model key `threads` non-empty → "Rust target does not support threads yet" (rust_target.c:1199); `thread_spawn`/`thread_detach`/`thread_sync` C partials exist, no Rust partials | **REJ** + **SIL** (thread_* expr kinds unhandled) |
| `lock`/`release`/`using`/`for_each_iter` stmt kinds | Present in C partials (`lock.hbs`, `using.hbs`, `for_each_iter.hbs`); absent in Rust; validator falls through → generic "unsupported construct" | **SIL** |
| Fire-and-forget / `str` stmt kind | `gen_model_stmt.c:94` | **SIL** |
| Callbacks (function-pointer style) | C-only integration coverage; Rust target has no callback primitive (no Fn/FnMut mapping tests) | **ABSENT** |

### 3.5 Native / FFI / target-specific behavior

| Area | Evidence | Class |
|---|---|---|
| Native functions with bodies vs extern forwards | PR scope: native fns rejected for Rust ("does not support function"); repo rule "NEVER generate extern forwards for native sindarin functions without bodies" | **REJ** |
| `#pragma source` / `#pragma include` | Rejected: "native C source/include pragmas" (rust_target.c:1213) | **REJ** |
| Checked arithmetic (`--checked` → `checked_add` etc.) | `rust_lower_checked_arithmetic` (rust_target.c:1309) maps add/sub/mul/div/rem to `checked_*` | **SNAP** (no rgen test exercises `--checked` overflow behavior; cgen has `expr_binary_checked.sn` C-side) |
| Unchecked arithmetic (`-O2` default) | Default; no dedicated negative test for overflow divergence | **UT** |

### 3.6 Diagnostics / malformed programs / unsupported-feature handling

- Rust-target diagnostics are plain `fprintf(stderr, "Error: Rust target ...")` strings; `tests/rgen/errors/*` (8 tests) assert only the first line of stderr.
- C diagnostics use `diagnostic.c` (phases, file:line context). **No parity**: Rust errors carry no file/line context; negative tests don't verify positions. Class: **UT**.
- Malformed programs (parse errors, type errors) are shared (lexer/parser/type_checker) and unit-tested (1609 unit tests incl. `tests/unit/lexer`, `parser`, `type_checker`), so malformed-program diagnostics are target-neutral and adequately tested; **unsupported-feature handling** for Rust is tested only at emission stage (rgen-errors, 8 tests) — no test asserts that e.g. `--target rust` (build stage) still fails after a successful emit (the toolchain check `rust_check_toolchain` is only hit for executable output, target.c:200-202).

### 3.7 CLI / config / build scripts / Rust toolchain integration

| Item | Evidence | Class |
|---|---|---|
| Default target | `options->target = TARGET_C` (compiler.c:31); `sn --help`: `--target <target> Select target compiler: c or rust (default: c)` | Implemented, but **no CLI test** asserts default-target behavior |
| `--target` parsing | `target_kind_parse` accepts "c"/"rust"/"rs" (target.c:74-84); conflict rules in compiler.c:237-283 | **UT** (no unit/CLI test on conflict errors) |
| `--emit-rust` / `--emit-c` aliases | compiler.c:265-283 | Implemented; exercised only via run_tests.py rgen paths |
| Toolchain check | `rust_check_toolchain` (rust_target.c:17) — `rustc --version` via `SN_RUSTC`; failure message suggests `--emit-rust` | **UT** (no negative test for missing-rustc behavior) |
| Build flags | `rust_build` (rust_target.c:1973): `rustc --edition=2021 [profile] [SN_RUSTFLAGS] main.rs -o exe`; `-g`/`-p` flag mapping (rust_target.c:1983-1987) | **UT** (no test asserts `-g`/`-p`/`SN_RUSTFLAGS` effect on the Rust build) |
| CI toolchain | `.github/workflows/ci.yml` installs build-essential/python3 (Linux), LLVM-MinGW (Windows), but **no `rustup`/rust install step** — relies on runner-image-bundled Rust; `make test` runs rgen which calls `--target rust` → rustc must be on PATH. No pinning of rustc version. | Risk / **UT** |
| `--keep-generated` / build dir layout | target.c:232-239 (`.sn/build/<target>/<base>_<pid>/`) | C and Rust share layout; **UT** |
| Config files (`sn.<os>.cfg` staged in `bin/`) | Makefile install target; no Rust-specific config (e.g., rustflags config) | **ABSENT** (SN_RUSTFLAGS is env-only) |

## 4. False-confidence findings

1. **C-only integration coverage without Rust equivalents.** 1142 integration + 224 exploratory + 58 + 11 error tests all run the C target. The Rust target is only behavior-tested by the 45 rgen tests. A Rust user's program compiled with `--target rust` for features covered by the 1366 C-side tests (files, dates, threads, closures, imports) will hit rejections — none of those rejections are covered by positive tests except the 8 rgen-errors.
2. **Snapshot false confidence is largely avoided** (rgen builds and runs binaries), but the `.expected.rs` snapshots can drift silently: no test verifies that the *behavior* (`.expected` output) changes when the snapshot is regenerated by `make test-rgen` without review.
3. **CI runs rgen without an explicit Rust toolchain install** — if a runner image lacks rustc, 45 behavior tests fail with a toolchain error rather than a source-parity failure, masking real regressions (false pass/fail signal).
4. **`while`/`if` partials exist with zero dedicated rgen tests** → snapshot-only confidence (SNAP) for the most common control-flow statements.
5. **`method_call` / `borrow_inferred_call` / `match` / `lock` / `using` / `for_each_iter`** are rejected by generic "unsupported construct" with no dedicated negative test → a program using any of these fails at emit with a non-specific message (silently unsupported, no test pins the behavior).

## 5. Prioritized gap list (dependency-ordered PR proposals)

Each PR is small, self-contained, and depends only on its predecessor's files.

1. **PR A — `feature/rust-while-if-tests`**: add rgen tests `tests/rgen/while_loop.sn/.expected/.expected.rs` (while + break) and `if_else.sn` covering if/else with output assertions. Touch: `tests/rgen/*` only (partials already exist). *(Closes SNAP gaps for while/if.)*
2. **PR B — `feature/rust-diagnostics-errors`** (depends on A for harness familiarity): add rgen-errors negative tests for `match`, `method_call` (struct operator overload), `sizeof`, `using`, `lock` — pin the generic "unsupported construct" message (first line) and the specific "pointer/stepped slice" and "main must return void" messages. Touch: `tests/rgen/errors/*` + optionally `scripts/run_tests.py` (no code change needed — errors already emitted). *(Closes SIL/UT diagnostics gaps.)*
3. **PR C — `feature/rust-checked-arithmetic-test`** (depends on B): add `tests/rgen/int_checked_overflow.sn` running `--checked` to assert `checked_add` panics/handles overflow consistently with C behavior. Requires `scripts/run_tests.py` to accept a per-test compiler flag (small harness extension: optional `--checked` marker file `*.checked`). *(Closes the checked-arithmetic SNAP gap.)*
4. **PR D — `feature/rust-conversions-tests`** (depends on C): add rgen tests for `int→str`, `str→int` conversions and `sizeof`-free numeric literal suffixes (`42u`, `3.14`), plus a negative test for unsupported conversion combos. Touch: `tests/rgen/*`. *(Closes the ABSENT conversions gap.)*
5. **PR E — `feature/rust-cli-toolchain-tests`** (depends on D): unit tests in `tests/unit/standalone/` (extend `compiler_driver_tests.c`) asserting: default target is C; `--target rs` alias works; `--emit-c --target rust` conflict error text; missing-rustc behavior when `SN_RUSTC` points to a nonexistent binary; `SN_RUSTFLAGS` propagation in the rustc command line. *(Closes CLI/toolchain UT gaps.)*
6. **PR F — `feature/ci-rust-toolchain`** (depends on E): add `rustup` install + version pin in `.github/workflows/ci.yml` for all three OS legs, and a CI step `bin/sn samples/main.sn --target rust -o /tmp/sn-rust-sample && /tmp/sn-rust-sample`. *(Closes CI toolchain risk.)*
7. **PR G — `feature/rust-integration-suite`** (depends on F): introduce `tests/integration-rust/` + `run_tests.py` suite type `integration-rust` that compiles a curated subset of C integration tests with `--target rust` and compares output (start with 20–30 representative programs: strings, arrays, structs, control flow). Mark explicitly-rejected features (threads/closures/globals) as excluded-with-reason. *(Closes the biggest gap: C-only integration coverage.)*
8. **PR H — `feature/rust-native-ffI-tests`** (depends on G): add rgen-errors negatives for `#pragma source`, native function with/without body (pin "does not support function" and pragma errors), and a positive FFI test only after native-with-body support lands (track as open). *(Closes native/FFI REJ pinning.)*

Dependency chain: A → B → C → D → E → F → G → H. Each PR stays under ~150 lines of diff; all are mergeable independently in order.

## 6. Open questions

1. Is `match` intentionally out of Rust scope (explicit rejection PR) or an oversight? No PR in #9–#56 touched `match` — needs maintainer confirmation.
2. Should the Rust target ever support threads (std::thread) or remain REJ? `sn_thread.h` C runtime has no Rust counterpart.
3. `SN_RUSTFLAGS` env is the only config surface — should a `sn.rust.cfg` exist (parity with `sn.linux.cfg` etc.)?
4. Are `borrow_inferred_call` semantics (borrow inference for method calls) expected to be supported in Rust, or should it be an explicit rejection with a dedicated error (currently generic)?
5. Which subset of the 1142 C integration tests should seed the first `tests/integration-rust` suite?

## 7. Counts summary

- rgen (Rust behavior): 45 positive / 8 negative — the ONLY Rust-target behavioral suite.
- C-side suites: unit 1609, cgen 107, mgen 79, integration 1142, integration-errors 58, exploratory 224, exploratory-errors 11.
- Rust target source: `src/target/rust/rust_target.c` (2010 lines), `rust_render.c`; templates `templates/rust/module.hbs` + 24 expr partials + 8 stmt partials.
- PRs #9–#56: 48 merged PRs, each adding 1+ rgen positive test and 0–3 rgen-errors negatives + `rust_target.c` validation/lowering changes.
- Host toolchain: rustc 1.93.1; CI installs no Rust toolchain (image-dependent).
