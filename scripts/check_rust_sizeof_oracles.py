#!/usr/bin/env python3
"""Check C ABI sizes independently of Sindarin's model and Rust value storage."""
import ctypes as c
import hashlib
import json
import os
from pathlib import Path
import sys

class Dense(c.Structure):
    _fields_ = [("first", c.c_char), ("value", c.c_int32), ("flag", c.c_bool), ("last", c.c_double)]
class Ordered(c.Structure):
    _fields_ = [("last", c.c_double), ("value", c.c_int32), ("first", c.c_char), ("flag", c.c_bool)]
class Nested(c.Structure):
    _fields_ = [("first", c.c_uint8), ("inner", Dense), ("last", c.c_uint8)]
class Point(c.Structure):
    _fields_ = [("value", c.c_int64), ("character", c.c_char)]
class Handles(c.Structure):
    _fields_ = [("first", c.c_char), ("text", c.c_void_p), ("values", c.c_void_p), ("point", Point), ("last", c.c_uint8)]
class Packed(c.Structure):
    _pack_ = 1
    _fields_ = [("first", c.c_char), ("value", c.c_int64), ("last", c.c_bool)]
class Natural(c.Structure):
    _fields_ = Packed._fields_
class Inner(c.Structure):
    _fields_ = [("packed", Packed), ("reference", c.c_void_p)]
class Outer(c.Structure):
    _fields_ = [("first", c.c_uint8), ("inner", Inner), ("last", c.c_uint8)]

def sizes(*types):
    return "".join(f"{c.sizeof(t)}\n" for t in types).encode("ascii")

ORACLES = {
    'tests/integration/test_struct_stack_alloc.sn': b'Testing stack-allocated structs (<8KB):\nSmallPoint test: PASS\nSmallConfig defaults test: PASS\nMediumStruct test: PASS\nAll stack allocation tests passed.\n',
    'tests/integration/test_struct_heap_alloc.sn': b'Testing heap-allocated structs (>=8KB):\nsizeof(LargeStruct) = 8192 (at 8KB threshold)\nSize test: PASS\nAllocation test: PASS\nField access test: PASS\nModification test: PASS\nAll heap allocation tests passed.\n',
    'tests/integration/test_struct_sizeof_equality.sn': b'=== Testing sizeof operator ===\nsizeof(int) = 8\nsizeof(long) = 8\nsizeof(double) = 8\nsizeof(float) = 4\nsizeof(byte) = 1\nsizeof(bool) = 1\nsizeof(char) = 1\nsizeof(int32) = 4\n\nsizeof(Point) = 16\nsizeof(Rectangle) = 32\nsizeof(SmallStruct) = 8\n\nsizeof p = 16\nsizeof r = 32\nsizeof s = 8\nsizeof(p) = 16\nsizeof(r) = 32\n\nsizeof(Point) + sizeof(Rectangle) = 48\nsizeof(Point) > 8\n\n=== Testing struct equality ===\np1 == p2: true (expected)\np1 == p3: false (expected)\np1 != p3: true (expected)\np1 != p2: false (expected)\n\nr1 == r2: true (expected)\nr1 != r3: true (expected)\n\nAll tests completed.\n',
    'tests/integration/test_sizeof.sn': b'=== Comprehensive sizeof Tests ===\n\ntest_primitive_types: PASS\ntest_struct_types: PASS\ntest_primitive_variables: PASS\ntest_struct_variables: PASS\ntest_without_parentheses: PASS\ntest_in_expressions: PASS\ntest_arrays: PASS\ntest_2d_arrays: PASS\ntest_sizeof_in_normal_function: PASS\ntest_sizeof_in_native: PASS\n\nAll sizeof tests passed!\n',
    'tests/rgen/sizeof_c_unevaluated.sn': b'16\n16\n0\n',
    'tests/rgen/sizeof_c_member_unevaluated.sn': b'1\n0\n',
    'tests/rgen/sizeof_c_hygiene.sn': b'1\n8\n1\n9\n',
    'tests/rust-native/scalar_local_aggregate.sn': b'24\n15\n',
    'tests/rust-native/scalar_local_aggregate_results.sn': b'abc\n0304\n',
    'tests/rust-native/scalar_operator_local_aggregate.sn': b'true\n',
    'tests/rgen/sizeof_struct.sn': b'',
    'tests/rgen/sizeof_fixed_scalars.sn': b'true true true true\n',
    'tests/rgen/sizeof_managed_handles.sn': b'true true true\n',
    'tests/rgen/sizeof_pointer_values.sn': b'8\ntrue\ntrue\ntrue\ntrue\n',
    'tests/rgen/sizeof-promoted/import_pure_unsupported_native_struct.sn': b'0\n',
    'tests/rgen/resolved_operator_source_child_precedence.sn': b'true\n',
    'tests/integration/test_struct_as_ref_chain.sn': b'v1.add(v2): (4, 6)\nv1.add(v2).scale(2): (8, 12)\nv1.add(v2).scale(3).negate(): (-12, -18)\nv1.add(v2).add(v1).add(v2): (8, 12)\nv1.negate().add(v2).scale(2): (4, 4)\n',
    'tests/integration/test_struct_codegen_validation.sn': b'=== Struct Code Generation Validation ===\n\n--- Testing Basic Field Types ---\nfield_int: PASS\nfield_int32: PASS\nfield_double: PASS\nfield_char: PASS\nfield_bool: PASS\nfield_byte: PASS\n\n--- Testing Nested Structs ---\nnested point.x: PASS\nnested point.y: PASS\nnested value: PASS\n\n--- Testing Packed Struct Size ---\nPackedData sizeof=10: PASS (expected 10)\n\n--- Testing Unpacked Struct Alignment ---\nAfterPacked sizeof=24: checking alignment\nAfterPacked alignment: PASS (size >= 17)\n\n--- C Type Sizes ---\nint: 8 bytes\nint32: 4 bytes\ndouble: 8 bytes\nfloat: 4 bytes\nchar: 1 bytes\nbool: 1 bytes\nbyte: 1 bytes\nsizeof(int) == 8: PASS\nsizeof(int32) == 4: PASS\nsizeof(double) == 8: PASS\nsizeof(float) == 4: PASS\n\n=== All Struct Code Generation Tests Completed ===\n',
    'tests/integration/test_struct_native_defaults.sn': b'Test 1 - Native zero init passed\nTest 2 - Native partial init passed\nTest 3 - Native full override passed\nTest 4 - Buffer (all required) passed\nTest 5 - StreamConfig defaults passed\nTest 6 - StreamConfig partial override passed\nTest 7 - StreamConfig full override passed\nAll native struct default tests passed!\n',
    'tests/integration/test_struct_packed.sn': b'Packed struct declarations compile successfully\nNative struct: size=0, capacity=1024\nAfter modification: size=100\nNative struct with pointer fields works!\n',
    'tests/rgen/sizeof_c_padding.sn': sizes(Dense, Ordered, Nested),
    'tests/rgen/sizeof_c_handles.sn': sizes(Handles, c.c_void_p),
    'tests/rgen/sizeof_c_packed_reference_native.sn': sizes(Packed, c.c_void_p, Natural),
    'tests/rgen/sizeof_c_nested_layout.sn': sizes(Packed, Inner, Outer),
}


