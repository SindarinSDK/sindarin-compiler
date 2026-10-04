import hashlib, json, os, pathlib, subprocess, sys
root=pathlib.Path.cwd()
sys.path.insert(0, 'scripts')
from check_rust_native_handle_oracles import FIXTURES
sources=list(FIXTURES)
asan=subprocess.check_output(['gcc','-print-file-name=libasan.so'],text=True).strip()
build_env=os.environ.copy()
build_env['SN_DEBUG_CFLAGS']='-fwrapv -no-pie -fsanitize=address,undefined -fno-omit-frame-pointer -g -Werror=implicit-function-declaration'
run_env=os.environ.copy();run_env.update(LD_PRELOAD=asan,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
report={'compiler_sha256':hashlib.sha256(pathlib.Path('bin/sn').read_bytes()).hexdigest(),'debug_flags':build_env['SN_DEBUG_CFLAGS'],'runtime_environment':{k:run_env[k] for k in ('LD_PRELOAD','ASAN_OPTIONS','UBSAN_OPTIONS')},'cases':[]}
for source in sources:
 for optimization in ['-O0','-O1','-O2']:
  for mode in ['default','checked','unchecked']:
   for target in ['c','rust']:
    exe=root/'.sn'/('native-handles-instrumented-'+target)
    cmd=['bin/sn',source,'--no-install','--target',target,'--keep-generated','-g',optimization,*([] if mode=='default' else ['--'+mode]),'-o',str(exe)]
    proc=subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,env=build_env)
    output,error=proc.communicate(timeout=120)
    pathlib.Path('.sn/native-handles-asan-last-compile.log').write_bytes(output+error)
    assert proc.returncode==0,(source,optimization,mode,target,output,error)
    symbols=subprocess.check_output(['nm',str(exe)],text=True)
    assert '__asan_init' in symbols and '__ubsan_handle' in symbols,(source,target,'missing instrumentation')
    objects=[]
    if target=='rust':
     directory=root/'.sn/build/rust'/(pathlib.Path(source).stem+'_'+str(proc.pid))
     for obj in sorted(directory.glob('*.o')):
      sym=subprocess.check_output(['nm','-u',str(obj)],text=True)
      assert '__asan_init' in sym,(str(obj),'missing C instrumentation')
      objects.append({'name':obj.name,'sha256':hashlib.sha256(obj.read_bytes()).hexdigest(),'asan':True,'ubsan':'__ubsan_handle' in sym})
     assert any(o['name']=='sn_native_handles.o' and o['ubsan'] for o in objects),objects
    result=subprocess.run([str(exe)],capture_output=True,env=run_env,timeout=30)
    expected=pathlib.Path(source).with_suffix('.expected').read_bytes()
    case={'source':source,'optimization':optimization,'arithmetic_mode':mode,'target':target,'status':result.returncode,'stdout_hex':result.stdout.hex(),'stderr_hex':result.stderr.hex(),'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'objects':objects,'passed':result.returncode==0 and result.stdout==expected and not result.stderr}
    report['cases'].append(case)
    pathlib.Path('.sn/native-handles-asan.json').write_text(json.dumps(report,indent=2)+'\n')
    assert case['passed'],case
   print(source,optimization,mode,'both instrumented targets pass',flush=True)
assert len(report['cases'])==234
report['passed']=True
pathlib.Path('.sn/native-handles-asan.json').write_text(json.dumps(report,indent=2)+'\n')
print('234 clean ASAN/UBSAN/leak controls',flush=True)
