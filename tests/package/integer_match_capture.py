#!/usr/bin/env python3
"""Force repeated compiler build IDs while exercising the real evidence checker."""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))


class IntegerMatchEvidence(unittest.TestCase):
    def test_reused_process_id_keeps_current_support(self):
        spec=importlib.util.spec_from_file_location('integer_match_capture_subject',Path(os.environ.get('SN_INTEGER_CAPTURE_SUBJECT',ROOT/'scripts/check_rust_integer_match_patterns.py')))
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='sn-integer-capture-regression-') as folder:
            root=Path(folder);(root/'bin').mkdir();(root/'bin'/('sn.exe' if module.os.name=='nt' else 'sn')).write_bytes(b'fake compiler')
            (root/'.sn').mkdir();(root/'scripts').mkdir()
            for name in ('check_rust_integer_match_patterns.py','integer_match_controls.py'):
                (root/'scripts'/name).write_bytes((ROOT/'scripts'/name).read_bytes())
            directories=[]
            def execute(command,cwd,**kwargs):
                if '--target' in command:
                    directories.append(str(cwd));target=command[command.index('--target')+1]
                    if target=='rust':
                        # The compiler deliberately reuses the same ID on every run.
                        support=Path(cwd)/'.sn/build/rust/main_7/sn_integer_matches.c'
                        support.parent.mkdir(parents=True,exist_ok=True);support.write_text('/* current C support */')
                    return subprocess.CompletedProcess(command,0,b'',b'')
                expected=b'true\r\n' if module.os.name=='nt' else b'true\n'
                return subprocess.CompletedProcess(command,0,expected,b'')
            with patch.object(module,'ROOT',root),patch.object(module,'ORIGINALS',[]),patch.object(module,'EXPECTED',b'true\n'),patch.object(module.subprocess,'run',side_effect=execute):
                module.verify()
            report=json.loads((root/'.sn/rust-integer-match-patterns.json').read_text())
            self.assertTrue(report['passed']);self.assertEqual(len(report['cases']),9)
            self.assertEqual(len(set(directories)),18)
            for case in report['cases']:
                self.assertEqual(case['targets']['rust']['C_boundary_helpers'],['/* current C support */'])


if __name__=='__main__':unittest.main()
