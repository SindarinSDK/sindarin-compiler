#!/usr/bin/env python3
"""Canonical package record copies own distinct storage in source/sealed consumers."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import unittest
import rust_record_bodies as fixtures

ROOT=fixtures.ROOT
COMPILER=fixtures.COMPILER


class RecordCopyTests(unittest.TestCase):
    def setUp(self):
        f=fixtures.RustRecordBodyTests();f.setUp();self.addCleanup(f.temp.cleanup)
        self.fixture=f;self.root=f.root;self.package=f.package
        header=self.package/'src/counter.h'
        header.write_text(header.read_text().replace('#endif\n','CanonicalCounter *counter_copy(const CanonicalCounter *);\n#endif\n'))
        source=self.package/'src/counter.c'
        source.write_text(source.read_text()+'CanonicalCounter *counter_copy(const CanonicalCounter *value) { if(!value) return NULL; CanonicalCounter *copy=counter_create();copy->__sn__amount=value->__sn__amount;return copy; }\n')
        self.manifest=f.manifest.replace('runtime: RS','runtime: C').replace('      owner: atomic','      owner: atomic\n      copy: counter_copy')
        f.write('.sn/methods/sn.yaml',self.manifest)
        f.write('main.sn','import "methods/src/api"\n'
            'struct Calls as ref =>\n  value: int\n'
            '  static fn staticCopy(item: Counter as val): Counter =>\n    item.amount=21\n    return item\n'
            '  fn instanceCopy(item: Counter as val): Counter =>\n    item.amount=31\n    return item\n'
            'fn mutate(item: Counter as val): Counter =>\n  item.amount=17\n  return item\n'
            'fn nilCopy(item: Counter as val): bool =>\n  return item==nil\n'
            'fn rebound(item: Counter as val): Counter =>\n  item=item\n  item=nil\n'
            '  item=Counter.make(41)\n  return item\n'
            'fn exercise(): void =>\n  var original: Counter = Counter.make(5)\n'
            '  var copied: Counter = mutate(original)\n  println(original.amount)\n  println(copied.amount)\n  println(same(original,copied))\n'
            '  var callable = mutate\n  var indirect: Counter = callable(original)\n  println(original.amount)\n  println(indirect.amount)\n'
            '  var stat: Counter = Calls.staticCopy(original)\n  println(original.amount)\n  println(stat.amount)\n'
            '  var calls: Calls = {value:0}\n  var instance: Counter = calls.instanceCopy(original)\n  println(original.amount)\n  println(instance.amount)\n'
            '  var next: Counter = rebound(original)\n  println(original.amount)\n  println(next.amount)\n'
            '  println(nilCopy(nil))\nfn main(): void =>\n  exercise()\n  println(destroyed())\n')

    def test_canonical_copy_survives_source_and_sealed_consumption(self):
        assembly=self.fixture.build();cases=[]
        for artifact in ('source','sealed'):
            if artifact=='sealed':
                shutil.copytree(assembly.parent,self.package/'sealed')
                descriptor=self.package/'sealed'/assembly.name;digest=hashlib.sha256(descriptor.read_bytes()).hexdigest()
                self.fixture.write('.sn/methods/sn.yaml',self.manifest.replace('native:\n',f'native:\n  assembly: {{path: sealed/{assembly.name}, sha256: {digest}}}\n'))
                (self.package/'src/counter.c').unlink()
            for target in ('c','rust'):
                for opt in ('-O0','-O1','-O2'):
                    for mode in ('default','checked','unchecked'):
                        exe=self.root/'client.exe';command=[str(COMPILER),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                        if mode!='default':command.append('--'+mode)
                        build=subprocess.run(command,cwd=self.root,capture_output=True,timeout=180)
                        self.assertEqual(build.returncode,0,build.stderr.decode(errors='replace'))
                        run=subprocess.run([str(exe)],cwd=self.root,capture_output=True,timeout=30)
                        expected=b'5\n17\nfalse\n5\n17\n5\n21\n5\n31\n5\n41\ntrue\n7\n'
                        if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                        self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))
                        cases.append(dict(artifact=artifact,target=target,opt=opt,mode=mode,passed=True))
        suffix='-sanitized' if 'sanitize=' in os.environ.get('SN_CFLAGS','') else ''
        report=ROOT/f'.sn/record-copy-contract{suffix}.json';report.parent.mkdir(exist_ok=True)
        report.write_text(json.dumps(dict(passed=True,cases=cases),indent=2)+'\n')

    def test_accepted_sdk_uuid_copy_keeps_original_fields(self):
        import sys
        sys.path.insert(0,str(ROOT/'scripts'))
        from build_sdk_package import prepare
        sdk=ROOT/'.sn/sdk-native-integration'
        manifest=prepare(COMPILER,sdk,self.root/'.sn/sindarin-pkg-sdk',('core/uuid',))
        self.fixture.write('main.sn','import "sindarin-pkg-sdk/src/core/uuid"\n'
            'fn copied(value: UUID as val): UUID =>\n  value._high=9l\n  return value\n'
            'fn main(): void =>\n  var original: UUID = UUID.namespaceDns()\n'
            '  var copy: UUID = copied(original)\n'
            '  println(original.toString()=="6ba7b810-9dad-11d1-80b4-00c04fd430c8")\n'
            '  println(copy._high==9l)\n')
        for target in ('c','rust'):
            for opt in ('-O0','-O1','-O2'):
                for mode in ('default','checked','unchecked'):
                    exe=self.root/'uuid.exe';command=[str(COMPILER),'main.sn','--no-install','--target',target,opt,'-o',str(exe)]
                    if mode!='default':command.append('--'+mode)
                    build=subprocess.run(command,cwd=self.root,capture_output=True,timeout=180)
                    self.assertEqual(build.returncode,0,build.stderr.decode(errors='replace'))
                    run=subprocess.run([str(exe)],cwd=self.root,capture_output=True,timeout=30)
                    expected=b'true\ntrue\n'
                    if os.name=='nt':expected=expected.replace(b'\n',b'\r\n')
                    self.assertEqual((run.returncode,run.stdout,run.stderr),(0,expected,b''))

    def test_copy_requires_a_valid_prototype_export_and_requested_capability(self):
        f=self.fixture
        for target in ('c','rust'):
            f.write('.sn/methods/sn.yaml',self.manifest.replace('      copy: counter_copy\n',''))
            exe=self.root/('missing-'+target+'.exe')
            run=subprocess.run([str(COMPILER),'main.sn','--no-install','--target',target,'-o',str(exe)],cwd=self.root,capture_output=True,timeout=180)
            self.assertNotEqual(run.returncode,0);self.assertIn(b'as-val copy requires a native.types copy contract',run.stderr)
            self.assertFalse(exe.exists())
        f.write('.sn/methods/sn.yaml',self.manifest)
        header=self.package/'src/counter.h';old=header.read_text()
        header.write_text(old.replace('CanonicalCounter *counter_copy(const CanonicalCounter *);','int counter_copy(const CanonicalCounter *);'))
        run=subprocess.run([str(COMPILER),'--build-native',str(self.package/'sn.yaml'),'-o',str(self.package/'invalid')],cwd=self.root,capture_output=True,timeout=180)
        self.assertNotEqual(run.returncode,0);self.assertIn(b'record copy prototype differs',run.stderr)
        header.write_text(old)
        source=self.package/'src/counter.c';source.write_text(source.read_text().replace('CanonicalCounter *counter_copy(', 'CanonicalCounter *undeclared_copy('))
        run=subprocess.run([str(COMPILER),'--build-native',str(self.package/'sn.yaml'),'-o',str(self.package/'invalid')],cwd=self.root,capture_output=True,timeout=180)
        self.assertNotEqual(run.returncode,0);self.assertIn(b'lifecycle export missing',run.stderr)


if __name__=='__main__':unittest.main(verbosity=2)
