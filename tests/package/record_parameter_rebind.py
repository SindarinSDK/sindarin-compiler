#!/usr/bin/env python3
"""Borrowed record parameter rebinding preserves caller identity and owned replacements."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import unittest
import rust_record_bodies as records

ROOT=records.ROOT
COMPILER=records.COMPILER


class RecordParameterRebindTests(unittest.TestCase):
    def test_c_and_rust_body_rebinding_preserves_borrowed_owners(self):
        cases=[]
        for runtime in ('C','RS'):
            fixture=records.RustRecordBodyTests();fixture.setUp()
            try:
                api=fixture.package/'src/api.sn'
                api.write_text(api.read_text().replace('  amount: int\n',
                    '  amount: int\n'
                    '  static fn rebindStatic(item: Counter): Counter =>\n'
                    '    item=Counter.make(31)\n    item=item\n    return item\n'
                    '  fn rebindInstance(item: Counter): Counter =>\n'
                    '    item=Counter.make(41)\n    return item\n')+
                    'fn bodyRebind(item: Counter): Counter =>\n'
                    '  var saved: Counter = item\n  item=item\n'
                    '  item=Counter.make(11)\n  item=nil\n'
                    '  item=Counter.make(13)\n  return item\n')
                manifest=fixture.manifest.replace('runtime: RS','runtime: '+runtime)
                manifest+=fixture.binding('bodyRebind','bodyRebind','facade','owned','item: borrowed')
                manifest+=fixture.binding('Counter.rebindStatic','Counter.rebindStatic','facade','owned','item: borrowed','static')
                manifest+=fixture.binding('Counter.rebindInstance','Counter.rebindInstance','facade','owned','self: borrowed, item: borrowed','instance')
                fixture.write('.sn/methods/sn.yaml',manifest)
                fixture.write('main.sn','import "methods/src/api"\n'
                    'fn local(item: Counter): Counter =>\n  item=item\n'
                    '  item=Counter.make(51)\n  item=nil\n  item=Counter.make(61)\n  return item\n'
                    'fn exercise(): void =>\n  var original: Counter = Counter.make(5)\n'
                    '  var alias: Counter = original\n  var replacement: Counter = bodyRebind(original)\n'
                    '  println(original.amount)\n  println(alias.amount)\n  println(replacement.amount)\n'
                    '  println(same(original,alias))\n  var stat: Counter = Counter.rebindStatic(original)\n'
                    '  println(original.amount)\n  println(stat.amount)\n'
                    '  var instance: Counter = original.rebindInstance(original)\n'
                    '  println(original.amount)\n  println(instance.amount)\n'
                    '  var ordinary: Counter = local(original)\n  println(original.amount)\n  println(ordinary.amount)\n'
                    '  var fromNil: Counter = bodyRebind(nil)\n  println(fromNil.amount)\n'
                    'fn main(): void =>\n  exercise()\n  println(destroyed())\n')
                assembly=fixture.build()
                for artifact in ('source','sealed'):
                    if artifact=='sealed':
                        shutil.copytree(assembly.parent,fixture.package/'sealed')
                        descriptor=fixture.package/'sealed'/assembly.name
                        digest=hashlib.sha256(descriptor.read_bytes()).hexdigest()
                        fixture.write('.sn/methods/sn.yaml',manifest.replace('native:\n',
                            f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                        (fixture.package/'src/counter.c').unlink()
                    for target in ('c','rust'):
                        for opt in ('-O0','-O1','-O2'):
                            for mode in ('default','checked','unchecked'):
                                exe=fixture.root/'consumer.exe'
                                command=[str(COMPILER),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                                if mode!='default':command.append('--'+mode)
                                run=subprocess.run(command,cwd=fixture.root,capture_output=True,timeout=180)
                                self.assertEqual(run.returncode,0,run.stderr.decode(errors='replace'))
                                run=subprocess.run([str(exe)],cwd=fixture.root,capture_output=True,timeout=20)
                                expected=b'5\n5\n13\ntrue\n5\n31\n5\n41\n5\n61\n13\n9\n'
                                if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                                self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))
                                cases.append(dict(body_runtime=runtime,artifact=artifact,target=target,opt=opt,mode=mode,passed=True))
            finally:fixture.temp.cleanup()
        suffix='-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS','') else ''
        report=ROOT/f'.sn/record-parameter-rebind{suffix}.json';report.parent.mkdir(exist_ok=True)
        report.write_text(json.dumps(dict(passed=True,cases=cases),indent=2)+'\n')


if __name__=='__main__':unittest.main(verbosity=2)
