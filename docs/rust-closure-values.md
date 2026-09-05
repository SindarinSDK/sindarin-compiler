# Rust immutable callable foundation

This branch delivers an immutable callable foundation, not full closure parity.
The original implementation base was PR114 head
`bc9c648ab55714c4648ac6de6ac059a62f7a127c`. Before final gates it was rebased
onto the exact PR114 merge, `8e142cea3dd901e2974f4d1109329c0366d96770`.
The format correction subsequently rebased PR117 onto PR116 merge
`f0564920ddd276ea2ee3f352c644bca1064b7561`, bringing in the synchronized
closure-array test. The parent branches and shared parity ledger were not edited.

## Implemented envelope

Internal captureless lambdas and named free functions become cloneable owned
callables. Function values support local declaration, copying and reassignment,
parameters, returns, ordinary value-struct fields, and normal arrays including
array copies, indexed reads/writes and independently owned foreach bindings.
Existing surrounding ownership/type restrictions still apply. Default parameters
can contain supported scalars, strings, arrays, function handles and heap-free
value structs; heap-owning struct parameter ABI and parameter mutation are not
extended by this work. Results can contain supported owned values and further
function handles. Function signatures use exact recursive type compatibility;
the shared checker's float/double compatibility does not imply a Rust Fn ABI
conversion.

Immutable snapshots cover all ten existing scalar value kinds together (`int`,
`long`, `int32`, `byte`, `uint32`, `uint`, `float`, `double`, `bool`, `char`),
owned strings, normal arrays, auto-copy plain value structs, and function handles.
Outer reassignment after creation does not change snapshots. Escaping snapshots,
sibling snapshots, independent factory invocations, lexical shadowing, foreach
captures, and two/three-level transitive captures are compiled and executed.
The existing source-char value representation is reused; this does not settle
`char as ref` or its ABI/layout decision.

A handle is `__SnClosure<dyn Fn(Args...) -> Result>` backed by `Rc`. Rust's
arity-specific Fn traits provide the per-signature callable interface directly,
so no parallel generated public trait registry or shared signature-ID model is
needed. A manual Clone implementation clones the Rc identity, never the captured
environment. The closure owns each scalar/owned snapshot; String/Vec/value-struct
Clone copies their values, while function Clone retains the same callable.
Derived struct Debug/PartialEq requirements are satisfied by handle Debug and
pointer-identity equality. The default support name is `__SnClosure`; lowering chooses a suffixed name
absent from every model string when that spelling collides. No user function or
struct name is reserved, including in modules that emit no closure support.
Every nested function type and constructor uses the chosen name; constructor
references are also module-qualified.

Explicit capture-list tuple initialization copies every snapshot before the
`move` Fn is created. A target-local lexical walk gives parameters, declarations,
foreach bindings, captures and variable occurrences binding IDs. Each nested
capture resolves against the current lambda's binding scope; a missing
transitive capture is rejected rather than left to Rust's implicit capture
inference. The inline lambda model is authoritative; the duplicate top-level C
lambda list is not separately rendered or independently reinterpreted.

Closure invocation evaluates an owned callee handle before arguments, then each
argument once in source order. The function-array access branch binds its array
and index together, before introducing private temporary names, and clones the
selected handle once. It does not repeat a side-effecting index. There is no
RefCell, callable-wide mutable borrow, fabricated static reference, self-cycle,
unsafe code, or manual Rust capture cleanup in this foundation.

## Required next work

None of these are accepted parity exceptions:

1. Shared mutable scalar captures as one family: outer/sibling visibility,
   escaping cells, binding-wide promotion, nested/transitive identity, compound
   and postfix operations, and callbacks during RHS evaluation. Use separate
   short-lived cells; never hold a callable or cell borrow across invocation.
