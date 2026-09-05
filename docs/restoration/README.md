# Restore v0.0.83 C compatibility

The correction restores the tagged C language, model, runtime, templates, build driver and default test harness while preserving the Rust backend behind a private projection. Base main is `7af0d19232eac05951b73a92c92c221c65ed04e1`; the intervening history has 270 commits. The quarantined callable branch is not an input.

The authoritative tag is annotated `v0.0.83`, object `a896688aefaf68a0eb502b9baeebe8b0d6185e37`, peeled commit `79c20bdb8314aff3c778471ceab20bb8f9ca8d62`. Remote tags/peels and ancestry agree. Git tagger time is `2026-08-29T18:44:14Z`; the release publication time supplied by root is `2026-08-29T18:54:03Z`.

## Exact restoration and exceptions

Every tag-tracked file is present. Only seven tag-tracked files differ, all enumerated with justification in `retained-wiring.json`: compiler options/main dispatch, CMake, Makefile, CI's Rust pin, and Rust README instructions. All other tagged files are exact Git blobs, including all 39 modified tagged fixture paths, the entire shared frontend/optimizer/C model/ownership/runtime, all C templates, `sn.yaml`, all platform configurations and `scripts/run_tests.py`. `evidence/tag-tracked-identity.tsv` is the exhaustive identity ledger; `tagged-identity.json` lists the 47 restored production/sample/config/doc paths and 39 restored test paths with SHA-256 and Git blobs.

C preserves its original toolchain availability check before package installation/parsing, emit-model precedence, C/model/executable generation bodies, artifact directory, diagnostics, compiler flags, linking and cleanup. The divergent C adapter and generic `target_compile(C)`/`target_compiler_for(C)` interfaces are removed. Only `rust_target_compile` uses the new artifact driver. No AST or C-model metadata exemption is retained.

PR115's sidecar is removed from the correction entirely. Its migrated source remains available at checkpoint `48975cbe` for the native worker's independently owned Rust route. The post-tag C headline CI step is removed; the Rust pin remains.

Two *added* `.expected` files attached new oracles to modified tagged sources: `tests/cgen/expr_binary_checked.expected` and `tests/exploratory/test_limitation_closure_array.expected`. Their unchanged main source/expectation snapshots are preserved under `post-tag-fixtures/`; they cannot remain next to the restored tag sources. Both restored tests pass their focused tag-runner checks. No tagged expectation was changed.

## Rust-private interface

`src/target/rust/projection/rust_model.h` exports `rust_gen_model_build` and its helpers. The main model construction was migrated into this directory; functions, state, ownership/type enums and guards are namespaced. Rust emission calls this projection directly. New Rust metadata belongs here, not in `src/cgen`, the shared AST/front end, C templates or runtime.

`RustVariableFacts` holds declaration facts outside the AST. Its scoped traversal reconstructs parameter qualifiers, synchronization, sized-array origin and iterator facts without altering shared checking or optimization. Private closure scope depth and iterator-protocol classification bugs found during migration are fixed. A compile-only check includes both C and Rust projection/ownership headers together successfully.

The default C runner is exactly the tag runner. `scripts/run_rust_tests.py` separately owns rgen/rgen-errors and six Rust toolchain checks, including Rust `.opt`, panic/exit, UTF-8 decoding and artifact assertions. Its C handlers/staging/lifecycle case are removed. Makefile Rust targets and README point to it; C commands are unchanged.

## Accepted baseline evidence

Both Release builds used existing project-local dependency copies. Tag `sn.yaml` declares only the libs dependency; reused SDK/test caches were operational copies, not added baseline dependencies. No tagged source/fixture references those extra package names. No host installers/configuration changes were performed.

The unchanged tag runner and exact tagged fixtures passed with both tag and correction:

| Suite | Tag | Correction |
|---|---:|---:|
| Unit | 1608 | 1608 |
| C generation | 107 | 107 |
| Model generation | 79 | 79 |
| Integration | 1141 | 1141 |
| Integration negative | 58 | 58 |
| Exploratory | 224 | 224 |
| Exploratory negative | 11 | 11 |

These full runs and emission comparisons identify checkpoint `48975cbe`, before the subsequent source-review fixes. Later changes are verified with affected focused checks and the exhaustive source-identity ledger, rather than rerunning an unchanged corpus.

Unit provenance was checked explicitly: the tag runner chooses `cwd/bin/tests`, independently of `--compiler`. The staged correction unit binary hash equals the correction build's binary; the correction binary was also executed directly at its absolute path, 1608/1608. Tag and correction unit evidence remain separate. Paths, binary hashes, fixture tree, runner blob and dependency identity are in `evidence/checkpoint-provenance.json`.

The **-O0-only** differential covers all 1691 tagged `.sn` sources, including supporting/negative inputs. Both `--emit-c` and `--emit-model` agree in exit status, diagnostic streams and generated bytes for every source. Generated C and model bytes are hashed without normalization. Diagnostics normalize only the per-invocation output file path, compiler installation directory and ANSI styling. Rechecking the saved diagnostics while preserving ANSI also finds zero differences. No source/output text, filenames inside models, numbers, errors or runtime expectations are normalized. The per-source ledger and scope are in `evidence/o0-emission-identity.tsv` and `evidence/o0-emission-summary.json`. This is not an all-optimization-mode or executable-output differential claim.

## Current regressions, separately classified

The main runner/main fixtures were run separately before restoring the default runner. Their C cases were also compared individually with the tagged compiler: all **1883** statuses and failure reasons match. Their **221** failures are 49 cgen, 36 mgen, 127 integration and 9 integration-negative; all paths, original changes, reason and disposition are recorded in `current-c-failures.json`. They assert post-tag output/model/arithmetic/ownership/admission/driver expectations that the tag also does not satisfy. Existing tagged cases pass with their unchanged tagged fixtures; main's rewritten fixture versions are not substituted for them.

Rust rgen has **235 passes and 16 post-tag semantic-expectation failures**: eight sources rejected by the tagged front end and eight checked-arithmetic DCE expectations inconsistent with the tag at the fixture's optimizer setting. Rust negatives initially had 174 passes and nine failures; one private iterator regression was fixed (focused 7/7 iterator negatives and 5/5 positives), leaving eight post-tag shared-language rejection expectations. All 20 closure cases and six Rust toolchain cases pass. `current-rust-failures.json` records every original failure and its exact tagged model/admission probe; it distinguishes the fixed migration regression from the remaining 24 semantic assertions. Rust rejection tests describe current target gaps and do not establish full parity.

`post-tag-source-inventory.json` classifies all 725 added source fixtures, including supporting inputs. Useful regressions and their expectations remain preserved. Added incompatible cases have **not** been silently excluded or rewritten; consequently the broad current/default corpus can still report their documented failures. Only the exact tag corpus is the C acceptance oracle.

## Review and remaining external evidence

The independent source map and checkpoint review live on spark1 under `/tmp/sn-restore-tagged-c-evidence/`. Full logs, compiler outputs and original harness scripts remain there. Selected immutable evidence is checked in here. O1/O2 emission and driver/toolchain modes are assigned to the independent verifier; they are not claimed by the O0 result. Integration remains gated on independent review of the final revision and its matrix. This branch does not claim complete Rust language parity or silently turn current Rust target-gap rejections into language rules.
