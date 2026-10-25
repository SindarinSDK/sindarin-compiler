"""C-derived plain value-record method parameter controls."""
CONTROLS = [
    ('basic', '''struct Point =>
  x: int
  fn increment(other: Point as ref): void =>
    other.x = other.x + self.x
  fn add(other: Point as val): int =>
    self.x = self.x + other.x
    other.x = other.x + 10
    return self.x
  fn replace(other: Point as ref): void =>
    other = Point {x:self.x}
fn main(): void =>
  var point: Point = {x:1}
  var other: Point = {x:2}
  point.increment(other)
  println(point.x==1 && other.x==3)
  println(point.add(other)==4 && point.x==4 && other.x==3)
  point.replace(other)
  println(other.x==4 && point.x==4)
  println(point.add(point)==8 && point.x==8)
''', '''''', b'true\ntrue\ntrue\ntrue\n'),
    ('indexed', '''struct Point =>
  x: int
  fn increment(other: Point as ref): void =>
    other.x = other.x + self.x
  fn add(other: Point as val): int =>
    self.x = self.x + other.x
    other.x = other.x + 10
    return self.x
  fn replace(other: Point as ref): void =>
    other = Point {x:self.x}
fn main(): void =>
  var point: Point = {x:1}
  var values: Point[] = {Point {x:2}}
  point.increment(values[0])
  println(point.x==1 && values[0].x==3)
  println(point.add(values[0])==4 && values[0].x==3)
''', '''''', b'true\ntrue\n'),
    ('sibling-fields', '''struct Point =>
  x: int
  fn increment(other: Point as ref): void =>
    other.x = other.x + self.x
  fn add(other: Point as val): int =>
    self.x = self.x + other.x
    other.x = other.x + 10
    return self.x
  fn replace(other: Point as ref): void =>
    other = Point {x:self.x}
struct Pair =>
  left: Point
  right: Point
fn main(): void =>
  var pair: Pair = {left:Point {x:1}, right:Point {x:2}}
  pair.left.increment(pair.right)
  println(pair.left.x==1 && pair.right.x==3)
  println(pair.left.add(pair.right)==4 && pair.right.x==3)
''', '''''', b'true\ntrue\n'),
    ('copy-hook', '''@source "main.sn.c"
native fn tick(): void
native fn count(): int
struct Item =>
  x: int
  fn copy(): Item =>
    tick()
    return Item {x:self.x+1}
struct Sink =>
  total: int
  fn add(other: Item as val): void =>
    self.total += other.x
fn main(): void =>
  var item: Item = {x:2}
  var sink: Sink = {total:0}
  sink.add(item)
  println(sink.total)
  println(count())
  println(item.x)
''', '''static long long calls;void tick(void){calls++;}long long count(void){return calls;}
''', b'2\n0\n2\n'),
    ('whole-record-reference-result', '''struct Point =>
  x: int
  fn replace(other: Point as ref): Point =>
    return other = Point {x:self.x}
  fn selfCopy(other: Point as ref): Point =>
    other = other
    return other
fn main(): void =>
  var point: Point = {x:1}
  var other: Point = {x:2}
  var result: Point = point.replace(other)
  other.x = 4
  println(result.x==1 && other.x==4 && point.x==1)
  var copied: Point = point.selfCopy(other)
  other.x = 6
  println(copied.x==4 && other.x==6)
''', '''''', b'true\ntrue\n'),
    ('owned-parameter-rebind-result', '''struct Point =>
  x: int
  fn replace(other: Point as val): Point =>
    self.x += 1
    return other = Point {x:5}
fn main(): void =>
  var point: Point = {x:1}
  var other: Point = {x:2}
  var result: Point = point.replace(other)
  println(point.x==2 && other.x==2 && result.x==5)
  var same: Point = point.replace(point)
  println(point.x==3 && same.x==5)
''', '''''', b'true\ntrue\n'),
    ('numeric-reference-and-value-fields', """struct Point =>
  x: int
  y: double
  fn increment(other: Point as ref): int =>
    other.x += self.x
    other.y += self.y
    other.x++
    return other.x
  fn owned(other: Point as val): int =>
    self.x += other.x
    other.x += 10
    other.y += self.y
    return other.x
fn main(): void =>
  var point: Point = {x:1,y:1.5}
  var other: Point = {x:2,y:2.5}
  println(point.increment(other)==4 && other.x==4 && other.y==4.0 && point.x==1)
  println(point.owned(other)==14 && point.x==5 && other.x==4 && other.y==4.0)
""", '', b'true\ntrue\n'),
    ('record-variable-array-values', """struct Point =>
  x: int
native fn nativeCopies(): bool =>
  var first: Point = {x:1}
  var values: Point[] = {first}
  first.x = 3
  return values[0].x==1 && first.x==3
fn main(): void =>
  var first: Point = {x:1}
  var second: Point = {x:2}
  var values: Point[] = {first,second}
  first.x = 3
  println(values[0].x==1 && values[1].x==2 && first.x==3)
  println(nativeCopies())
""", '', b'true\ntrue\n'),
    ('scalar-record-field-families', '''struct Slot0 =>
  value: int
  fn change(other: Slot0 as ref): void =>
    other.value = 9
  fn owned(other: Slot0 as val): Slot0 =>
    self.value = 9
    return other = Slot0 {value:7}
struct Slot1 =>
  value: long
  fn change(other: Slot1 as ref): void =>
    other.value = 9l
  fn owned(other: Slot1 as val): Slot1 =>
    self.value = 9l
    return other = Slot1 {value:7l}
struct Slot2 =>
  value: int32
  fn change(other: Slot2 as ref): void =>
    other.value = 9i32
  fn owned(other: Slot2 as val): Slot2 =>
    self.value = 9i32
    return other = Slot2 {value:7i32}
struct Slot3 =>
  value: uint
  fn change(other: Slot3 as ref): void =>
    other.value = 9u
  fn owned(other: Slot3 as val): Slot3 =>
    self.value = 9u
    return other = Slot3 {value:7u}
struct Slot4 =>
  value: uint32
  fn change(other: Slot4 as ref): void =>
    other.value = 9u32
  fn owned(other: Slot4 as val): Slot4 =>
    self.value = 9u32
    return other = Slot4 {value:7u32}
struct Slot5 =>
  value: byte
  fn change(other: Slot5 as ref): void =>
    other.value = 9b
  fn owned(other: Slot5 as val): Slot5 =>
    self.value = 9b
    return other = Slot5 {value:7b}
struct Slot6 =>
  value: bool
  fn change(other: Slot6 as ref): void =>
    other.value = true
  fn owned(other: Slot6 as val): Slot6 =>
    self.value = true
    return other = Slot6 {value:false}
struct Slot7 =>
  value: char
  fn change(other: Slot7 as ref): void =>
    other.value = 'b'
  fn owned(other: Slot7 as val): Slot7 =>
    self.value = 'b'
    return other = Slot7 {value:'a'}
struct Slot8 =>
  value: float
  fn change(other: Slot8 as ref): void =>
    other.value = 2.5f
  fn owned(other: Slot8 as val): Slot8 =>
    self.value = 2.5f
    return other = Slot8 {value:1.5f}
struct Slot9 =>
  value: double
  fn change(other: Slot9 as ref): void =>
    other.value = 2.5
  fn owned(other: Slot9 as val): Slot9 =>
    self.value = 2.5
    return other = Slot9 {value:1.5}
fn main(): void =>
  var receiver0: Slot0 = {value:7}
  var argument0: Slot0 = {value:7}
  receiver0.change(argument0)
  var result0: Slot0 = receiver0.owned(argument0)
  var values0: Slot0[] = {argument0}
  println(receiver0.value==9 && argument0.value==9 && result0.value==7 && values0[0].value==9 && sizeof(Slot0)==8)
  var receiver1: Slot1 = {value:7l}
  var argument1: Slot1 = {value:7l}
  receiver1.change(argument1)
  var result1: Slot1 = receiver1.owned(argument1)
  var values1: Slot1[] = {argument1}
  println(receiver1.value==9l && argument1.value==9l && result1.value==7l && values1[0].value==9l && sizeof(Slot1)==8)
  var receiver2: Slot2 = {value:7i32}
  var argument2: Slot2 = {value:7i32}
  receiver2.change(argument2)
  var result2: Slot2 = receiver2.owned(argument2)
  var values2: Slot2[] = {argument2}
  println(receiver2.value==9i32 && argument2.value==9i32 && result2.value==7i32 && values2[0].value==9i32 && sizeof(Slot2)==4)
  var receiver3: Slot3 = {value:7u}
  var argument3: Slot3 = {value:7u}
  receiver3.change(argument3)
  var result3: Slot3 = receiver3.owned(argument3)
  var values3: Slot3[] = {argument3}
  println(receiver3.value==9u && argument3.value==9u && result3.value==7u && values3[0].value==9u && sizeof(Slot3)==8)
  var receiver4: Slot4 = {value:7u32}
  var argument4: Slot4 = {value:7u32}
  receiver4.change(argument4)
  var result4: Slot4 = receiver4.owned(argument4)
  var values4: Slot4[] = {argument4}
  println(receiver4.value==9u32 && argument4.value==9u32 && result4.value==7u32 && values4[0].value==9u32 && sizeof(Slot4)==4)
  var receiver5: Slot5 = {value:7b}
  var argument5: Slot5 = {value:7b}
  receiver5.change(argument5)
  var result5: Slot5 = receiver5.owned(argument5)
  var values5: Slot5[] = {argument5}
  println(receiver5.value==9b && argument5.value==9b && result5.value==7b && values5[0].value==9b && sizeof(Slot5)==1)
  var receiver6: Slot6 = {value:false}
  var argument6: Slot6 = {value:false}
  receiver6.change(argument6)
  var result6: Slot6 = receiver6.owned(argument6)
  var values6: Slot6[] = {argument6}
  println(receiver6.value==true && argument6.value==true && result6.value==false && values6[0].value==true && sizeof(Slot6)==1)
  var receiver7: Slot7 = {value:'a'}
  var argument7: Slot7 = {value:'a'}
  receiver7.change(argument7)
  var result7: Slot7 = receiver7.owned(argument7)
  var values7: Slot7[] = {argument7}
  println(receiver7.value=='b' && argument7.value=='b' && result7.value=='a' && values7[0].value=='b' && sizeof(Slot7)==1)
  var receiver8: Slot8 = {value:1.5f}
  var argument8: Slot8 = {value:1.5f}
  receiver8.change(argument8)
  var result8: Slot8 = receiver8.owned(argument8)
  var values8: Slot8[] = {argument8}
  println(receiver8.value==2.5f && argument8.value==2.5f && result8.value==1.5f && values8[0].value==2.5f && sizeof(Slot8)==4)
  var receiver9: Slot9 = {value:1.5}
  var argument9: Slot9 = {value:1.5}
  receiver9.change(argument9)
  var result9: Slot9 = receiver9.owned(argument9)
  var values9: Slot9[] = {argument9}
  println(receiver9.value==2.5 && argument9.value==2.5 && result9.value==1.5 && values9[0].value==2.5 && sizeof(Slot9)==8)
''', '', b'true\n'*10),
]