2. Recursive lambda self and recursive/reentrant invocation through copied
   aliases/callbacks. Use weak self identity, without a strong self-cycle.
   Existing defined C scalar capture and recursive tests anchor this next slice
   independently of the unrelated C call/temporary defects below. Named recursive
   free functions used as values already execute in the foundation.
3. Mutable owned captures, including C's privately mutated array snapshot;
   captured receiver methods; broader parameter mutation and owned-struct ABI;
   consumed function field/index assignment results; uninitialized/sized
   function storage and other surrounding unsupported ownership forms.
4. Borrowed/as-ref captures and qualified signatures need a lifetime-aware
   ownership design. Reference structs, custom copy semantics, native/foreign
   callbacks and threads remain required dependency-ordered parity work.

C's `prescan_needs_ref` currently lists int/long/double/bool/byte/char/array but
omits int32/uint32/uint/float. The next scalar slice must establish each kind's
actual defined C behavior rather than blindly apply the older promotion list.
This does not make scalar/recursive work depend on the unrelated C fixes below.

Validation retains the global gate first, then closure lexical/signature/capture
checks, then existing thread/type/pragma/struct/function gates and recursive body
validation. Shared frontend errors still precede target checks. Qualified/native
signatures, borrowed/promoted captures, mutable snapshots, recursive self,
missing captures, incompatible signatures and unsupported body forms fail
closed. The comparison script checks the exact first target error and absence
of emitted Rust at O0/O1/O2. Two obsolete whole-closure negatives were moved to
compiled/run positives (`closure_values_foreach`, `closure_values_match_string`);
three neighboring negatives now pin their newly reachable precise diagnostic.

## Production boundaries

Production changes are confined to closure validation/lowering/render fragments,
closure support and lambda/read/write/call partials. Necessary common glue is
seven lines in `rust_validate.c` (closure gate and function copy admission), plus
the function-element branch of `expr/array_access.hbs`.
`rust_validate_calls.c`, `rust_lower_calls.c`, and
`direct_call/static_call/method_call/borrow_inferred_call` partials are untouched.
Parser, checker, shared model, C production/runtime and default-target selection
are untouched. No agent, PR opening/merging, provider change or host-package
installation outside `make setup` was performed.

## C evidence and preserved defects

The model/runtime audit was independently checked against
`src/cgen/gen_model_{expr,func,stmt}.c`, `src/runtime/sn_core.h` retain/release and
array-element callbacks, and C lambda/read/assign/call/index templates.
Three original C behavioral sources are reused byte-for-byte as Rust positives:
`test_lambda_no_params`, `test_lambda_capture_fn_with_captures`, and exploratory
`test_lambda_no_capture`. The existing outlives-scope, recursive, foreach function
array, nested/transitive and mutable-array cases remain evidence for the next
required slices; their unsupported ref/mutable forms are not claimed here.

Exact C probes are preserved under [tests/rgen/closure_probes](../tests/rgen/closure_probes).
They are intentionally separate from normalized passing fixtures and are not
registered as passing integration cases. Generate them from repository root:

```sh
bin/sn tests/rgen/closure_probes/c_index_twice.sn --emit-c -O0 --no-install -l 1 -o /tmp/c_index_twice.c
bin/sn tests/rgen/closure_probes/c_index_twice.sn -g -O0 --no-install -l 1 -o /tmp/c_index_twice
/tmp/c_index_twice
```

All nine probes were emitted and built at O0 with `-g` on Spark2. Exact generated
C, stdout and stderr are retained in `/tmp/rust-closure-c-probes/`, named after
the source stem. Line references below are to those standalone `.c` files.

