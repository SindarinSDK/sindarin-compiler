"""Independent C inputs and oracle controls for integer match literals."""

ORIGINALS = [
    ('tests/rgen/errors/statement_match_byte_out_of_range_pattern.sn', 'c25c46cab307ed300e215be7471415f77417b6f1492228bd859c07e6044203e6', 'e849494ac07bbc80fdb3f0898322533eba539afdbdc457ed748710cc1d06da7b', ''),
    ('tests/rgen/errors/statement_match_uint32_out_of_range_pattern.sn', '6f0456c498e0e44b3719c25cd2fbc66c10e112e19d9f0fca78aa57d33b610c90', 'e849494ac07bbc80fdb3f0898322533eba539afdbdc457ed748710cc1d06da7b', ''),
    ('tests/rgen/errors/statement_match_unsigned_subject_negative_pattern.sn', '7dcd0250154a85b7992af25d5ab01e854ee436f3d5cc983dae64b3220759d275', 'b6a3eb51dd51951fead3a52219637cb03716c62d25642b228913ea274cb468e7', ''),
    ('tests/rgen/errors/value_match_int32_out_of_range_pattern.sn', '54907fd549e7498a1fe5efbb10358aac53cafdf478685b194f85d431ebbab82c', '5de14f69f5a2070379af7575eb0adc11e6abe459bd6d40adea815729ca62e430', '300a'),
    ('tests/rgen/errors/value_match_uint_model_boundary_pattern.sn', '052ff29e45b2a57d4355ab16bf069a38aa34993749f64f0617f4caca83f9272b', '5de14f69f5a2070379af7575eb0adc11e6abe459bd6d40adea815729ca62e430', '300a'),
    ('tests/rgen/errors/value_match_unsigned_literal_negative_pattern.sn', '95049c670acd9226690823be84aa4ca8dbf5ac01d3f3a123a16361051df0d38d', '566c94a3616129cd8c40639bcc4b29cb5dfb1127bbdbf8558ea9fb5bfa4467f5', '300a'),
]

CONTROL = """@source "main.sn.c"
native fn foreignMax(): uint
native fn foreignMid(): uint
native fn foreignMax32(): uint32
native fn foreignNegative(): int
native fn foreignMin(): int
native fn foreignNegative32(): int32
native fn foreignZero(): byte
native fn foreignWideLiteral(): int
native fn foreignCountedMax(): uint
native fn foreignCalls(): int
fn matchMax(value: uint): int =>
  return match value =>
    -1 => 1
    else => 0
fn matchMax32(value: uint32): int =>
  return match value =>
    -1 => 0
    else => 1
native fn __sn_integer_match_midpoint_uint_1(): int =>
  return 9
native fn privateAdapter(): int =>
  return __sn_integer_match_midpoint_uint_1()
fn main(): void =>
  println(matchMax(foreignMax())==1)
  println(matchMax32(foreignMax32())==1)
  var signed: int = foreignNegative()
  var result: int = match signed =>
    18446744073709551615u => 1
    else => 0
  println(result==1)
  var narrow: int32 = foreignNegative32()
  var wide: int = match narrow =>
    4294967295 => 1
    else => 0
  println(wide==0)
  var minimum: int = foreignMin()
  var midpoint: int = match minimum =>
    9223372036854775808u => 1
    else => 0
  println(midpoint==1)
  var middle: uint = foreignMid()
  var negativeMidpoint: int = match middle =>
    9223372036854775808u => 1
    else => 0
  println((negativeMidpoint==1)==(foreignWideLiteral()==0))
  var zero: byte = foreignZero()
  var wrapping: int = match zero =>
    256 => 1
    else => 0
  println(wrapping==0)
  var counted: int = match foreignCountedMax() =>
    0, 1 => 0
    -1 => 1
    else => 0
  println(counted==1 && foreignCalls()==1)
  var selected: int = 0
  match foreignCountedMax() =>
    0 => selected=7
    -1 => selected=1
    else => selected=9
  println(selected==1 && foreignCalls()==2)
  var maximum32: uint32 = foreignMax32()
  var noWrap: int = match maximum32 =>
    -1u32 => 7
    4294967296 => 9
    4294967295 => 1
    else => 0
  println(noWrap==1)
  var __sn_integer_match_midpoint_int: int = 4
  var __sn_integer_match_midpoint_uint: int = 5
  var hygiene: int = match foreignMin() =>
    9223372036854775808u => 1
    else => 0
  println(hygiene==1 && __sn_integer_match_midpoint_int + __sn_integer_match_midpoint_uint == 9)
  println(privateAdapter()==9)
"""

BACKING = """#include "sn_types.h"
uint64_t foreignMax(void) { return ~0ULL; }
uint64_t foreignMid(void) { return 9223372036854775808ULL; }
uint32_t foreignMax32(void) { return 4294967295U; }
long long foreignNegative(void) { return -1LL; }
long long foreignMin(void) { return -9223372036854775807LL - 1LL; }
int32_t foreignNegative32(void) { return -1; }
unsigned char foreignZero(void) { return 0; }
long long foreignWideLiteral(void) { return sizeof(-9223372036854775808LL)>8 && -9223372036854775808LL < 0; }
static long long input_calls;
uint64_t foreignCountedMax(void) { input_calls++; return UINT64_MAX; }
long long foreignCalls(void) { return input_calls; }
"""

EXPECTED = b'true\n' * 12
