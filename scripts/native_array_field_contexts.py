"""Independent source controls for canonical native array-field ownership."""

COMMON = """import "sindarin-pkg-sdk/src/net/udp"
native fn makeResult(): UdpReceiveResult =>
  return UdpReceiveResult { _data: {1b,2b}, _sender: "before" }
fn change(values: byte[]): int =>
  values.push(3b)
  values = {9b}
  return values.length
fn bytes(): byte[] =>
  return makeResult()._data
fn save(values: byte[]): fn(): int =>
  return fn(): int => values.length
fn records(): UdpReceiveResult[] =>
  return {makeResult()}
var made: int = 0
fn countedResult(): UdpReceiveResult =>
  made += 1
  return makeResult()
fn delayed(): fn(): int =>
  return fn(): int =>
    return countedResult()._data.length
fn delayedInline(): fn(): int =>
  return fn(): int => countedResult()._data.length
fn delayedBytes(): fn(): byte[] =>
  return fn(): byte[] => countedResult()._data
fn identity(value: UdpReceiveResult): UdpReceiveResult =>
  return value
"""

FIELD_CASES = [
    (
        'inline-lambda-reads',
        """fn main(): void =>
  var action: fn(): int = delayedInline()
  println(made==0)
  println(action()==2 && made==1)
  println(action()==2 && made==2)
""",
        b'true\ntrue\ntrue\n',
    ),
    (
        'inline-lambda-array',
        """fn main(): void =>
  var action: fn(): byte[] = delayedBytes()
  var a: byte[] = action()
  var b: byte[] = action()
  a.push(9b)
  println(a.length==3 && b.length==2 && made==2)
""",
        b'true\n',
    ),
    (
        'factory-returned-alias',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  println(change(identity(r)._data)==1)
  println(r._data.length==3)
""",
        b'true\ntrue\n',
    ),
    (
        'lazy-reads',
        """fn main(): void =>
  println(false && countedResult()._data.length==2)
  println(true || countedResult()._data.length==2)
  println(made==0)
""",
        b'false\ntrue\ntrue\n',
    ),
    (
        'loop-header-reads',
        """fn main(): void =>
  var iteration: int = 0
  while countedResult()._data.length==2 && iteration<3 =>
    iteration += 1
  println(iteration==3 && made==4)
""",
        b'true\n',
    ),
    (
        'escaped-lambda-reads',
        """fn main(): void =>
  var action: fn(): int = delayed()
  println(made==0)
  println(action()==2 && made==1)
  println(action()==2 && made==2)
""",
        b'true\ntrue\ntrue\n',
    ),
    (
        'temporary-indexed-call',
        """fn main(): void =>
  println(change(records()[0]._data)==1)
""",
        b'true\n',
    ),
    (
        'temporary-indexed-copy',
        """fn main(): void =>
  var bytes: byte[] = records()[0]._data
  println(bytes.length==2 && bytes[1]==2b)
""",
        b'true\n',
    ),
    (
        'assignment-result-call',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  println(change(r._data = {4b,5b})==1)
  println(r._data.length==3 && r._data[2]==3b)
""",
        b'true\ntrue\n',
    ),
    (
        'borrowed-assignment-result-call',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  var bytes: byte[] = {4b,5b}
  println(change(r._data = bytes)==1)
  println(r._data.length==3 && bytes.length==2)
""",
        b'true\ntrue\n',
    ),
    (
        'self-assignment-result-call',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  println(change(r._data = r._data)==1)
  println(r._data.length==3)
""",
        b'true\ntrue\n',
    ),
    (
        'assignment-result-copy',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  var a: byte[] = (r._data = {4b,5b})
  a.push(9b)
  println(r._data.length==2 && a.length==3)
""",
        b'true\n',
    ),
    (
        'copy-read',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  var a: byte[] = r._data
  a.push(9b)
  println(r._data.length==2 && a.length==3)
""",
        b'true\n',
    ),
    (
        'owned-return',
        """fn main(): void =>
  var a: byte[] = bytes()
  println(a.length==2 && a[0]==1b && a[1]==2b)
""",
        b'true\n',
    ),
    (
        'default-call',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  println(change(r._data)==1)
  println(r._data.length==3)
""",
        b'true\ntrue\n',
    ),
    (
        'owned-call',
        """fn main(): void =>
  println(change(makeResult()._data)==1)
""",
        b'true\n',
    ),
    (
        'indexed-call',
        """fn main(): void =>
  var records: UdpReceiveResult[] = {makeResult()}
  println(change(records[0]._data)==1)
  println(records[0]._data.length==3)
""",
        b'true\ntrue\n',
    ),
    (
        'closure-call',
        """fn main(): void =>
  var action: fn(): int = save(makeResult()._data)
  println(action()==2)
""",
        b'true\n',
    ),
    (
        'field-replace',
        """fn main(): void =>
  var r: UdpReceiveResult = makeResult()
  r._data = {4b,5b}
  println(r._data[0]==4b && r._data[1]==5b)
""",
        b'true\n',
    ),
    (
        'borrowed-record-parameter-fields',
        """fn parameterFieldStore(value: UdpReceiveResult as ref): void =>
  value._data[0]=3b
  value._sender="after"
  value._data={4b,5b}
fn main(): void =>
  var value: UdpReceiveResult = makeResult()
  var alias: UdpReceiveResult = value
  var retained: byte[] = value._data
  parameterFieldStore(value)
  println(alias._sender=="after" && alias._data[0]==4b && alias._data[1]==5b)
  println(retained[0]==1b && retained[1]==2b)
""",
        b'true\ntrue\n',
    ),
]


FIELD_CASES.append((
    'helper-name-collisions',
    'fn main(): void =>\n'
    + ''.join(f'  var __chain_tmp_{i}: int = {i}\n' for i in range(64))
    + '  var __sn_native_field_owner_0: int = 7\n'
    + '  var __sn_native_field_value_0: byte[] = {4b,5b}\n'
    + '  var __sn_native_field_raw_0: int = 7\n'
    + '  var r: UdpReceiveResult = makeResult()\n'
    + '  r._data = __sn_native_field_value_0\n'
    + '  println(change(countedResult()._data)==1 && r._data[0]==4b)\n'
    + '  println(__chain_tmp_0 + __chain_tmp_63==63)\n',
    b'true\ntrue\n',
))


def combined_controls(native_declaration=None):
    source = COMMON
    if native_declaration is not None:
        source = source.replace('import "sindarin-pkg-sdk/src/net/udp"', native_declaration, 1)
    calls = []
    expected = b''
    for label, body, stdout in FIELD_CASES:
        name = 'check_' + label.replace('-', '_')
        source += body.replace('fn main(): void =>', 'fn ' + name + '(): void =>', 1)
        calls.append('  made = 0\n  ' + name + '()\n')
        expected += stdout
    return source + 'fn main(): void =>\n' + ''.join(calls), expected
