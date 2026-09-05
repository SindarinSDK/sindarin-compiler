# Explicit historical C regression corpus

This directory preserves post-tag C/model/integration/exploratory fixtures outside the authoritative tag runner's discovery paths. It is not an implicit extension of the tagged C acceptance suite.

`relocation.json` records the 677 files moved from the frozen correction checkpoint `2b4717569e52cded9958d5812087af2d1f2ad500`, including all 47 added cgen, 29 mgen and 196 integration/error source fixtures, their companion/support files, original paths and exact SHA-256. The files retain their main (`7af0d192`) bytes. All compatible additions remain useful explicit regressions; all incompatible expectations remain historical evidence, with per-case classifications in `../current-c-failures.json` and `../post-tag-source-inventory.json`.

Two previously relocated `.expected` additions are also preserved with their matching main source snapshots: `tests/cgen/expr_binary_checked` and `tests/exploratory/test_limitation_closure_array`. They accompanied modifications to tagged sources; combining those new expectations with the restored tag sources changed the oracle.

The current-main corpus evidence ran these files in their original paths using exact main fixtures and the main runner, once with the correction and once with the tagged compiler. For faithful replay, use that exact main fixture checkout and original paths; copied model fixtures may contain source-path identity. Do not combine this historical corpus with tagged expectations or modify its bytes to obtain a pass.

The normal C-facing directories now contain exactly the tag's 3,183 files. The separate active Rust runner and its rgen/error/toolchain fixtures remain in place. No tagged fixture/expectation or compiler implementation changes as part of this relocation.
