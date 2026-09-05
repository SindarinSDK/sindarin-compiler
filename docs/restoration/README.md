# Tagged C restoration checkpoint

Baseline: annotated `v0.0.83` object `a896688aefaf68a0eb502b9baeebe8b0d6185e37`, peeled `79c20bdb8314aff3c778471ceab20bb8f9ca8d62`. Base main: `7af0d19232eac05951b73a92c92c221c65ed04e1` (270 intervening commits). The quarantined callable branch is not an input.

## Restoration and retained wiring

All tagged production changes under `src/` are restored byte-for-byte except `compiler.c`, `compiler.h`, and `main.c`. All eight C templates and all 39 modified tagged test files are exact tag bytes. `tagged-identity.json` records the per-path Git blob and SHA-256 manifest, including restored sample/docs/config paths.

The only production exceptions are target enums/options and CLI dispatch in `compiler.c`/`.h`, and the conditional C setup/Rust invocation in `main.c`. C keeps its original compiler availability check before package installation/parsing, original model/emit-C/executable code, artifact directory, errors, compilation and cleanup. Legacy emit-model precedence is restored. No AST, type-checker, optimizer, C JSON model, runtime or template metadata exception is retained.

CMake retains target/Rust compilation, private Rust projection compilation, Rust-template staging and the fake-rustc fixture. Makefile Rust test targets remain additive. The current test runner is separate regression evidence; the tag runner is the authoritative baseline.

## Rust-private interface for dependent workers

`src/target/rust/projection/rust_model.h` exports `rust_gen_model_build` and private model helpers. The projection was migrated from main, with every builder/ownership function and global state namespaced. Rust emission calls it directly. Modify these private projection files for new Rust metadata; do not modify `src/cgen`, shared AST/front-end semantics, C templates or runtime.

`RustVariableFacts` holds declaration facts outside the AST. A Rust-only scoped traversal reconstructs these facts from declarations and parameters. It does not change shared type checking or optimization. Existing Rust render/lower/validate code and templates are preserved.

PR115's sidecar source/header are preserved at `src/target/rust/cc_sidecar.*` and removed from the build and all C dependencies. They are an unbuilt migration input for the native Rust worker, who must provide its private configuration lookup before enabling it. The old public link-library getter is removed with the restored GCC header/driver.

## Verification status at checkpoint — incomplete, not integration-ready

Both Release builds succeeded with existing dependencies copied project-locally. No setup/installers or host configuration changes were used.

The unchanged tag runner against tag source/fixtures and against the correction with a separate exact-tag fixture checkout passed identically: 1,608 unit; 107 C generation; 79 model generation; 1,141 integration; 58 integration-negative; 224 exploratory; 11 exploratory-negative. Compiler and fixture hash manifests and detailed differential/toolchain verification are still being completed.

Current Rust rgen: 235 pass, 16 fail. The closure regressions pass after correcting a private projection scope-depth error. Remaining failures involve post-tag scalar-field as-ref admission, namespace type resolution and checked-arithmetic DCE expectations; each still needs exact tagged differential classification. No expectation was changed to hide these failures.

Logs are preserved on spark1 under `/tmp/sn-restore-tagged-c-evidence/`, including the complete tag-to-main diff, independent harness outputs, original post-tag versions of modified tests and build logs. Independent audit input: `/tmp/sindarin-independent-restoration-map.md`. Integration requires the independent audit and completed differential evidence.
