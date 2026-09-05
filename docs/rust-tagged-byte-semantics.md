# Rust tagged fixed-width arithmetic semantics

## Scope

This branch restores the `v0.0.83` fixed-width arithmetic contract in the Rust backend. The oracle is peeled tag commit `79c20bdb8314aff3c778471ceab20bb8f9ca8d62`; the branch is composed with main `ca485726a9f01321d3eca9361bd2f9cea65f51a8`, including the resolved-call, native scalar, and owned-array closure integrations.

Rust-private lowering now covers:

- byte checked `+`, `-`, and `*` as wrapping byte helpers;
- C-style byte promotion for unchecked binary arithmetic, bitwise operations, shifts, unary `-`, and unary `~`, with one narrowing conversion at the outer byte storage boundary;
- direct byte observation and comparisons without premature narrowing;
- wrapping `int32` checked `+`, `-`, and `*`;
- wrapping `uint32` and `uint` binary arithmetic, unary operations, compound mutation, and postfix mutation;
- stable local unchecked signed compound mutation for tagged-supported, non-overflowing programs;
- mixed integral compound mutation using C integer promotions, followed by one narrowing conversion into the destination type;
- explicit integer types where a checked Rust method would otherwise fail with E0689 on a literal receiver.

The same target-local templates preserve left-to-right, once-only binary operand evaluation. Compound operations evaluate the RHS once before taking one short-lived mutable place borrow. Postfix operations evaluate and borrow the place once. Signed `int` and `long` retain the merged checked diagnostic helpers and messages.

Mixed compound mutation promotes `byte` through the other operand's rank, uses the promoted left type for shifts, wraps unsigned and explicitly unchecked promoted arithmetic, and preserves checked signed diagnostics. The destination place and RHS are each rendered once. The focused regression covers `byte += int`, `int += byte`, and `int32 *= int` without changing the accepted tagged source behavior.

Direct tagged printing represents bytes as uppercase hexadecimal and `uint` through its signed 64-bit bit pattern. The existing `uint_checked_mul_overflow.sn` source remains byte-for-byte identical to main `80c689c2` (Git blob `663aca84188aacbfb78412dd2bda683c31180592`) and produces no output. The separate `tagged_uint_value_print.sn` regression observes the same wrapped calculation as `-4`.

No C generator, shared model, frontend, or tagged oracle source is changed.

## Tagged differential evidence

The final matrix is `/tmp/sn-s2-byte-work/differential-fixed-width-final-20260905/results.tsv`. It contains 43 C/Rust pairs and 86 successful compilations and executions:

- `tagged_byte_promotions_checked.sn`: default, checked, and unchecked, each at O0/O1/O2 (9 pairs);
- `tagged_byte_promotions_unchecked.sn`: unchecked at O0/O1/O2 plus default O2 (4 pairs);
- `tagged_wrapping_integer_values.sn`: checked at O0/O1/O2 (3 pairs);
- `tagged_unsigned_wrapping_unchecked.sn`: default, checked, and unchecked, each at O0/O1/O2 (9 pairs);
- the unchanged, no-output `uint_checked_mul_overflow.sn`: default, checked, and unchecked, each at O0/O1/O2 (9 pairs);
- the separate value-observing `tagged_uint_value_print.sn`: default, checked, and unchecked, each at O0/O1/O2 (9 pairs).

Every source was unchanged between the peeled tag C compiler and this Rust compiler. Every compile and execution returned zero. Runtime stdout and stderr were compared with raw `cmp`; all 43 pairs matched. No diagnostic, path, ANSI, newline, or numeric-output normalization was applied. The unchanged overflow fixture produced zero-byte stdout and stderr in all 18 executions; the separate printed-value fixture produced exactly `-4\n` in all 18 executions.

Twelve representative members of the measured E0689 group were compiled and run through both current C and Rust at O0. All 24 compilations and executions returned zero, all 12 raw stdout/stderr pairs matched, and no Rust stream contained E0689. Results are in `/tmp/sn-s2-byte-work/numeric-inference-representative-80c/results.tsv`.

After the `ca485726` composition, the 22 fixtures named by the fresh main653 rgen failure catalog were compiled individually at O0. Fourteen produced Rust executables and none of the 22 raw compile streams contained E0689 or an ambiguous-numeric-type diagnostic. Eight stopped at existing source/frontend boundaries before rustc; they are retained in `/tmp/sindarin-s2-e0689-main653-catalog-1788647749-3277354/results.tsv` and were not treated as numeric parity successes or changed by this branch.