| Probe | Generated C location and defect | Observed execution |
|---|---|---|
| `c_named_declaration` | Line 23 stores `__sn__increment` directly in an owning closure variable; subsequent call dereferences it as a closure header | Sanitizer exit 1 |
| `c_copy_declaration` | Line 29 copies `__sn__first` without retaining; replacement releases the shared allocation | Sanitizer exit 1 |
| `c_parameter_return` | Line 24 returns the borrowed parameter without retaining; line 35 acquires a competing owning local | Sanitizer exit 1 |
| `c_borrowed_field` | Line 79 stores the borrowed function in an owning field without retaining | Prints 7, sanitizer exit 1 at cleanup |
| `c_borrowed_field_assign` | Line 91 replaces the field with the borrowed function without retaining | Prints 7, sanitizer exit 1 at cleanup |
| `c_borrowed_index_assign` | Line 46 overwrites the array slot without retain or release | Prints 7, sanitizer exit 1 at cleanup |
| `c_index_replace_leak` | Line 38 overwrites the old owning slot without release | Isolated probe exited 0 on this run; the combined fixture detected a 32-byte leak from the old named-function wrapper |
| `c_index_twice` | Line 45 renders `__sn__index()` in both callable lookup and environment argument | Prints `index` twice, then 7, exit 0 |
| `c_temporary_field` | Line 91 constructs an auto-cleaned `__mtmp_0__` for each extraction from the returned holder; the function is released before invocation | Sanitizer exit 1 |

The original combined leak report is `/tmp/closure-asan.err` from
`.sn/build/c/test_closure_values_foundation_1373850/main.c`: allocation line 201,
slot overwrite line 209. Its original output matched, but LeakSanitizer failed.
Earlier borrowed-field/index versions likewise matched stdout before ASAN failed.
Those runs are failures, not parity passes. The final normalized foundation uses
fresh function values for field ownership transfers and replaces the whole array
instead of overwriting an owning C slot. Rust's direct-ownership positive retains
the original borrowed declaration/return/field/index edges and tests correct
surviving aliases.

`closure_values_order.sn` retains the side-effecting index and returned-temporary
field access as a Rust-only behavioral oracle. `closure_values_order_parity.sn`
binds the index, returned array and holder, and each side-effecting call
argument to explicit locals before calling. Directly passing two side-effecting
arguments cannot be a portable C comparison: C leaves their evaluation order unspecified,
GCC evaluates this fixture right-to-left, and Clang happened to evaluate it
left-to-right. The two sources are deliberately distinct. No byte-identical C
parity is claimed for the failing originals. All compiler findings are queued C
repairs, not language exclusions; unspecified C argument order is documented as
a comparison limitation rather than treated as evidence of Sindarin semantics.

## Verification

`make setup` ran in this fresh worktree before the first build/test; prerequisites
reported zero host packages installed/upgraded/removed. The branch was rebased
onto exact merged PR114 before final gates. Harnesses use their default **20
workers**, one invocation at a time. The additional comparison script likewise
runs one compiler/program at a time, without changing harness configuration.

The final command sequence passed. The focused comparison script is checked
in for reproducibility:

```sh
python3 tests/rgen/closure_values_compare.py
python3 scripts/run_tests.py rgen --filter closure_values --verbose
python3 scripts/run_tests.py rgen-errors --filter closure_values --verbose
python3 scripts/run_tests.py integration --filter closure_values --verbose
make test-rgen
make build && make test
bin/sn --format --check
```

The corrected comparison covers 20 sources at O0/O1/O2: sixteen run as Rust,
explicit C and default C; four C-defect families run as Rust only. That is **156
compiled executions** (96 C/default-C and 60 Rust), plus **57 ordered rejection
checks** over 19 negatives, each requiring no emitted Rust artifact. Eleven shared
source/output pairs additionally run in the sanitizer-enabled C integration
harness. No unrelated golden is regenerated, and no failure is disabled.

Initial foundation focused results were Rust positives **11/11**, Rust negatives
**15/15**, and sanitized C **4/4**, with `make test-rgen` **242/242**.
The initial subsequent literal `make build && make test` passed on its first run:

