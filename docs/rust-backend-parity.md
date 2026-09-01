# Sindarin Rust Backend — Parity Audit

**Status:** Finalized. Branch `audit/rust-parity-architecture-v2` (base `origin/main`), tracking `origin/audit/rust-parity-architecture-v2`.
C is the default target (`src/compiler.c:31` `options->target = TARGET_C`); Rust is opt-in via `--target rust` / `--emit-rust`.
Reconciled against the independent audit `audit-rust-parity-tests.md` @ `1f26b33` on `audit/rust-parity-tests-v2` (fetched, not merged/cherry-picked).

## Baseline — historical audit baseline (build & test results)

> These results are the **historical audit baseline** recorded at commit `940d5593b14a99454671b3004ada8592fddf9734` on `audit/rust-parity-architecture-v2`. They document the audit snapshot, **not the current state**: the current rgen suite has grown from the historical **47** to **52** positive tests, while the C-side counts (cgen/mgen/integration/exploratory) are unchanged.

| Gate | Result |
|---|---|
| `make setup` (fresh worktree, run **once** at start) | PASS — installed sindarin v0.0.83 to `~/.sn`; `sn --install` pulled `sindarin-pkg-libs` v0.0.18 (linux-arm64), `-sdk`, `-test`. |
| `make test-rgen` | PASS — **47/47** Rust generation tests green (historical audit baseline; **current** rgen is **52**). |
| `make build && make test` (exact, unpiped, unredirected) | PASS — unit **1622**, cgen **107**, rgen **47** (historical; current **52**), rgen-errors **10**, mgen **79**, integration **1142**, integration-errors **58**, exploratory **224**, exploratory-errors **11**. |
| host `rustc --version` | `rustc 1.93.1` — verified on **Spark 1** (read-only check) and **Spark 2**. |

## Methodology (verification)

- Every disputed claim from the independent audit was re-verified against actual source/tests before being written in:
  - `method_call`/`borrow_inferred_call` model kinds: `src/cgen/gen_model_expr.c:1046` (operator-overload → `method_call`), `:1883` (`borrow_inferred_call`), `:1908-1918`. CONFIRMED.
  - Stmt kinds `str`/`for_each_iter`/`lock`/`using`: `src/cgen/gen_model_stmt.c:94` (`str`), `:797` (`for_each_iter`), `:1019` (`lock`), `:1042` (`using`). CONFIRMED.
  - C cgen tests prove C support for `sizeof` (`tests/cgen/expr_sizeof.expected.c`), `spread` (`expr_spread.expected.c`), `thread_spawn/detach/sync`, `lambda`, `method_call` (`expr_call_method.expected.c`), `mem_struct_val_cleanup`, `mem_struct_ref_refcount`, `struct_operator_eq`. CONFIRMED.
  - Default target C: `src/compiler.c:31`; target parse `src/target/target.c:74` (c/rust/rs); toolchain gate hit only for executable output: `src/target/target.c:201`. CONFIRMED.
  - CI (`.github/workflows/ci.yml`) installs/selects the exact Rust **1.93.1** via `rustup` for **every OS matrix job** before setup/build/test and records `rustc --version --verbose`. CONFIRMED.
  - Rust param gating: method-param block `rust_target.c:1249-1284`, function-param block `:1397-1425`; `as_ref` restricted to `int`/`long` (`:1410-1413`), heap-free named structs only (`rust_heap_free_named_struct_type`, `:338-344`), relevant uses `:1264-1271`, `:1410-1415`; `rust_validate_expr` (`:575`). CONFIRMED.
  - `match`, `sizeof`, `typeof`, `value_of`, `address_of`, `spread`, `sized_array`, pointer/stepped slices all verified as C-supported + Rust-gap via `rust_validate_expr` (`rust_target.c:575`). CONFIRMED.

## Parity status legend

- **IBT** — implemented & behavior-tested (rgen: snapshot `.expected.rs` + `rustc` build + run + stdout diff). Because every rgen case snapshots, compiles, runs, and checks output, implemented behavior is **not** downgraded to snapshot-only when a dedicated test is missing — classify as **UT** (under-tested) instead.
- **SNAP** — Rust partial/template exists but no dedicated rgen positive test.
- **UT** — under-tested (implemented; few/no dedicated negative/edge tests).
- **REJ** — explicitly rejected with a dedicated single-line diagnostic (rgen-errors pins first line).
- **SIL** — silently unsupported: construct reaches the Rust validator and falls through to the generic "unsupported construct" message.
- **ABSENT** — C supports it; Rust has neither a template nor a validator case.

## Evidence ledger (verified line references)