The earlier byte-only matrices remain at `/tmp/sn-s2-byte-work/differential-20260905b` and `/tmp/sn-s2-byte-work/differential-20260905c`. They contain 18 raw pairs each for the two original byte fixtures across three arithmetic selections and O0/O1/O2.

The mixed integral compound source was accepted independently by the verified tagged compiler in default, checked, and unchecked modes at O0/O1/O2. All nine tagged compilations and executions returned zero and produced exactly `0x07\n7\n48\n`; results are `/tmp/sindarin-s2-tag-mixed-compound-13eO3Z/results.tsv`. The Rust regression passed the same nine-mode matrix with executable checks and raw output comparison at commit `7d34e95ae3c6dc86db673412ca7b4cc37a1416c9`; results are `/tmp/sindarin-s2-rust-mixed-compound-head7d34-1788647151-3212756/results.tsv`.

## Separate oracle evidence

Tagged checked binary byte division and remainder call nonexistent `sn_div_byte` and `sn_mod_byte` helpers. The exact rejected source and streams remain under `/tmp/sn-s2-byte-work/oracle/tagged_byte_checked_div.sn` and `/tmp/sn-s2-byte-work/oracle/results-20260905b`. These forms are not counted as tagged-valid parity.

A compound assignment nested directly in a comparison exposes the tagged C template's missing grouping. The exact source is `/tmp/sn-s2-byte-work/oracle/tagged_byte_compound_context_raw.sn` (SHA-256 `b8bf3c3f4b32bd06ea91c54af0399335b87eb0d67d54d552e5e2920a0457efa9`). Its generated C and streams are under `/tmp/sn-s2-byte-work/oracle/raw-compound-context`; generated C SHA-256 is `4fa2dad56cfae99783bade251fc88e2ae81332debbe48f58d32b18dc76403146`. The committed comparison fixture sequences the result before comparing it.

The tag emits suffixed `uint32` and `uint` literals as signed `long long` C literals. The direct literal-unary probe therefore prints `-1`, while unary mutation of a typed unsigned variable follows the fixed-width unsigned rule. The exact probe and generated C are preserved at `/tmp/sn-s2-byte-work/oracle/numeric-values-20260905/tagged_wrapping_literal_unary.sn` and `tagged_wrapping_literal_unary.generated.c`, with SHA-256 `33be63935f3fbb936b1400ce700acb8a92c40d3c0f30a3cbfd5f9de4e6a8e03b` and `9ba9ae3f6d649e05ddb932a703ed9d9425264ce9334387c733e2300765afd9f1`. Exact literal-expression lowering remains separate work and is not counted by the unsigned-variable claim.

Shift counts from 0 through 31 are the defined byte C-oracle range. Larger counts and division or remainder by zero are excluded from valid tagged behavior evidence.

## Composition and remaining work

The merged diagnostic helper contract remains authoritative. Numeric routing occurs before checked helper rendering only for operations whose tag contract wraps; signed checked operations continue through the collision-free `__sn_checked` family. The literal-receiver type annotation changes only checked expressions whose Rust receiver otherwise has no inherent integer type.

The array text branch is required to execute the unchanged `test_arr_sum_pairs.sn`, `test_fn_collatz_seq.sn`, and `test_fn_powers_of_two.sn` sources. On the numeric branch alone they advance to the independent array-join boundary. The historical `a6a1cfb1`/`a343` evidence remains under `/tmp/sn-s2-byte-work/array-numeric-combined-a6a1-a343`, but is not used as approval for the current head.

A fresh isolated composition used numeric commit `7d34e95ae3c6dc86db673412ca7b4cc37a1416c9` and reviewed array head `8c1cb92072d94957a1a618abc9bb7a85cd0c10b2`. A hello-world control and all three unchanged integration sources compiled and ran through Rust's default optimizer, produced executables, returned zero, and matched their existing expected output byte for byte. Every child status was asserted by a bash harness. Results are `/tmp/sindarin-s2-array-numeric-final-1788647124-3211774/results.tsv`.

After composing main `ca485726`, the same strict check was repeated at numeric candidate `77f0c3f353965a17a2c7ceac283701a601ac0787` with reviewed array head `8c1cb92072d94957a1a618abc9bb7a85cd0c10b2`. The hello-world control and all three unchanged integration sources again compiled, produced executables, ran with status zero, and matched expected output byte for byte. Results are `/tmp/sindarin-s2-array-numeric-main-ca-1788647708-3268794/results.tsv`.

Remaining numeric parity includes the preserved signed-literal C emission nuance, undefined signed C overflow forms outside the valid oracle envelope, and other numeric types not listed in this scope.