| Suite | Passed |
|---|---:|
| Unit | 1634 |
| C generation | 154 |
| Rust generation | 242 |
| Rust generation errors | 179 |
| Shared model generation | 108 |
| Default-C integration | 1300 |
| Integration errors | 76 |
| Exploratory | 224 |
| Exploratory errors | 11 |
| Rust toolchain/lifecycle | 7 |
| Total | **3935** |

All initial final suites had **zero failures and zero skips**. `git diff --check` and the
protected-file diff checks passed. The 231 pre-existing Rust positive snapshots
remain unchanged. Initial development failures and C sanitizer discoveries are
not counted as successful gates. Independent feature review and cross-platform
CI remain integration obligations; these results are from Spark2 only.

The initial foundation commit bypassed the pre-commit formatter. That was an
error: CI requires `bin/sn --format --check`, and Ubuntu/macOS CI identified 13
new sources needing formatting. The correction applies the required formatter
and retains the normal pre-commit hook. Changes are struct-literal whitespace/
layout and spacing before indexed callable invocation. All nine raw C probes
emit byte-identical C compared with the original archived files, preserving
their semantics and the generated-source line references above. Existing Rust
snapshots and expected outputs are retained; the correction reruns focused
comparisons, Rust generation, the full suite, and the final format check.

The formatting correction requires no golden regeneration: all existing Rust
snapshots still match. Its final gates run on the PR116 merge base above with
the normal 20-worker harnesses, sequentially, including the format check.

## Independent review corrections

The source review identified an unconditional `__SnClosure` reservation,
computed/shadowed callees misclassified by the shared flags, and bare named
function equality allocating two different wrappers. All are corrected in the
closure-owned production files. Four helper-name fixtures compile/run plain
functions and structs with no closure support, plus actual closure programs
colliding with the base helper name and its first two suffix candidates.

The lexical call-site walk now decides variable callees from its active binding
scope before setting `rust_direct_callee` and the target-local `rust_closure_call`
marker. Computed function-typed callees also receive the indirect marker; the
closure validator consumes that same decision. Ordinary member methods remain
with the call author. Tests cover local, parameter, nested-block, normal-array
loop, and match-arm references to shadowing function bindings, including scope
exit restoration. Match-arm declaration prefixes and function-valued match
results retain their existing precise rejections; the latter is pinned by
`value_match_function_result` and cannot reach callable emission unchecked.

Bare comparisons of two resolved named free functions compare symbol identity
without allocating wrappers. This is deliberately not global interning. The
O0/O1/O2 C/Rust identity matrix proves `same == same`, `same != different`,
distinct names with identical bodies, separate assignments from the same named
function (unequal boxes), copied handles (equal boxes), mixed bare-name/box
comparison, and copied lambda identity.

Two additional exact failing C sources are preserved as
`closure_probes/c_computed_calls.sn` and `c_shadow_calls.sn`. Their standalone
C is archived in `/tmp/rust-closure-c-probes/`. The former emits `__sn__(...)`
at lines 54/56 for returned-function and immediately invoked lambda calls.
The latter emits direct `__sn__action(...)` calls on a `void *` parameter at
line 41 and on shadowing locals/loop bindings at lines 55/68/71/100/120/122/139.
Both fail C compilation. The Rust originals are runtime fixtures but are not
counted as C passes. `closure_values_computed_calls_parity` supplies the
separate passing C/Rust source with explicit intermediate callable locals.

The original nine probe outputs are byte-identical C after formatting, and
no pre-existing Rust golden required regeneration for these corrections.

Callable relational ordering (`<`, `<=`, `>`, `>=`) is explicitly rejected with
`Rust target does not support ordered function-value comparisons yet`. Four
negative fixtures cover all operators with bare symbols and boxed handles at
O0/O1/O2. Equality support does not imply an address-based ordering contract;
portable ordering semantics remain unresolved required parity work.

`closure_values_self_field` additionally compiles and runs a method invoking
`self.action`, including a copied struct whose original function field is
replaced. The foundation fixture already covers ordinary caller array cloning,
and the identity matrix explicitly verifies copied handles.