def verify(path):
    report = json.loads(path.read_text())
    required = {(source, optimization, mode) for source in ORACLES
                for optimization in ("-O0", "-O1", "-O2")
                for mode in ("default", "checked", "unchecked")}
    if not report["passed"] or len(report["cases"]) != len(required):
        raise ValueError("expected 216 successful sizeof differential cases")
    seen = set()
    for case in report["cases"]:
        source = case["source"].replace("\\", "/")
        identity = (source, case["optimization"], case["arithmetic_mode"])
        if not case["passed"] or identity in seen or identity not in required:
            raise ValueError(f"unexpected or duplicate case: {identity}")
        seen.add(identity)
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() != case["source_sha256"]:
            raise ValueError(f"source changed: {source}")
        expected = ORACLES[source]
        if Path(source).with_suffix(".expected").read_bytes() != expected:
            raise ValueError(f"stale fixture oracle: {source}")
        if os.name == "nt":
            expected = expected.replace(b"\n", b"\r\n")
        if set(case["targets"]) != {"c", "rust"}:
            raise ValueError(f"missing target: {identity}")
        for target in ("c", "rust"):
            result = case["targets"][target]
            if result["compile"]["status"] != 0 or result["run"]["status"] != 0:
                raise ValueError(f"unsuccessful execution: {identity} {target}")
            if result["run"]["stdout_hex"] != expected.hex() or result["run"]["stderr_hex"]:
                raise ValueError(f"independent output mismatch: {identity} {target}")
    if seen != required:
        raise ValueError("incomplete optimization/arithmetic coverage")
    report["independent_oracle_cases"] = len(seen)
    report["oracle_scope"] = (
        "Eight unchanged originals; ctypes C layouts for padding, nesting, "
        "packed values, reference and managed handles; operand non-evaluation; "
        "helper-name collisions; C-local aggregate operators and owned results. "
        "Public native aggregate ABI and runtime packed/reference values are excluded."
    )
    path.write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 216 independent sizeof C/Rust oracles, all modes and optimizations")


if __name__ == "__main__":
    try:
        verify(Path(sys.argv[1]))
    except (KeyError, ValueError, OSError, IndexError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
