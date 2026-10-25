"""Independent C-clean string match assignment, selection and mutation controls."""
CONTROL = '''@source "main.sn.c"
native fn reset(): void
native fn step(kind: int): int
native fn trace(): int
fn marked(text: str, kind: int): str =>
  step(kind)
  return text
struct Label =>
  text: str
  fn take(): str =>
    step(4)
    self.text = "after"
    return self.text
fn lazyPatterns(): void =>
  reset()
  var pattern: str = "before"
  var values: str[] = {"first","tail"}
  var result: str = match marked("hit",1) =>
    (pattern = marked("miss",2)) => marked("bad",8)
    (pattern = marked("hit",3)) => values.pop()
    (pattern = marked("bad",4)) => marked("bad",9)
    else => "bad"
  println(trace()==123)
  println(result=="tail" && values.length==1 && pattern=="hit")
fn indexedPatterns(): void =>
  reset()
  var patterns: str[] = {"hit","miss"}
  var found: bool = false
  match "hit" =>
    patterns[1] => step(8)
    patterns[0] => found = true
    else => step(9)
  println(found && trace()==0)
fn assignedResult(): void =>
  reset()
  var value: str = "before"
  var result: str = match 1 =>
    1 => value = marked("after",2)
    else => marked("bad",8)
  println(result=="after" && value=="after" && trace()==2)
  value = "later"
  println(result=="after")
fn poppedResult(): void =>
  reset()
  var values: str[] = {"first","last"}
  var result: str = match 2 =>
    1 => marked("bad",8)
    2 => values.pop()
    else => marked("bad",9)
  println(result=="last")
  result = "edited"
  println(values[0]=="first" && trace()==0)
fn methodResult(): void =>
  reset()
  var label: Label = {text:"before"}
  var result: str = match 1 =>
    1 => label.take()
    else => marked("bad",8)
  println(result=="after" && label.text=="after" && trace()==4)
  label.text = "later"
  println(result=="after")
fn nestedAssignments(): void =>
  reset()
  var left: str = "before"
  var right: str = "before"
  var result: str = match 1 =>
    1 => left = (right = "hit")
    else => "bad"
  println(result=="hit" && left=="hit" && right=="hit")
  println(trace()==0)
fn copiedResult(): void =>
  var source: str = "one"
  var result: str = match 1 =>
    1 => copyOf(source)
    else => "bad"
  source = "later"
  println(result=="one" && source=="later")
native fn nativeCopy(): str =>
  var source: str = "one"
  var result: str = match 1 =>
    1 => copyOf(source)
    else => "bad"
  return result
native fn nativeAssigned(value: str): str =>
  return value = "after"
native fn nativeAppended(value: str): str =>
  return value += "!"
native struct NativeLabel as ref =>
  marker: int
  fn assigned(value: str): str =>
    return value = "after"
  fn appended(value: str): str =>
    return value += "!"
native fn nativeLabel(): NativeLabel =>
  return NativeLabel {marker:1}
native fn nativeClosure(value: str): str =>
  var callback: fn(str): str = fn(input: str): str =>
    return input = "after"
  return callback(value)
native fn nativeClosureRestored(value: str): str =>
  var callback: fn(str): str = fn(input: str): str =>
    return input = "after"
  var result: str = callback(value)
  if result=="after" =>
    return value += "!"
  return "bad"
fn nativeParameterControls(): void =>
  var source: str = "before"
  println(nativeAssigned(source)=="after" && source=="before")
  println(nativeAppended(source)=="before!" && source=="before")
  var label: NativeLabel = nativeLabel()
  println(label.assigned(source)=="after" && source=="before")
  println(label.appended(source)=="before!" && source=="before")
  println(nativeClosure(source)=="after" && source=="before")
  println(nativeClosureRestored(source)=="before!" && source=="before")
fn closurePatterns(): void =>
  var choose: fn(): str = fn(): str =>
    var pattern: str = "before"
    return match "hit" =>
      (pattern = "hit") => pattern
      else => "bad"
  var result: str = choose()
  println(result=="hit")
fn closurePop(): void =>
  var choose: fn(): str = fn(): str =>
    var values: str[] = {"first","last"}
    return match 1 =>
      1 => values.pop()
      else => "bad"
  var result: str = choose()
  println(result=="last")
fn assignmentOnly(): str =>
  var value: str = "before"
  return match 1 =>
    1 => value = "after"
    else => "bad"
fn parameterDirect(value: str): str =>
  return value = "after"
fn parameterMatched(value: str): str =>
  return match 1 =>
    1 => value = "after"
    else => "bad"
fn parameterSelf(value: str): str =>
  return match 1 =>
    1 => value = value
    else => "bad"
fn parameterShadow(value: str): str =>
  if value=="before" =>
    var value: str = "local"
    return match 1 =>
      1 => value = "shadow"
      else => "bad"
  return "bad"
fn parameterControls(): void =>
  var source: str = "before"
  println(parameterDirect(source)=="after" && source=="before")
  println(parameterMatched(source)=="after" && source=="before")
  println(parameterSelf(source)=="before" && source=="before")
  println(parameterShadow(source)=="shadow" && source=="before")
fn parameterAppend(value: str): str =>
  return value += "!"
fn parameterAppendSelf(value: str): str =>
  return value += value
fn parameterAppendControls(): void =>
  var source: str = "before"
  println(parameterAppend(source)=="before!" && source=="before")
  println(parameterAppendSelf(source)=="beforebefore" && source=="before")
fn selectedPrefix(): void =>
  reset()
  var value: str = "before"
  var result: str = match 1 =>
    1 =>
      step(5)
      value = marked("after",6)
    else =>
      step(8)
      "bad"
  println(result=="after" && value=="after" && trace()==56)
fn main(): void =>
  lazyPatterns()
  indexedPatterns()
  assignedResult()
  poppedResult()
  methodResult()
  nestedAssignments()
  selectedPrefix()
  copiedResult()
  parameterControls()
  parameterAppendControls()
  println(assignmentOnly()=="after")
  println(nativeCopy()=="one")
  nativeParameterControls()
  closurePatterns()
  closurePop()
'''
BACKING = '''#include "sn_types.h"
static long long order;
void reset(void) { order=0; }
long long step(long long value) { order=order*10+value; return value; }
long long trace(void) { return order; }
'''
EXPECTED = b'true\n' * 29