## Corrected final gates (Spark2, 2026-09-05)

On base `f0564920ddd276ea2ee3f352c644bca1064b7561`, the final focused
commands above passed: 20 Rust runtime fixtures, 19 rejection fixtures,
11 sanitized default-C integration fixtures, and the comparison script's
156 compiled executions plus 57 ordered rejections. `make test-rgen` passed
251/251. The subsequent literal `make build && make test` passed:

| Suite | Passed |
|---|---:|
| Unit | 1634 |
| C generation | 154 |
| Rust generation | 251 |
| Rust generation errors | 183 |
| Shared model generation | 108 |
| Default-C integration | 1307 |
| Integration errors | 76 |
| Exploratory | 224 |
| Exploratory errors | 11 |
| Rust toolchain/lifecycle | 7 |
| Total | **3955** |

Every suite had zero failures and zero skips, using default 20 workers and
sequential harness invocations. The final `bin/sn --format --check`,
`git diff --check`, and protected call-file diff checks passed. No existing
Rust golden changed; only the new review-regression goldens were generated.
The normal `.githooks/pre-commit` formatter remains enabled.

Mutable-worker handoff: the original foundation boundary was
`1dd17f181ec0ee056caf06c62a30829756911082`. Its rebased equivalent before these
corrections is `c6669920fd6f242543a04cd843157e4ca2413c2c`; stack only the mutable
worker's own changes onto the final correction commit reported with PR117.
Shared mutable captures, recursive self/reentrancy, remaining owned/borrowed
capture families, foreign callbacks and threads remain required later slices.
This PR continues to claim only the documented immutable callable foundation.

## Shared scalar and recursive follow-on

The mutation commit was replayed alone onto final PR117 fixture-fix head
`5e7a01e2657d6775904fbd2a85bd6a0f9db495d2` in the dedicated
`feature/rust-mutable-closures` worktree. Conflict resolution preserves PR117's
collision-free handle name, bare-symbol versus wrapper identity, computed and
shadowed callee classification, ordered-comparison diagnostic, and comparison
fixtures. The clean starting tree and successful worktree-local `make setup`
were confirmed before the first build.

The source implementation retains immutable `Rc<dyn Fn>` callable identity.
Promoted scalar declarations own `Rc<Cell<T>>`; capture aliases clone the cell
identity and outer/sibling reads and writes use `get`/`set`. No mutable borrow
survives an RHS evaluation or invocation. Compound updates evaluate the RHS
before reading and storing the resulting cell value. Unsigned unchecked cell
updates use wrapping arithmetic; signed overflow has no defined C parity oracle.
Recursive self captures own a shared initialization slot containing only a
`Weak<dyn Fn>`; reading self upgrades to a temporary strong callable handle.
Nested closures capturing self acquire a normal owning handle at creation.

C's non-reference scalar captures are copied into invocation locals, so mutable
snapshots must reset on every invocation. The four kinds omitted from C's
promotion list (`int32`, `uint32`, `uint`, `float`) retain that behavior. They are
included in the new ten-kind fixture, not excluded from this scalar slice.
Owned mutable and borrowed captures remain immediate subsequent required slices.

New `closure_values_mutable_*` sources cover outer/sibling visibility, independent
escaping factories, shadowed binding identity, transitive shared cells, callback
recursion, nested closures capturing recursive self, reentrant assignment and
compound RHS evaluation, and all ten scalar kinds. Identical C integration
sources cover five mutable families with sanitizers. Three former scalar/self
negatives are now compiled positives with Rust generation snapshots.

