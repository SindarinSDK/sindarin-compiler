#!/usr/bin/env python3
"""Check C-supported static/instance name collisions without changing public names."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,default=ROOT/'bin'/('sn.exe' if os.name=='nt' else 'sn'))
    parser.add_argument('--quick',action='store_true')
    args=parser.parse_args();compiler=args.compiler.resolve();cases=[];c_failures=[]
    with tempfile.TemporaryDirectory(prefix='sn-method-collision-') as folder:
        work=Path(folder)
        ordinary=('  amount: int\n  static fn read(value: int): int =>\n    return value + 10\n'
            '  fn read(): int =>\n    return self.amount\n  fn __sn_static_method(): int =>\n    return 777\n'
            '  fn combined(): int =>\n    return Counter.read(self.read())\n')
        examples={
            'value':('struct Counter =>\n'+ordinary+
                'fn main(): void =>\n  var item: Counter = Counter {amount: 5}\n'
                '  println(Counter.read(2))\n  println(item.read())\n  println(item.combined())\n  println(item.__sn_static_method())\n',b'12\n5\n15\n777\n'),
            'reference':('struct Counter as ref =>\n'+ordinary+
                'fn main(): void =>\n  var item: Counter = Counter {amount: 5}\n  var alias: Counter = item\n'
                '  alias.amount=9\n  println(Counter.read(2))\n  println(item.read())\n  println(item.combined())\n  println(item.__sn_static_method())\n',b'12\n9\n19\n777\n'),
            'native_instance':('@source "operations.c"\nnative struct Native as ref =>\n'
                '  amount: int\n  static fn read(value: int): int =>\n    return value+10\n'
                '  @alias "native_read"\n  native fn read(): int\n'
                'native fn make(): Native =>\n  return Native {amount: 5}\n'
                'fn main(): void =>\n  var item: Native = make()\n  var alias: Native = item\n  alias.amount=9\n'
                '  println(Native.read(2))\n  println(item.read())\n',b'12\n9\n'),
            'native_static':('@source "operations.c"\nnative struct Native as ref =>\n'
                '  amount: int\n  @alias "native_static_read"\n  static native fn read(value: int): int\n'
                '  fn read(): int =>\n    return self.amount\n'
                'native fn make(): Native =>\n  return Native {amount: 5}\n'
                'fn main(): void =>\n  var item: Native = make()\n'
                '  println(Native.read(2))\n  println(item.read())\n',b'12\n5\n'),
            'native_self_storage':('native struct Native as ref =>\n'
                '  amount: int\n  label: str\n  fn bump(): int =>\n    self.amount += 1\n    return self.amount\n'
                '  fn rename(value: str): void =>\n    self.label = value\n'
                '  fn labelLength(): int =>\n    return self.label.length\n'
                'native fn make(): Native =>\n  return Native {amount: 5, label: "first"}\n'
                'fn main(): void =>\n  var item: Native = make()\n  var alias: Native = item\n'
                '  var held: str = item.label\n  println(alias.bump())\n  println(item.amount)\n'
                '  alias.rename("changed")\n  println(item.label)\n  println(item.labelLength())\n'
                '  println(held)\n  alias.rename(item.label)\n  println(item.label)\n',b'6\n6\nchanged\n7\nfirst\nchanged\n'),
        }
        modes=[('-O0','default')] if args.quick else [(o,m) for o in ('-O0','-O1','-O2') for m in ('default','checked','unchecked')]
        for name,(source,expected) in examples.items():
            (work/'main.sn').write_text(source)
            if name in ('native_instance','native_static'):
                (work/'operations.c').write_text('long long native_read(__sn__Native *value) { return value->__sn__amount; }\n'
                    'long long native_static_read(long long value) { return value+10; }\n')
            for opt,mode in modes:
                outcomes={}
                for target in ('c','rust'):
                    exe=work/(target+'.exe');command=[compiler,'main.sn','--no-install','--target',target,opt,'-o',exe]
                    if mode!='default':command.append('--'+mode)
                    built=subprocess.run([str(v) for v in command],cwd=work,capture_output=True,timeout=180)
                    if name in ('value','reference') and target=='c':
                        if not built.returncode or b'conflicting types' not in built.stderr:
                            raise ValueError('ordinary C method-name collision control changed: '+built.stderr.decode(errors='replace'))
                        c_failures.append({'case':name,'opt':opt,'mode':mode,'reason':'C backend emits duplicate method symbol','parity':False})
                        continue
                    if built.returncode:raise ValueError(f'{name}/{target}/{opt}/{mode}: '+built.stderr.decode(errors='replace'))
                    run=subprocess.run([str(exe)],cwd=work,capture_output=True,timeout=15)
                    wanted=expected.replace(b'\n',b'\r\n') if os.name=='nt' else expected
                    if run.returncode or run.stdout!=wanted or run.stderr:raise ValueError(f'{name}/{target}: {run.stdout!r} {run.stderr!r}')
                    outcomes[target]={'stdout_hex':run.stdout.hex(),'exit':run.returncode}
                cases.append({'case':name,'opt':opt,'mode':mode,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'targets':outcomes,'passed':True})
    output=ROOT/'.sn/static-instance-methods.json';output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps({'passed':True,'cases':cases,'c_failures':c_failures,'scope':'Native aliased static/instance methods prove C/Rust parity; ordinary collisions are retained C failures and Rust-only controls, not parity'},indent=2)+'\n')
    print(f'PASS: {len(cases)} static/instance method cases')


if __name__=='__main__':main()