| # | Claim | Evidence (verified) | Status |
|---|-------|----------|--------|
| E1 | Target abstraction; C default | `src/compiler.c:31`; `src/target/target.c:74,91`; PR #9 | CONFIRMED |
| E2 | Rust validation gate | `rust_target.c:1302` `rust_validate_model_impl` (globals `:1305`, lambdas `:1306`, threads `:1307`, type_decls `:1308`, native pragmas `:1323`), `:575` expr, `:995` stmt, `:192` structs, `:1182` methods | CONFIRMED |
| E3 | Rust lowering | `rust_target.c` — `rust_lower_checked_arithmetic` (`:1448`), `rust_lower_strings` (`:1496`), `rust_lower_array_searches` (`:1618`), `rust_mark_instance_method_clones` (`:1665`), `rust_lower_instance_method_clones` (`:1736`), `rust_lower_interpolation_formats` (`:1758`), `rust_lower_for_continues` (`:1938`), `rust_lower_integer_ref_parameters` (`:2045`) | CONFIRMED |
| E4 | Rust build/toolchain | `rustc_command`/`rustc_quoted` (`rust_target.c:11,17`), `rust_check_toolchain` (`:112`), `rust_build` (`:2112` build: `rustc --edition=2021`), `SN_RUSTFLAGS` (`:2120`) | CONFIRMED |
| E5 | C backend | `c_target.c:31,120` (modular C + `gcc_compile_modular`), `src/gcc_backend.c` | CONFIRMED |
| E6 | C-only expr kinds | `gen_model_expr.c:1046,1883,1908`; C partials `templates/c/partials/expr/{match,lambda,thread_*,address_of,typeof, sizeof, spread,method_call}.hbs` | CONFIRMED — Rust gap |
| E7 | C-only stmt kinds | `gen_model_stmt.c:94,797,1019,1042`; C partials `stmt/{lock,using,for_each_iter}.hbs` | CONFIRMED — Rust gap |
| E8 | PR #49 rejection | PR #49 diff → `rust_target.c:1148,1266` + `tests/rgen/errors/*` | CONFIRMED |
| E9 | PR #54 C-incompatibility | PR #54 body: "Instance-method `as ref` remains rejected because the C backend currently emits incompatible value arguments for pointer parameters." | CONFIRMED (evaluation item) |
| E10 | rgen coverage | 47 positive (`tests/rgen/*`) + 10 negative (`tests/rgen/errors/*`) | CONFIRMED |
| E11 | Baseline green | Baseline table above | CONFIRMED |
| E12 | C-only suites | integration 1142, integration-errors 58, explore 224, explore-errors 11, cgen 107 — all C target; **no `--target rust` integration suite exists** | CONFIRMED |
| E13 | CI rustc pin | `.github/workflows/ci.yml` installs/selects exact Rust **1.93.1** via `rustup` for every OS matrix job before setup/build/test and records `rustc --version --verbose` | CONFIRMED |

---

# SOURCE-LANGUAGE parity matrix

Columns: **feature/behavior | exact C implementation & tests | historical PRs | current Rust status | missing implementation/tests | semantic/architectural risk | dependency-ordered milestone + PR boundary**