`closure_values_mutable_alias` additionally exercises a copied recursive handle
surviving replacement of its original binding, with reentrant compound RHS
updates. It is Rust-only because it deliberately retains C's known
unretained copy-declaration/replacement defect. Its byte-identical C source is
preserved as `closure_probes/c_mutable_recursive_alias.sn` and is never counted
as a differential pass. An O0 debug/ASAN run exits 1 with a heap-use-after-free:
replacement releases the allocation still referenced by the unretained alias,
and the later alias call reads the freed closure header. This remains an exact C
regression rather than a Rust parity exclusion. The weak construction helper is
an associated constructor, so its initialization temporaries are never introduced
into the source lambda's lexical capture scope.

## Mutable follow-on verification (Spark2, 2026-09-05)

The focused comparison covers 29 sources at O0/O1/O2. Twenty-four run as Rust,
explicit C, and default C; five known C-defect families run as Rust only. That is
231 compiled executions: 87 Rust and 144 C/default-C. Sixteen remaining closure
negatives produce 48 exact ordered rejections with no emitted Rust artifact.
Focused Rust generation passes 29/29 with zero skips after adding nine new
snapshots; focused sanitized integration passes 16/16.

`make test-rgen` passes 260/260. The subsequent literal
`make build && make test` passes all 3,966 tests with default 20-worker harnesses:

| Suite | Passed |
|---|---:|
| Unit | 1634 |
| C generation | 154 |
| Rust generation | 260 |
| Rust generation errors | 180 |
| Shared model generation | 108 |
| Default-C integration | 1312 |
| Integration errors | 76 |
| Exploratory | 224 |
| Exploratory errors | 11 |
| Rust toolchain/lifecycle | 7 |
| Total | **3966** |

Every reported suite has zero failures and zero skips. Final formatting and
protected-file checks passed after the documentation and checkpoint commit.
Mutable owned captures and borrowed/as-ref captures remain
the immediate subsequent slices; this work does not depend on repairing the
separately preserved C callable ownership failures.

## Mutable snapshot arithmetic review correction

Snapshot mutation now follows both the selected Rust validation arithmetic mode
and the model's per-expression `mutation_arithmetic_mode`. Checked O0/O1 output
continues to use `checked_*` methods and preserves the existing arithmetic-failure
behavior. Unchecked `uint32` and `uint` snapshots, including the default O2
model, use Rust wrapping methods for `+`, `-`, `*`, `/`, `%`, postfix increment,
and postfix decrement. An explicit `--checked` or `--unchecked` override takes
precedence at every optimization level. Each invocation still reconstructs its
private snapshot from the captured value, so a second call repeats the same
boundary result rather than observing the first call's mutation.

The unsigned boundary fixture exercises discarded compound results as well as
observable postfix results for both widths. Its O0/O1/O2 matrix contains nine
unchecked C/Rust/default executions and nine corresponding checked failures
across those targets. Two additional Rust fixtures verify
checked division and remainder by zero for both widths at O0/O1/O2. The exact C
sources are retained separately as
`closure_probes/c_snapshot_unsigned_zero_division.sn` and
`closure_probes/c_snapshot_unsigned_zero_modulo.sn`: their generated C contains
native division or remainder by a captured zero. The observed executables exited
0 without output under optimization, which is undefined C behavior and therefore
is not recorded as a parity pass or used as a Rust oracle.

Floating snapshot compound assignment now validates the operator before
rendering. `+=`, `-=`, `*=`, and `/=` retain their existing behavior; `%=` is
rejected at O0/O1/O2 with the established Rust-target floating-compound
diagnostic and no emitted artifact. The corresponding C source also fails C
code generation separately, so it is not treated as a differential success.

After this correction the focused comparison passes 240 compiled executions,
15 expected checked failures, and 51 exact ordered rejections with no emitted
Rust artifact. The isolated focused harnesses pass 32/32 Rust generation,
17/17 Rust generation errors, and 17/17 integration tests. Each harness uses a
private `TMPDIR`; direct harness invocations retain `--no-cleanup` so concurrent
worktrees cannot delete one another's temporary files.

The production-change gates pass `make test-rgen` at 263/263 and the subsequent
`make build && make test` at 3,971/3,971 with zero failures and zero skips:

