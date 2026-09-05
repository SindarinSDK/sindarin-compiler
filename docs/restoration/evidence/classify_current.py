import importlib.util,json,os,sys
from pathlib import Path
label,fixture,compiler=sys.argv[1:]
root=Path('/tmp/sn-restore-tagged-c-evidence')
os.chdir(fixture)
spec=importlib.util.spec_from_file_location('current_runner',str(Path(fixture)/'scripts/run_tests.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);m.Colors.disable()
results=[]
original=m.TestRunner._run_single_test
def record(self,info):
 r=original(self,info);results.append({'path':info['test_file'],'suite':info['test_type'],**r});return r
m.TestRunner._run_single_test=record
with m.TestRunner(compiler,60,30,[],False,os.cpu_count()) as runner:
 for suite in ['cgen','mgen','integration','integration-errors','explore','explore-errors']:
  runner.run_sn_tests(suite)
(root/f'{label}-current-c-results.json').write_text(json.dumps(results,indent=2)+'\n')
