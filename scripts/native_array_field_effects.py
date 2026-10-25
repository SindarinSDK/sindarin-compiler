"""C-derived copy callback and receiver effects for native array field stores."""
SOURCE = '''@source "main.sn.c"
native struct Fields as ref =>
  data: byte[]
native fn make(kind: int): Fields =>
  if kind==1 =>
    return Fields {data:nil}
  if kind==2 =>
    return Fields {data:{7b}}
  return Fields {data:{1b}}
native fn reset(mode: int): void
native fn observeCopies(values: byte[]): void
native fn observeRelease(values: byte[]): void
native fn chooseIndex(): int
native fn observedTrace(): int
native fn observedCalls(): int
native fn foreignMid(): uint
native fn foreignMidExpected(): int
fn change(values: byte[]): int =>
  values.push(9b)
  values = {8b}
  return values.length
native fn privateCount(): int =>
  var values: byte[] = make(0).data
  return values.length
native fn __sn_integer_match_midpoint_uint(): int =>
  return privateCount()
native fn nativeCounter(): int =>
  return __sn_integer_match_midpoint_uint()
fn sameSlot(): void =>
  reset(0)
  var rows: Fields[] = {make(0)}
  var values: byte[] = {4b}
  observeCopies(values)
  observeRelease(rows[0].data)
  rows[chooseIndex()].data = values
  println(observedTrace()==12322 && observedCalls()==3)
  println(rows[0].data[0]==4b && values[0]==4b)
fn rotatingSlot(): void =>
  reset(1)
  var rows: Fields[] = {make(1),make(1),make(2)}
  var values: byte[] = {4b}
  observeCopies(values)
  var result: byte[] = (rows[chooseIndex()].data = values)
  println(observedTrace()==1222 && observedCalls()==3)
  println(result[0]==7b && rows[1].data[0]==4b && rows[0].data==nil)
fn passedResult(): void =>
  reset(1)
  var rows: Fields[] = {make(1),make(1),make(2)}
  var values: byte[] = {4b}
  observeCopies(values)
  println(change(rows[chooseIndex()].data = values)==1)
  println(observedTrace()==1222 && observedCalls()==3)
  println(rows[1].data.length==1 && rows[2].data.length==2 && rows[2].data[1]==9b)
fn copiedResult(): void =>
  reset(0)
  var rows: Fields[] = {make(0)}
  var values: byte[] = {4b}
  observeCopies(values)
  var result: byte[] = (rows[chooseIndex()].data = values)
  println(observedTrace()==12221 && observedCalls()==3)
  result.push(9b)
  println(result.length==2 && rows[0].data.length==1)
native fn watched(): Fields =>
  var owner: Fields = make(0)
  observeRelease(owner.data)
  return owner
fn consume(values: byte[]): void =>
  println(values.length==1)
fn localRead(): void =>
  reset(0)
  var values: byte[] = watched().data
  println(observedTrace()==0)
  println(values.length==1)
fn callRead(): void =>
  reset(0)
  consume(watched().data)
  println(observedTrace()==0)
fn lazyRead(): void =>
  reset(0)
  if false && watched().data.length==1 =>
    println(false)
  println(observedTrace()==0)
fn loopRead(): void =>
  reset(0)
  var count: int = 0
  while watched().data.length==1 && count<3 =>
    count = count+1
  println(observedTrace()==333 && count==3)
fn main(): void =>
  sameSlot()
  rotatingSlot()
  passedResult()
  copiedResult()
  println(nativeCounter()==1)
  var mid: uint = foreignMid()
  var selected: int = match mid =>
    9223372036854775808u => 1
    else => 0
  println(selected==foreignMidExpected())
  localRead()
  callRead()
  lazyRead()
  loopRead()
'''
BACKING = '''#include "sn_types.h"
#include <string.h>
static long long order, calls;
static int rotating;
static void copied(const void *a, void *b) { order=order*10+1; memcpy(b,a,1); }
void reset(long long mode) { order=0; calls=0; rotating=(int)mode; }
static void released(void *value) { (void)value; order=order*10+3; }
void observeCopies(SnArray *values) { values->elem_copy=copied; }
void observeRelease(SnArray *values) { values->elem_release=released; }
long long chooseIndex(void) { order=order*10+2; calls++; return rotating ? calls-1 : 0; }
long long observedTrace(void) { return order; }
long long observedCalls(void) { return calls; }
uint64_t foreignMid(void) { return UINT64_C(9223372036854775808); }
long long foreignMidExpected(void) { return !(sizeof(-9223372036854775808LL)>8 && -9223372036854775808LL<0); }
'''
EXPECTED = b'true\n' * 17