ORIGINALS = [('tests/rgen/errors/by_value_parameter_string_compound_mutation.sn', '789065537f14b4319aee0bb75bc988b17663ac6b77cf476026ab62a94b2182d0', '1f318c3dece4d6f03ac54cf0a5d3c22b3d113233cd70a1bbb0c79d49e8c56967', '736f75726365210a'), ('tests/rgen/errors/by_value_parameter_direct_assignment_string.sn', 'b2478a8688cc329325c9d90b0e41b50db67ba5ea0a4398194863e5ae6e71e70f', 'f610fd1649803f97a3f8bf2984eedcbc09f65e173893980f8f4889300f746db1', '6368616e6765640a'), ('tests/rgen/errors/value_match_string_copy_of_result.sn', '2a6f23d7ae02a8f28c38ce9d11869fd9d9aca02a26fd8b4dd2adcfcf825989f3', '667fc3fd4dd188c9d94cd377618cbf9c363534c839f3c5ba8bad50656d3771bd', '6f6e650a'), ('tests/rgen/errors/statement_match_string_indexed_pattern.sn', '4a4c3d11ddb930449db339e7a9f598c386f788ff7d4ed565f4918b1cfec5941d', '4aa2fd9963db72287d66dc8c1d36da8b7a188cdc7da4130e487538bab32370a0', '6d6174636865640a'), ('tests/rgen/errors/value_match_string_mutation_pattern.sn', '46f1284748aeeff4a9e896eb7067343b6fcb6d40289fd811fcf031818052d15f', '73d2d403889379b0253325c8f586d96b5c3a99e855e229a0dcf25dd3878a53bb', ''), ('tests/rgen/errors/value_match_string_mutating_assignment.sn', 'a691ca44b4b19c8aaa08673126f92c8a6d0c835fe965910493018543e2a5050a', '667fc3fd4dd188c9d94cd377618cbf9c363534c839f3c5ba8bad50656d3771bd', '61667465720a'), ('tests/rgen/errors/value_match_string_mutating_array_call.sn', 'bc6f1030a5dd2330df578b1664b8723b98378f7f9a6fdb431dae80b7cc35719e', '682a55b99d759c724097f495ac68a334042f431a2c41be7078f7441a494fbb96', '6e6f0a'), ('tests/rgen/errors/value_match_string_mutating_member_call.sn', '8ee33a464b7eca165e14a8c6d461359a95e15df3d571c97fec2bfb485619b8ed', '682a55b99d759c724097f495ac68a334042f431a2c41be7078f7441a494fbb96', '61667465720a')]
