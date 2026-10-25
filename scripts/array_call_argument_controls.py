"""Temporary slice loans and an independent reference for selected C call order."""
SOURCE = r'''fn length(values: int[]): int =>
  return values.length
fn returned(values: int[]): int[] =>
  return values
fn discard(values: int[], result: int as ref): void =>
  result = values.length
fn mixed(values: int[], text: str): int =>
  return values.length + text.length
native fn nativeLength(values: int[]): int =>
  return values.length
native fn nativeNested(values: int[]): int[] =>
  return returned(values[0..2])
struct Sink =>
  seen: int
  fn take(values: int[]): int =>
    self.seen = values.length
    return self.seen
  static fn count(values: int[]): int =>
    return values.length
native fn __chain_tmp_0(): int =>
  return 11
native fn __chain_tmp_1(): int =>
  return 22
fn main(): void =>
  var original: int[] = {1,2,3,4}
  var sink: Sink = {seen:0}
  println(length(original[1..3])==2)
  println(Sink.count(original[0..3])==3)
  println(sink.take(original[1..4])==3 && sink.seen==3)
  var callback: fn(int[]): int = fn(values: int[]): int =>
    return values.length
  println(callback(original[0..2])==2)
  println(nativeLength(original[1..3])==2)
  var result: int[] = returned(original[1..3])
  original[1] = 9
  println(result.length==2 && result[0]==2 && original[1]==9)
  var nested: int[] = nativeNested(original[0..3])
  println(nested.length==2 && nested[0]==1 && nested[1]==9)
  var count: int = 0
  while count<3 =>
    sink.take(original[0..2])
    count = count+1
  var observed: int = 0
  discard(original[0..2],observed)
  println(count==3 && sink.seen==2 && observed==2)
  println(mixed(original[0..2], "a"+"b")==4)
  println(__chain_tmp_0()+__chain_tmp_1()==33)
'''
EXPECTED = b'true\n'*10
ORDER_SOURCE = r'''@source "main.sn.c"
native fn markedBegin(): int
native fn markedEnd(): int
native fn markedNumber(): int
native fn observe(values: int[], number: int): int
native fn observedTrace(): int
fn main(): void =>
  var source: int[] = {1,2,3}
  var result: int = observe(source[markedBegin()..markedEnd()],markedNumber())
  print(observedTrace())
  print(":")
  println(result)
'''
ORDER_BACKING = r'''#include "sn_array.h"
#include <stdio.h>
static long long trace;
__attribute__((noinline)) long long markedBegin(void) { trace=trace*10+1; return 0; }
__attribute__((noinline)) long long markedEnd(void) { trace=trace*10+2; return 2; }
__attribute__((noinline)) long long markedNumber(void) { trace=trace*10+3; return 7; }
__attribute__((noinline)) long long observe(SnArray *values, long long number) {
    long long result=values->len+number;
#ifdef STRING_ARGUMENT_REFERENCE
    sn_array_free(values);
#endif
    return result;
}
long long observedTrace(void) { return trace; }
'''
ORDER_REFERENCE_MAIN = r'''int main(void) {
    SnArray *source=sn_array_new(sizeof(long long),3);
    source->elem_tag=SN_TAG_INT;
    for (long long i=1;i<=3;i++) sn_array_push(source,&i);
    long long result=observe(sn_array_slice(source,markedBegin(),markedEnd()),markedNumber());
    printf("%lld:%lld\n",observedTrace(),result);
    sn_array_free(source);
    return 0;
}
'''