| Feature / behavior | Exact C implementation & tests | Historical PRs | Current Rust status | Missing implementation / tests | Risk | Milestone + PR boundary |
|---|---|---|---|---|---|---|
| `fn` declarations + plain calls | `c_target.c` + `templates/c/partials/function.hbs`; `tests/cgen/expr_call*.expected.c`; `tests/integration/*` | #9 | **IBT** — `templates/rust/partials/function.hbs`; `tests/rgen/basic_functions.*` | — | Low | M0 done |
| Value structs + literals | `templates/c/partials/struct_typedef.hbs` + C integration struct tests | #10 | **IBT** — `templates/rust/partials/struct.hbs`, `expr/struct_literal.hbs`; `tests/rgen/struct_values.*` | — | Low | M0 done |
| `var x: T = init` | C `stmt/var_decl` | #9 | **IBT** — `templates/rust/partials/stmt/var_decl.hbs`; `basic_functions.*` | — | Low | M0 done |
| **Global variables** | C model key `globals`; C integration covers | — | **REJ** — `rust_target.c:1305` "does not support global variables yet" | Add Rust globals (module-level `static mut`/`OnceLock`) + rgen test | Med | **PR-E** |
| **`type` aliases / `type_decls`** | C `type_decls` model key; parser type-arg handling | — | **REJ** — `rust_target.c:1308` "does not support type declarations yet" | Add Rust `type` alias | Med | **PR-E** |
| Primitive types (`int`/`long`/`int32`/`uint`/`uint32`/`byte`/`bool`/`char`/`double`/`float`/`string`) | C codegen for all; cgen primitive tests | #9 | **PARTIAL** — `rust_type_supported` (`rust_target.c:150-168`) accepts these kinds; `int`/`long` as-ref only (see param rows) | — | Low | M0 done |
| **Packed / serializable structs** | C struct variants + cgen fixtures `struct_packed`, `struct_handle_fields`, `expr_struct_literal_handle`; **serializable source support has no dedicated cgen fixture** (handled via encoder/cleanup path) | — | **REJ** — `rust_target.c:205-212` rejects packed/serializable structs and non-val `mem_mode`; `tests/rgen/errors/heap_owning_*` | Implement C-equivalent behavior; any exception requires explicit user approval | High | **PR-D** |
| **Native structs** | C native struct codegen | #3 | **REJ** — `rust_target.c:205` rejects native structs (native-struct FFI) | Implement C-equivalent behavior (native-struct FFI); any exception requires explicit user approval | High | **PR-H** |
| Numeric binary ops (checked) | `gen_model_expr.c` + `--checked`; cgen `expr_binary_checked` | #9 | **IBT/UT** — `rust_lower_checked_arithmetic` (`:1415`) → `checked_*`; `basic_functions.expected.rs`; **no rgen overflow test** (implemented-but-under-tested) | Add `int_checked_overflow` rgen test | Med | **PR-B** |
| **Numeric compound assign** `x += 1` | C `templates/c/partials/expr/compound_assign.hbs`; cgen covers | — | **REJ (specific)** — `rust_target.c:808-826` whitelists string `+=` only; numeric `+=` is **not implemented** on Rust | Implement numeric compound assignments + rgen tests (implementation gap, not under-tested) | Med | **PR-C** |
| **Increment/decrement** | C `compound_assign.hbs` (inc/dec) | #9 | **REJ (specific)** — `rust_target.c:830-842` restricts inc/dec to variables + struct fields; `arr[i]++` / complex lvalues rejected | Add rgen tests for **supported** inc/dec (variables/struct fields) + implement the missing lvalue forms | Med | **PR-C** |
| Type conversions (`int`→`str` etc.) | C emits runtime conversion macros via the method-call path (`gen_model_expr.c:1414-1440`: `__sn__int_toChar`, `__sn__str_toInt`); **no dedicated cgen fixture** | — | **ABSENT / implementation gap** — Rust has no `toChar`/`toInt`/… conversion-method lowering (only printf-style format-specifier handling exists in `rust_target.c:408-558,1730-1825`) | Implement Rust conversion-method lowering + add rgen tests | Med | **PR-C** |
| **`match`** | C `templates/c/partials/expr/match.hbs`; cgen match tests | — | **SIL** — `rust_validate_expr` has no `match` case → generic "unsupported construct" | Add match lowering + template + rgen test | High | **PR-F** |
| **Operator overloads** (`method_call`) | C `gen_model_expr.c:1046`; `tests/cgen/expr_call_method.expected.c`, `struct_operator_eq.expected.c` | — | **SIL** — only `call` whitelisted; `method_call` kind → generic rejection | Implement C-equivalent behavior; any exception requires explicit user approval | High | **PR-F** |
| **`borrow_inferred_call`** | C `gen_model_expr.c:1883` | — | **ABSENT/SIL** — no partial/validator case | Implement C-equivalent behavior (Rust borrow-inference); any exception requires explicit user approval | Med | **PR-F** |
| **`sizeof`** | C `templates/c/partials/expr/sizeof.hbs`; `tests/cgen/expr_sizeof.expected.c` | — | **ABSENT** — no Rust partial/validator case | Add Rust `size_of` | Med | **PR-C** |
| **`typeof` / `value_of` / `address_of` / `spread`** | C expr partials `templates/c/partials/expr/{typeof,value_of,address_of,spread}.hbs`; `tests/cgen/expr_spread.expected.c` | — | **ABSENT** — no Rust partial/validator case | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-C** |
| **Sized struct arrays** | C `sized_array` codegen | — | **REJ (specific)** — `rust_target.c:672-680` requires supported element_type; struct element → rejected | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| **Pointer / stepped slices** | C pointer param codegen (`mem_*` cgen) | — | **REJ (specific)** — pointer array slices + stepped slices rejected (`rust_target.c:689-710`); PR #54 C-incompatibility (evaluation item) | Implement C-equivalent pointer/stepped-slice emission; any exception requires explicit user approval | High | **PR-D** |
| `if` / `else` | C `stmt/if.hbs` | #9 | **UT** — Rust partial exists (`stmt/if.hbs`); no dedicated rgen test (implemented-but-under-tested, not snapshot-only) | Add `if_else` rgen behavioral test | Low | **PR-B** |
| `while` | C `stmt/while.hbs` | #9 | **UT** — partial exists; no dedicated rgen test (implemented-but-under-tested) | Add `while_loop` rgen behavioral test | Low | **PR-B** |
| `for` + `continue` | C `stmt/for.hbs` | #18 | **IBT** — `tests/rgen/c_style_for_continue.*`; `rust_lower_for_continues` (`:1905`) | — | Low | M0 done |
| `for_each` | C `stmt/for_each.hbs` | #12 | **IBT** — `tests/rgen/array_iteration.*` | — | Low | M0 done |
| **`for_each_iter`** | C `stmt/for_each_iter.hbs`; `gen_model_stmt.c:797` | — | **SIL** — no Rust partial/validator case | Add partial + validator case | Med | **PR-C** |
| **`using` / `import` / namespaces** | C `stmt/using.hbs`; `tests/integration/imports/*` (PR #8 fixed self-package resolution) | #8 | **SIL** — `using`/`import` model kinds (`gen_model_stmt.c:1042`); no Rust partial/validator case | Add Rust import/namespace emission | Med–High | **PR-E** |
| **`lock` / `release`** | C `stmt/lock.hbs`; `gen_model_stmt.c:1019` | — | **SIL/ABSENT** | Add Rust mutex/lock statement | Med | **PR-G** |
| **`str` statement kind** | C `gen_model_stmt.c:94` | — | **SIL** — no Rust partial/validator case | Implement C-equivalent behavior; any exception requires explicit user approval | Low | **PR-C** |
| Functions / static / instance methods | C `function.hbs`, `struct_method.hbs` | #29,#30,#31,#35,#36,#40 | **IBT** — `tests/rgen/{static,readonly,mutating}_struct_methods.*`, `instance_function_calls.*`, `instance_static_calls.*`, `return_self.*`; `templates/rust/partials/struct_method.hbs` | — | Low | M0 done |
| **`main()` non-void / arguments** | C allows non-void main and main arguments (C model `has_main_args` → `int main(int argc, char **argv)`) | — | **IBT** — parameterized `main(args: str[])` is now **implemented**: Rust validator accepts exactly one `str[]` param and sets `rust_main_has_args`/`rust_main_args_name` (`rust_target.c:1367-1396`); `function.hbs` renders `let mut <name>: Vec<String> = std::env::args().collect();` in both the `void` and `int` main paths, preserving body returns; positives `main_args_{count,order,space,mutate}` + `main_args_int_exit` (`.exit` = expected code `4`, `main_int_exit` `.exit` = `7`, `main_int_fallthrough` no sidecar), negative `main_non_str_param` | Non-Unicode argv limitation recorded below | Low | **PR-A** |
| **`as ref`/`as val` struct params** (heap-free) | C param `mem_qual` codegen | #50,#51,#52,#53 | **IBT** — `tests/rgen/{as_ref,as_val,static_as_ref,static_as_val}_struct_parameter.*` | — | Low | M0 done |
| **int / long `as ref`** (scalar ref) | C `mem_*` cgen | #55,#56 | **IBT** — `tests/rgen/{int,long}_as_ref_parameter.*`; `rust_lower_integer_ref_parameters` (`:2012`) | — | Low | M0 done |
| **int32 / uint32 / byte `as ref`** | C supports these scalar types | — | **REJ (specific)** — `rust_target.c:1410-1413` limits `as_ref` to `int`/`long`; `tests/rgen/errors/int32_as_ref_parameter.sn` (int32 only tested; uint32/byte untested) | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| **Heap-owning struct params** | C refcount/cleanup (`mem_struct_ref_refcount`, `mem_struct_val_cleanup` cgen) | — | **REJ (specific)** — `rust_heap_free_named_struct_type` (`:338-344`), relevant uses `:1264-1271`, `:1410-1415`; 5 `tests/rgen/errors/heap_owning_*` | Add heap-owning (refcount) Rust support | High | **PR-D** |
| **Instance `as ref`** | C emits pointer params | #54 (impl. commit `dbce38e`) | **REJ (specific)** — `rust_target.c:1260-1271`; PR #54 (GitHub description + commit `dbce38e`): C emits incompatible value args for pointer params (evaluation item) | First fix the **shared C pointer-argument contract** (with C regression coverage), then enable the **equivalent Rust reference mapping**; any exception requires explicit user approval | High | **PR-D** |
| **Mutating instance `as val`** | C codegen | #54 | **REJ (specific)** — `rust_target.c:1268-1271` (rust_mutating + as_val); `tests/rgen/errors/mutating_instance_as_val_struct_parameter.sn` | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| **sync/memory qualifiers** | C memory qualifiers | #49 | **REJ (specific)** — method `rust_target.c:1256-1275`, function `:1404-1419` reject non-default `mem_qual`/non-`none` `sync_mod` | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| String interpolation + ops + format specifiers | C `sn_string.c`; cgen string tests | #13,#14,#15–#28 | **IBT** — `tests/rgen/string_interpolation.*`, `string_operations.*`, `string_format_specifiers.*`, `scientific/float_alternate/integer_alternate/space_sign/string_precision` | — | Low | M0 done |
| Arrays: literal/access/slice + methods | C `sn_array.c`; cgen `expr_array_access`, `mem_array_string` | #11,#19,#20,#21,#22,#23,#37,#38,#39,#41 | **IBT** — `tests/rgen/array_values.*`, `array_slicing.*`, `array_literal_flattening.*`, `array_search_methods.*`, `string_array_concat.*`, `string_array_join.*`, `struct_array_concat.*` | — | Low | M0 done |
| **Array method/type restrictions** | C array method codegen | #37,#38,#41 | **REJ (specific)** — definitions `rust_array_method_supported` (`:241`), `rust_array_concat_type_supported` (`:324`), `rust_array_copy_type_supported` (`:346`); validator uses `rust_target.c:864-910` | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| `copyOf` per element type | C `copy_of.hbs` | #42–#48 | **IBT** — `tests/rgen/{integer,string,bool,char,double,float,struct}_array_copy_of.*` | — | Low | M0 done |
| **Heap-owning copy/concat restrictions** | C `copy_of`/`concat` codegen | — | **REJ (specific)** — `rust_target.c:727-744` (copyOf), `:884-910` (concat/join method/type checks) | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-D** |
| **Generics** | C generics support (parser `type_args`) | — | **REJ** — `rust_target.c:1308` (type_decls) | Add Rust generics | High | **PR-E** |
| **Native functions / native structs** | C native codegen; repo rule: never emit `extern` forwards for native fns without bodies | #3 | **REJ** — `rust_target.c:1349-1356` (native fn), `:205` (native struct) | Add native fn FFI + native struct support | High | **PR-H** |
| **Native `#pragma source/include`** | C native C source/include handling | — | **REJ** — `rust_target.c:1316-1323` "native C source/include pragmas" | Implement C-equivalent behavior; any exception requires explicit user approval | Med | **PR-H** |
| **Packages / SDK interaction** | C runtime `src/runtime/sn_*.c/h`; SDK `sindarin-pkg-sdk` is C-runtime-based | — | **ABSENT** — Rust codegen is std-only; no SDK/runtime counterpart | Decide SDK-on-Rust strategy | High | **PR-E** |

---

# RUNTIME & SDK PARITY

| Feature / behavior | Exact C implementation & tests | Historical PRs | Current Rust status | Missing implementation / tests | Semantic / arch. risk | Milestone + PR boundary |
|---|---|---|---|---|---|---|
| Standalone std-based Rust (no C runtime linkage) | C runtime `src/runtime/sn_*.c/h` (`sn_string`, `sn_array`, `sn_thread.h`); runtime cgen tests | #9 | **PARTIAL / DESIGN DIVERGENCE** — Rust std types (`std::string`, `Vec<T>`, plain structs) implement only a **subset**; C-runtime-equivalent **semantics, SDK, cleanup/finalization, and concurrency are not established** | Implement C-equivalent runtime behavior (keep as parity work); any exception requires explicit user approval | High | **PR-D** |
| String / array ownership & cleanup | C `src/cgen/ownership.c` (move/clone); `sn_string.c`; cgen `mem_array_string`, `mem_val_struct_copy` | #13 | **IBT/UT** — `rust_lower_strings` marks `rust_needs_clone` (`rust_target.c:1496-1593`); `tests/rgen/string_operations.*` (implemented-but-under-tested) | Add behavioral clone-vs-move test | Med | **PR-B** |
| `copyOf` deep-copy (heap-free structs) | C `copy_of.hbs`; cgen covers | #42–#48 | **IBT** — `tests/rgen/struct_array_copy_of.*` | — | Low | M0 done |
| **Heap-owning struct params / refcount cleanup** | C `mem_struct_refcount`, `mem_struct_val_cleanup` cgen | — | **REJ (specific)** — `rust_heap_free_named_struct_type` (`:338-344`); 5 `tests/rgen/errors/heap_owning_*` | Add refcount / `Drop`-based cleanup | High | **PR-D** |
| **Serializable / packed structs** | C cgen `struct_packed`, `struct_handle_fields`, `expr_struct_literal_handle`; serializable source support (encoder finalization idempotency, PR #5) — **no dedicated cgen fixture for serializable** | #5 | **REJ** — `rust_target.c:205-212` rejects packed/serializable structs | Implement C-equivalent behavior; any exception requires explicit user approval | High | **PR-D** |
| **Native structs (struct FFI)** | C native struct codegen | #3 | **REJ** — `rust_target.c:205` rejects native structs (handled under native-struct FFI) | Add native-struct FFI support | High | **PR-H** |
| **Finalization / disposal / `using`** | C `val_cleanup` + `sn_serial.h` encoder finalization (PR #5); `tests/integration/test_serializable_encoder_cleanup.*` | #5 | **ABSENT** — no Rust `Drop`/finalization path | Add finalization/`using` + test | High | **PR-E** |
| **Packages / SDK APIs** | C SDK `sindarin-pkg-sdk` (C-runtime-based); PR #8 fixed self-package resolution | #8 | **ABSENT** — Rust codegen is std-only; SDK APIs unavailable on Rust target | Decide SDK-on-Rust (std reimpl vs FFI bridge) | High | **PR-E** |
| **Lambdas / callbacks** | C `expr_lambda_basic`, `expr_lambda_capture_ref` cgen; C integration closure tests | — | **REJ** — `rust_target.c:1306` "does not support closures yet" | Add Rust closure lowering (`Fn`/`FnMut`) | High | **PR-F** |
| **Threads / `lock` / `release`** | C `sn_thread.h`; `templates/c/partials/expr/thread_{spawn,detach,sync}.hbs`; cgen `expr_thread_*` | — | **REJ** — `rust_target.c:1307` "does not support threads yet"; `lock`/`release` stmt kinds unhandled (SIL) | Add `std::thread` + `lock` statement | High | **PR-G** |
| **Native FFI / `#pragma source` / `#pragma include`** | C native codegen; repo rule: never emit `extern` forwards for native fns without bodies | #3 | **REJ** — `rust_target.c:1349-1356` (native fn), `:1316-1323` (pragmas) | Add `extern "C"` FFI + pin rgen-error negatives | High | **PR-H** |

# DIAGNOSTIC PARITY

| Feature / behavior | Exact C implementation & tests | Historical PRs | Current Rust status | Missing implementation / tests | Semantic / arch. risk | Milestone + PR boundary |
|---|---|---|---|---|---|---|
| Shared malformed-program diagnostics (parse/type errors) | `src/diagnostic.c` (phase + file:line context); 1609 unit tests cover lexer/parser/type_checker | — | **PARITY (target-neutral)** — shared front-end, target-agnostic | — | Low | — |
| Rust-target-specific emission diagnostics | `rust_target.c` plain `fprintf(stderr, "Error: Rust target ...")`; **no file/line context**; first-line negatives pinned in `tests/rgen/errors/*` (currently **10**); SIL constructs (`match`/`method_call`/`sizeof`/`using`/`lock`/`for_each_iter`) fall through to the generic "unsupported construct" message with no dedicated negative test | #49,#54 | **UT** — add file/line richness + dedicated negatives for SIL constructs | Med | **PR-C/PR-F/PR-G** |
| Rust toolchain availability & compile-stage failures | `rust_check_toolchain` (`rust_target.c:112`); gate invoked at `src/target/target.c:201` only for executable output | #9 | **IBT** — the new `rust-toolchain` suite (**4 cases**; `tests/rust-toolchain/basic.sn` + `tests/fixtures/fake_rustc.c`) behavior-tests the toolchain diagnostics. The cases distinguish three distinct failure modes: **case 2 = missing executable** (point `SN_RUSTC` at a nonexistent binary → toolchain-unavailable diagnostic + nonzero exit); **case 3 = nonzero `--version` availability failure** (spaced fixture with `SN_FAKE_RUSTC_EXIT` set nonzero, so the `--version` check fails first → same diagnostic + nonzero exit); **case 4 = successful `--version` followed by a nonzero build** (`SN_FAKE_RUSTC_VERSION_EXIT=0`, `SN_FAKE_RUSTC_BUILD_EXIT=3`) pinning the exact stderr diagnostic text `Error: rustc failed to build generated source` | — | Low | **PR-A** |

# CLI / TOOLCHAIN / PLATFORM PARITY

| Item | Exact C implementation & tests | Historical PRs | Current Rust status | Missing implementation / tests | Semantic / arch. risk | Milestone + PR boundary |
|---|---|---|---|---|---|---|
| C default target | `src/compiler.c:31` | #9 | **PARITY** — default-target unit test `target_default_c` (`tests/unit/standalone/compiler_driver_tests.c`) asserts no explicit target keeps `TARGET_C` + `OUTPUT_EXECUTABLE`; C behavioral regression = 1142 integration + 224 exploratory tests (default C pipeline) | — | Low | **PR-A** |
| `--target` / `--emit-rust` / `--emit-c` aliases + conflict rules | `src/target/target.c:74` (c/rust/rs parse); `src/compiler.c:237-283` conflict handling | #9 | **IBT** — 13 new unit cases in the `Target Selection` section of `tests/unit/standalone/compiler_driver_tests.c` cover `c`/`rust`/`rs` selection, `--emit-rust` shorthand positives, unknown-target rejection, and all three conflict rules; the `--emit-c` positive path is covered by the strengthened pre-existing `emit_c_output_path` unit case (now also asserting `TARGET_C` + `OUTPUT_SOURCE`), so both shorthands are unit-tested; end-to-end behavior of `--emit-rust` + `--target rust` (emit → snapshot → `rustc` → run → stdout diff) already covered by the 52 rgen tests; `--emit-c` covered by 107 cgen tests | — | Low | **PR-A** |
| Emitted source & executable compilation | `rust_build` (`rust_target.c:2112`) invokes `rustc --edition=2021`; rgen harness builds + runs | #9 | **PARITY** — rgen emit→snapshot→`rustc`→run→output | — | Low | M0 done |
| `SN_RUSTC` override (space/apostrophe/ampersand toolchain path) | `rustc_command`/`rustc_quoted` (`rust_target.c:11,17`); `rust_check_toolchain` (`:112`); `rust_build` (`:2112`) | this branch | **IBT** — `rust-toolchain` case 1 copies `sn_fake_rustc` into a directory containing a **space, a single quote, and an ampersand** (e.g. `fake rustc 'dir' &`), sets `SN_RUSTC` to it, and asserts the captured invocation records: a `--version` toolchain-check record and an `--edition=2021` build record containing the `.rs` source and `-o <executable>` target | — | Med (residual `system()`/raw-flag shell-interpolation risk: the `SN_RUSTC` path is shell-quoted for space/apostrophe/ampersand, but raw `SN_RUSTFLAGS` flags are still interpolated unquoted into the shell command) | **PR-A** |
| `SN_RUSTFLAGS` / `-g` / `-p` / build dirs / keep-generated / config | `SN_RUSTFLAGS` getenv (`rust_target.c:2120`); profile flags (`:2122-2126`); `src/target/target.c:232-239` (`.sn/build/<target>/<base>_<pid>/`) | #9 | **UT / open (immediate follow-up)** — `SN_RUSTFLAGS` is env-only (no `sn.rust.cfg`); no test asserts flag/profile/build-dir/keep-generated propagation | Add `SN_RUSTFLAGS`/`-g`/`-p`/build-dir/keep-generated propagation tests (**PR-A**); user input required **only if a public configuration contract would change** | Med | **PR-A** |
| Host toolchain | `rustc 1.93.1` verified on **Spark 1** (read-only `rustc --version`) and **Spark 2** | — | **CONFIRMED** | — | Low | — |
| CI Rust toolchain | `.github/workflows/ci.yml`: `make setup`→`make build`→`make test`; matrix-wide `Pin Rust toolchain` step installs/defaults Rust **1.93.1** via `rustup` and records `rustc --version --verbose` | #9–#56 | **PINNED** for **1.93.1** across **Ubuntu/Windows/macOS** | Explicit Rust headline sample build/run | Med | **PR-A** |
| PR #9–#56 check/review evidence | **Laptop GitHub historical inspection:** all cross-platform **Build checks green**; **no review/comments** on any PR | #9–#56 | **Laptop GitHub historical inspection** (docs-only PR checks may be skipped by `paths-ignore`; re-inspect before each merge) | — | Low | — |
| Platform assumptions | Platform = `linux`/`darwin`/`windows` (Makefile; pkg libs `.sn/<pkg>/libs/<platform>/`) | — | **PARTIAL** — per-OS toolchain now pinned (Rust 1.93.1), but target-specific platform behavior remains under-tested | Document platform assumptions + per-platform testing | Med | **PR-A** |

# TEST-COVERAGE GAP

| Suite | Count | Target |
|---|---|---|
| rgen (Rust behavior) | **52 positive** | Rust |
| rgen-errors | **10** first-line negatives | Rust |
| rust-toolchain | **4** (case 1 space/quote/ampersand `SN_RUSTC` override + invocation records; case 2 missing executable; case 3 nonzero `--version` availability failure; case 4 successful `--version` + nonzero build, via `sn_fake_rustc` fixture) | Rust |
| unit | 1622 (1609 baseline + 13 CLI target-selection cases) | target-neutral |
| cgen | 107 | **C** |
| mgen | 79 | **C** |
| integration | **1142** | **C only** |
| integration-errors | **58** | **C only** |
| exploratory | **224** | **C only** |
| exploratory-errors | **11** | **C only** |

**rgen behavior:** every rgen test (run_tests.py:755) does emit (`--emit-rust` → `.expected.rs` snapshot) → `rustc --edition=2021` build → run binary → diff stdout vs `.expected`. **There is no Rust integration suite**; the 1366 C-side behavioral tests (1142 integration + 224 exploratory, plus 58 + 11 error tests) have **no `--target rust` equivalent**, so C-only features (files/dates/threads/closures/imports) compiled on Rust hit rejections pinned only by the 10 rgen-errors.

**Exit-code sidecar:** each rgen test may ship an optional `<name>.exit` sidecar file containing a single decimal integer that pins the expected process exit code. When the sidecar is absent, the runner defaults to expecting exit code **0** (run_tests.py:800-801); when present, the file must hold a valid decimal integer (empty or non-numeric sidecars fail the test, run_tests.py:802-810). `main_int_exit` uses `main_int_exit.exit` (value `7`) to pin the `std::process::exit` value emitted by a zero-argument `int` main; `main_int_fallthrough` has **no** `.exit` sidecar so the runner defaults to exit **0** — its `int` main reaches the closure's unconditional `return 0` fall-through (empty stdout).

**Argument sidecar:** each rgen test may ship an optional `<name>.args` sidecar — a **JSON array of strings** that the runner passes as the process's user `argv` (run_tests.py `.args` block). An **empty array** `[]` is valid (no extra args); an **empty file** (whitespace only), **invalid JSON**, a **non-array root**, **non-string elements**, or a **string with an embedded NUL** each fail the test distinctly. The sidecar is opened as **UTF-8**. These user args are appended after `argv[0]` (the binary path), so `args.length` in the program equals `1 + len(.args)` — this is what the `main_args_*` tests assert.

**Spark 1 evidence — `test_limitation_closure_array`:** during this audit the exploratory test `test_limitation_closure_array` (a closure capturing a local array, pushed from 3 concurrent threads) produced an ASAN `heap-buffer-overflow` **twice on Spark 1**, while the same focused baseline test passed **5/5 on Spark 2** and earlier full baselines were green. Classify it as a **nondeterministic / host-sensitive existing C concurrency/closure risk** (concurrent `SnArray` push → `sn_realloc` race), **not** a docs-branch or Rust-backend regression. Keep it in the **PR-G** (threads/closures) verification scope.

## Dependency-ordered implementation roadmap

**PR-A → PR-H are *milestones*, each decomposed into small, self-contained vertical PRs (not one giant PR).** Every vertical PR ships **positive + negative + behavioral** Rust tests (rgen + rgen-errors + a `--target rust` integration case). Any **shared compiler/IR** change adds **both** C (cgen/integration) and Rust (rgen/integration-rust) regressions. **C remains the unchanged default target.** Each substantive missing family in the matrix maps to **exactly one** milestone A–H (no family is dropped/N-A).

| Milestone (decomposed into small vertical PRs) | Vertical slice (implementation) | Prerequisites | Required tests (Rust) | C regression obligation | Risk |
|---|---|---|---|---|---|
| **PR-A** | **Coverage/CLI/toolchain + non-void main.** CLI/toolchain unit tests (default target = C; `--target rs` alias; `--emit-c`+`--target rust` conflict; missing `SN_RUSTC`; `SN_RUSTFLAGS`/`-g`/`-p` propagation; build-dir layout); `main()` non-void support. | — | positive + negative + behavioral (CLI unit + rgen main) | cgen + integration (C default unaffected) | Low |
| **PR-B** | **Already-implemented but under-tested behavior only:** `if`/`while` behavioral tests, checked-arithmetic/overflow test, string clone/move ownership, existing target/CLI aliases. | PR-A | positive + negative + behavioral | Shared IR edits need C cgen + integration regression | Low–Med |
| **PR-C** | **Low-risk missing expressions/operators/control flow:** `for_each_iter`, `str` stmt, `sizeof`/`typeof`/`value_of`/`address_of`/`spread`, numeric compound assignments (impl + tests), `inc/dec` (tests for supported forms + implementation of missing lvalue forms). | PR-B | positive + negative + behavioral | cgen `expr_sizeof`, `expr_spread` (C) | Med–High |
| **PR-D** | **Ownership/references/non-plain data:** sized struct arrays, pointer/stepped slices, array method/type restrictions, heap-owning copy/concat, heap-owning params + refcount cleanup, serializable/packed structs, instance `as ref`, mutating instance `as val`. | PR-C | positive + negative + behavioral | cgen `mem_struct_ref_refcount`, `mem_struct_val_cleanup` (C) | High |
| **PR-E** | **Modules/imports/using/type declarations/generics/packages/SDK/finalization:** `import`/`using`/namespaces, `type_decls`/generics, SDK-on-Rust strategy, finalization/`using` cleanup. | PR-D | positive + negative + behavioral | integration/imports suite (C) | High |
| **PR-F** | **Advanced expressions/operator overloads/lambdas/callbacks:** `match`, `method_call`, `borrow_inferred_call`, closures, operator overloads, callbacks. | PR-E | positive + negative + behavioral | cgen `expr_lambda_basic/capture_ref`, `struct_operator_eq` (C) | High |
| **PR-G** | **Concurrency:** `std::thread`, `lock`/`release`, `thread_spawn`/`detach`/`sync`. | PR-F | positive + negative + behavioral | cgen `expr_thread_*` (C) | High |
| **PR-H** | **Native/FFI:** native functions (with body), native structs, `#pragma source`/`include`, `extern "C"` FFI blocks. | PR-G | positive + negative + behavioral | native cgen + repo rule: never emit bare `extern` forwards for native fns without bodies | High |

# SEMANTIC / ARCHITECTURAL DECISION REGISTER

Four genuine contract forks. Implementation proceeds along the recommended starting direction; **pause only when the decision would change the language contract** (observable behavior, not internal mechanics).

| # | Decision | Evidence | Public-behavior consequence | Recommended starting direction |
|---|---|---|---|---|
| D1 | **PR #54 — instance-method `as ref` rejected on Rust** | **GitHub PR #54 description**: "Instance-method `as ref` remains rejected because the C backend currently emits incompatible value arguments for pointer parameters." (Implementation commit **`dbce38e`** — "Support Rust readonly instance as-val parameters"; validator `rust_validate_struct_methods`, `rust_target.c` ~L1074). | A method declared `as ref` whose receiver is a struct: C compiles (emits value/pair args), Rust rejects → a valid-on-C program errors on Rust. | **First establish/fix the shared C pointer-argument contract** (C regression coverage), **then** enable the equivalent Rust reference mapping (emit the Rust receiver as a reference). Do **not** introduce a Rust-only divergence; any exception requires **explicit user approval**. |
| D2 | **SDK strategy — native Rust vs C ABI bridge** | SDK package (`sindarin-pkg-sdk`) ships C sources + `libs`; Rust backend currently has no Rust-native SDK layer. | Determines whether SDK functions are reimplemented in Rust, or exposed to Rust via `extern "C"` ABI. | **Start with the C ABI bridge** (reuse existing C SDK via `extern "C"`), defer a native-Rust rewrite. A contract-affecting split needs user approval. |
| D3 | **Native pragmas/functions — sidecar C compilation vs Rust FFI** | `#pragma source`/`include` + native functions with bodies; repo rule: never emit bare `extern` forwards for native fns without bodies. | Native `.c` files must compile alongside Rust — either a **sidecar C compilation** (gcc_backend path) or direct **Rust FFI**. | **Start with Rust FFI (`extern "C"`)** for the simple case, and sidecar C compilation when a native fn has a body. Changing which files are compiled to which language is a contract change → pause for user approval. |
| D4 | **Thread panic/detach/synchronization semantics** | `src/runtime/sn_thread.h` (C runtime); C-only `expr_thread_spawn/detach/sync` (gen_model_expr.c). Rust's `std::thread` panic/detach/sync semantics differ from the C thread runtime. | Panic propagation, detach semantics, and join/barrier behavior observable to user code may diverge from C. | **Start by mirroring the C thread-runtime semantics** on `std::thread`; any semantic divergence (panic behavior, detach/join ordering) requires explicit user approval. |
| D5 | **Main argv Unicode contract — `std::env::args()` vs `args_os()`** | Normative docs define `str` as **UTF-8** (`docs/interop.md:213` "Null-terminated UTF-8 string"; `docs/strings.md:391` UTF-8 encoding). Rust `std::env::args()` yields owned UTF-8 `String` and **panics on a non-UTF-8 OS argv element**; `args_os()` + `to_string_lossy()` would silently replace invalid bytes with U+FFFD (a byte-changing lossy conversion, not parity). | A program receiving a non-UTF-8 argv element: the C backend's `strdup(argv[i])` copies the raw bytes **without** boundary/encoding validation, while Rust `std::env::args()` **enforces the declared UTF-8 invariant** by panicking. | **Use strict `std::env::args()`** (matches the normative UTF-8 `str` contract; reject lossy `args_os()`). Track **C bootstrap boundary/encoding validation** as a **diagnostic/runtime consistency risk** (C currently does not validate) — **not** an approved target exception; revisit if the C backend gains argv validation. |

# VALIDATION / MERGE GATES

Per-PR gate sequence (manual, in order):
1. **Fresh worktree:** run `make setup` **once** at the start (toolchain check).
2. **Targeted Rust tests:** run the milestone's new rgen + rgen-errors + `--target rust` integration cases.
3. **`make test-rgen`:** full Rust generation/behavioral suite green.
4. **`make build && make test`:** C default target + full suite green (C regression obligation).
5. **Independent verification on another Spark machine** (second machine re-runs build + tests to confirm reproducibility).
6. **Laptop: inspect the actual `git diff` + GitHub check runs** before merge.
7. **Docs-only caveat:** CI may **skip** docs-only PRs via `paths-ignore`, but the **manual gates above still apply** — never skip the gates because CI skipped.
8. **Merge order:** merge **serially** in milestone order (A→H); **refresh/rebase** each PR onto the updated main before merging.

# CONCLUSION / STATUS

- **Parity is NOT achieved.** The Rust backend implements a partial subset (value structs, arrays, strings, struct methods, copyOf/concat, int/long `as ref`); C-supported features across source language, runtime/SDK, diagnostics, and concurrency remain open.
- **No C-supported gap is N/A / accepted target exception.** Every missing family maps to exactly one milestone (PR-A … PR-H); each is **implementation work**, not a dismissed category.
- **Known unsupported paths require action:** each is either implemented to match C-equivalent behavior, or resolved by an **explicit user decision** (see Decision Register D1–D4). Implementation pauses only when a choice would change the language contract.
- **C remains the default target** (`src/compiler.c:31`); Rust is opt-in and every change must keep C green.
