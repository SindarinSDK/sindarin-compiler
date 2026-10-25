"""Caller string loans, callee rebinds and C argument-order controls."""

SOURCE = r'''fn length(value: str): int =>
  return value.length
fn returned(value: str): str =>
  return value
fn discard(value: str, result: int as ref): void =>
  result = value.length
native fn nativeLength(value: str): int =>
  return value.length
native fn nativeAssigned(value: str): str =>
  return value = "changed"
native fn nativeNested(value: str): str =>
  return nativeAssigned(value+"!")
struct Sink =>
  seen: int
  fn take(value: str): int =>
    self.seen = value.length
    return self.seen
  static fn count(value: str): int =>
    return value.length
native fn __chain_tmp_0(): int =>
  return 11
native fn __chain_tmp_1(): int =>
  return 22
fn main(): void =>
  var base: str = "base"
  var sink: Sink = {seen:0}
  println(length(base+"-fn")==7)
  println(Sink.count(base+"-static")==11)
  println(sink.take(base+"-method")==11 && sink.seen==11)
  var callback: fn(str): int = fn(value: str): int =>
    return value.length
  println(callback(base+"-closure")==12)
  println(nativeLength(base+"-native")==11)
  println(returned(base+"-return")=="base-return" && base=="base")
  println(nativeNested(base+"-outer")=="changed" && base=="base")
  var count: int = 0
  while count<3 =>
    sink.take(base+"-loop")
    count = count+1
  println(count==3 && sink.seen==9)
  var observed: int = 0
  discard(base+"-void",observed)
  println(observed==9)
  println(__chain_tmp_0()+__chain_tmp_1()==33)
'''

EXPECTED = b'true\n' * 10

ORDER_SOURCE = r'''@source "main.sn.c"
native fn markedText(): str
native fn markedNumber(): int
native fn observe(value: str, number: int): int
native fn observedTrace(): int
fn main(): void =>
  var result: int = observe(markedText()+"!",markedNumber())
  print(observedTrace())
  print(":")
  println(result)
'''

ORDER_BACKING = r'''#include "sn_string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static long long trace;
__attribute__((noinline)) char *markedText(void) { trace=trace*10+1; char *value=malloc(2); memcpy(value,"a",2); return value; }
__attribute__((noinline)) long long markedNumber(void) { trace=trace*10+2; return 2; }
__attribute__((noinline)) long long observe(char *value, long long number) {
    long long result=strcmp(value,"a!")==0 ? number : -1;
#ifdef STRING_ARGUMENT_REFERENCE
    free(value);
#endif
    return result;
}
long long observedTrace(void) { return trace; }
'''

ORDER_REFERENCE_MAIN = r'''int main(void) {
    long long result=observe(({ char *part=markedText(); char *joined=sn_str_concat(part,"!"); free(part); joined; }),markedNumber());
    printf("%lld:%lld\n",observedTrace(),result);
    return 0;
}
'''
