"""Independent array rebinding contracts for default borrowed parameters."""
LEAVES = [('int', '7', '9'), ('long', '7l', '9l'), ('int32', '7i32', '9i32'),
          ('uint', '7u', '9u'), ('uint32', '7u32', '9u32'), ('byte', '7b', '9b'),
          ('bool', 'false', 'true'), ('char', "'a'", "'b'"),
          ('float', '1.5f', '2.5f'), ('double', '1.5', '2.5'), ('str', '"before"', '"after"')]


def families(native=False):
    functions, main = [], ['fn main(): void =>']
    for i, (kind, old, new) in enumerate(LEAVES):
        functions += [f'{"native " if native else ""}fn change{i}(value: {kind}[]): {kind}[] =>',
                      f'  value.push({old})', f'  value = {{{new}}}',
                      f'  value.push({new})', '  return value']
        main += [f'  var source{i}: {kind}[] = {{{old}}}',
                 f'  var result{i}: {kind}[] = change{i}(source{i})',
                 f'  println(source{i}.length==2 && source{i}[0]=={old} && result{i}.length==2 && result{i}[0]=={new})']
    return '\n'.join(functions + main) + '\n'


CONTEXTS = '''fn activate(value: int[]): int[] =>
  return value = {2}
fn selfCopy(value: int[]): int[] =>
  value = value
  value[0] = 9
  return value
fn repeated(value: int[]): int[] =>
  value = {4}
  value = {5}
  return value = {6}
fn nested(value: int[][]): int[][] =>
  value = {{8}}
  value[0].push(9)
  return value
fn shadowed(value: int[]): int[] =>
  if true =>
    var value: int[] = {3}
    value = {4}
    println(value[0]==4)
  return value = {5}
fn erased(value: int[]): int[] =>
  return value = nil
fn filled(value: int[]): int[] =>
  return value = {9}
fn stringValues(value: str[]): str[] =>
  var missing: str = nil
  return value = {missing,"\\x80\\xff"}
struct Ops =>
  tag: int
  fn replace(value: int[]): int[] =>
    value = {self.tag}
    return value
  static fn change(value: int[]): int[] =>
    return value = {7}
native struct NativeOps as ref =>
  tag: int
  fn nativeReplace(value: int[]): int[] =>
    value = {self.tag}
    return value
  static fn nativeChange(value: int[]): int[] =>
    return value = {7}
native fn makeOps(): NativeOps =>
  return NativeOps {tag:8}
fn main(): void =>
  var original: int[] = {1,2}
  var result: int[] = activate(original)
  println(result[0]==2 && original[0]==1)
  var selfResult: int[] = selfCopy(original)
  println(selfResult[0]==9 && original[0]==1)
  var repeatedResult: int[] = repeated(original)
  println(repeatedResult[0]==6 && original[0]==1)
  var rows: int[][] = {{1}}
  var nestedResult: int[][] = nested(rows)
  println(rows[0].length==1 && rows[0][0]==1 && nestedResult[0].length==2 && nestedResult[0][0]==8)
  var shadowResult: int[] = shadowed(original)
  println(shadowResult[0]==5 && original[0]==1)
  var ops: Ops = Ops {tag:8}
  println(ops.replace(original)[0]==8 && original[0]==1)
  println(Ops.change(original)[0]==7 && original[0]==1)
  var nativeOps: NativeOps = makeOps()
  println(nativeOps.nativeReplace(original)[0]==8 && original[0]==1)
  println(NativeOps.nativeChange(original)[0]==7 && original[0]==1)
  var privateSelf = fn(value: int[]): int[] =>
    value = value
    value[0] = 9
    value = value
    return value
  var privateResult: int[] = privateSelf(original)
  println(privateResult[0]==9 && privateResult.length==2 && original[0]==1)
  var replacement: int[] = {3}
  var captured = fn(value: int[]): int[] =>
    value = replacement
    value.push(4)
    return value
  var capturedResult: int[] = captured(original)
  println(capturedResult.length==2 && replacement.length==1 && original.length==2)
  var assignment = fn(value: int[]): int[] =>
    return value = {4}
  println(assignment(original)[0]==4 && original[0]==1)
  var expression: fn(int[]): int[] = fn(value: int[]): int[] => value = {5}
  println(expression(original)[0]==5 && original[0]==1)
  println(erased(original)==nil && original[0]==1)
  var missing: int[] = nil
  var fromNil: int[] = filled(missing)
  println(missing==nil && fromNil[0]==9)
  var empty: int[] = {}
  var fromEmpty: int[] = filled(empty)
  println(empty!=nil && empty.length==0 && fromEmpty[0]==9)
  println(sizeof(original)==8 && sizeof(fromNil)==8 && sizeof(fromEmpty)==8)
  var words: str[] = {"before"}
  var changedWords: str[] = stringValues(words)
  println(words[0]=="before" && changedWords[0]==nil && changedWords[1].length==2 && changedWords[1].toBytes()[0]==128b)
  var more: int[] = {1}
  for index in 0..3 =>
    var changed: int[] = repeated(more)
    println(changed[0]==6 && more[0]==1)
'''
CONTEXT_OUTPUTS = 22

COPY_EFFECTS = '@source "main.sn.c"\nnative fn watch(value: int[]): void\nnative fn reset(): void\nnative fn events(): int\nnative fn copies(): int\nfn replaced(value: int[], replacement: int[]): int[] =>\n  return value = replacement\nfn replacedVariable(value: int[], replacement: int[]): int[] =>\n  value = replacement\n  return value\nnative fn nativeReplaced(value: int[], replacement: int[]): int[] =>\n  return value = replacement\nnative fn nativeReplacedVariable(value: int[], replacement: int[]): int[] =>\n  value = replacement\n  return value\nfn main(): void =>\n  var original: int[] = {1}\n  var replacement: int[] = {2}\n  watch(replacement)\n  reset()\n  var a: int[] = replaced(original,replacement)\n  println(copies())\n  println(events())\n  reset()\n  var b: int[] = replacedVariable(original,replacement)\n  println(copies())\n  println(events())\n  reset()\n  var c: int[] = nativeReplaced(original,replacement)\n  println(copies())\n  println(events())\n  reset()\n  var d: int[] = nativeReplacedVariable(original,replacement)\n  println(copies())\n  println(events())\n  var callback: fn(int[]): int[] = fn(value: int[]): int[] =>\n    value = replacement\n    return value\n  reset()\n  var e: int[] = callback(original)\n  println(copies())\n  println(events())\n  println(original[0]==1 && a[0]==2 && b[0]==2 && c[0]==2 && d[0]==2 && e[0]==2)\n'
COPY_BACKING = '#include "sn_types.h"\n#include <string.h>\nstatic long long count;\nstatic uint64_t trace;\nstatic void released(void*a){(void)a;trace=trace*10+3;}\nstatic void copied(const void*a,void*b){count++;trace=trace*10+1;memcpy(b,a,sizeof(long long));}\nvoid watch(SnArray*value){value->elem_copy=copied;value->elem_release=released;}\nvoid reset(void){count=0;trace=0;}\nlong long events(void){return (long long)trace;}\nlong long copies(void){return count;}\n'
COPY_EXPECTED = b'2\n113\n2\n113\n2\n113\n2\n113\n2\n113\ntrue\n'