| Suite | Passed |
|---|---:|
| Unit | 1634 |
| C generation | 154 |
| Rust generation | 263 |
| Rust generation errors | 181 |
| Shared model generation | 108 |
| Default-C integration | 1313 |
| Integration errors | 76 |
| Exploratory | 224 |
| Exploratory errors | 11 |
| Rust toolchain/lifecycle | 7 |
| Total | **3971** |

## Owned string capture follow-on

Mutable owned string captures follow the documented default reference-capture
semantics in v0.0.83 `docs/lambdas.md`: mutation through a closure is visible to
the outer binding and to sibling closures. Rust promotes a captured owned string
to `Rc<RefCell<String>>`; closure construction clones only the `Rc`, and every
read clones the current string through a short-lived immutable borrow. This keeps
the callable itself `Fn`, supports escaping and transitive captures, and preserves
binding identity across shadowed names.

Assignment evaluates its right-hand side before replacing the cell value. String
append likewise evaluates and owns the complete right-hand side before taking a
short-lived mutable borrow. The reentrant fixture invokes the same recursive
callable through a callback during RHS evaluation, after which the caller borrows
the cell and commits its append. Additional Rust regression fixtures cover
reassignment, append, captured string methods, outer and sibling visibility,
binding shadowing, two-level transitive capture, captured function parameters,
independent factories, escaping closures, and evaluate-once callbacks at
O0/O1/O2. The original `closure_values_owned_mutation.sn` source is unchanged and
now serves as a Rust runtime fixture.

The C backend, runtime, templates, shared frontend, and existing C expectations
are unchanged. A detached build of the authoritative v0.0.83 source at
`79c20bdb8314aff3c778471ceab20bb8f9ca8d62` passes its existing read-only,
escaping string-capture integration case on repeated calls. Separate raw mutable
reassignment and append probes compile at the tag but abort at O0/O1/O2; the
minimal reassignment prints `two` before ASAN reports a double free. These failing
executions establish no expected language result and are never counted as parity
passes. Their byte-identical sources remain under `closure_probes/` as diagnostic
evidence. The tag build also aborts on the separate by-value string parameter
mutation probe, which remains outside this capture slice.

Calling a string method on a closure parameter is a defined C-supported form but
is still rejected by the Rust target's existing closure-parameter boundary. Its
O0/O1/O2 C output and exact Rust rejection are pinned in
`closure_values_owned_string_parameter_method`. Mutable closure parameters are
also retained as an explicit rejection and raw C probe. Owned arrays, structs,
function captures, receiver capture, and borrowed/as-ref capture families remain
required subsequent work.

Fresh-worktree dependency setup used only the existing `~/.sn/bin/sn --install`;
no setup or host installer ran. The fresh worktree also exposed that PR119's
unsigned-boundary integration `.opt` file existed locally but was omitted from
commit `4bed011` because integration fixtures are ignored by default. This branch
tracks that exact `-O2` metadata so the already-committed boundary source runs in
its intended unchecked mode; its source and expectation are unchanged.

Focused verification passes 37/37 Rust generation fixtures, 18/18 closure
rejections, and the unchanged 17/17 integration controls. The O0/O1/O2
comparison passes 255 compiled executions, 15 expected checked failures, and 54
exact ordered rejections with no emitted Rust artifact. `make test-rgen` passes
268/268. The subsequent isolated `make build && make test` passes 3,977/3,977
with zero failures and zero skips:

| Suite | Passed |
|---|---:|
| Unit | 1634 |
| C generation | 154 |
| Rust generation | 268 |
| Rust generation errors | 182 |
| Shared model generation | 108 |
| Default-C integration | 1313 |
| Integration errors | 76 |
| Exploratory | 224 |
| Exploratory errors | 11 |
| Rust toolchain/lifecycle | 7 |
| Total | **3977** |
