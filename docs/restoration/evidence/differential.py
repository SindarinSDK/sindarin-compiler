import concurrent.futures,hashlib,json,os,re,subprocess
from pathlib import Path
ROOT=Path('/tmp/sn-restore-tagged-c-evidence')
REPO=Path('/home/gavin/code/sindarin/sindarin-restore-tagged-c')
BASE=REPO.with_name('sindarin-restore-tagged-c-baseline')
FIX=ROOT/'tagged-corrected/fixtures'
OUT=ROOT/'differential';OUT.mkdir(exist_ok=True)
paths=subprocess.check_output(['git','ls-tree','-r','--name-only','v0.0.83','--','tests'],cwd=REPO).decode().splitlines()
paths=[p for p in paths if p.endswith('.sn')]
settings=[('tag',BASE,BASE/'bin/sn'),('correction',FIX,REPO/'bin/sn')]
for label,cwd,compiler in settings:(OUT/label/'tmp').mkdir(parents=True,exist_ok=True)
def digest(b):return hashlib.sha256(b).hexdigest()
def one(path):
 results={}
 for mode,suffix in [('--emit-c','.c'),('--emit-model','.json')]:
  pair=[]
  for label,cwd,compiler in settings:
   output=OUT/label/(path+suffix);output.parent.mkdir(parents=True,exist_ok=True)
   env=os.environ.copy();env['TMPDIR']=str(OUT/label/'tmp')
   cmd=['timeout','60',str(compiler),path,mode,'-O0','--no-install','-o',str(output),'-l','1']
   p=subprocess.run(cmd,cwd=cwd,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=65)
   def normalize(b):
    b=b.replace(str(output).encode(),b'<OUTPUT>').replace(str(compiler.parent).encode(),b'<COMPILER_DIR>')
    return re.sub(rb'\x1b\[[0-9;]*m',b'',b)
   stdout=normalize(p.stdout);stderr=normalize(p.stderr)
   output.with_suffix(output.suffix+'.stderr').write_bytes(p.stderr)
   output.with_suffix(output.suffix+'.stdout').write_bytes(p.stdout)
   data=output.read_bytes() if output.exists() else b''
   pair.append({'exit':p.returncode,'stdout':digest(stdout),'stderr':digest(stderr),'artifact':digest(data),'artifact_present':output.exists()})
  results[mode]={'tag':pair[0],'correction':pair[1],'equal':pair[0]==pair[1]}
 return {'path':path,'modes':results,'equal':all(x['equal'] for x in results.values())}
with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool: results=list(pool.map(one,paths))
summary={'fixture_commit':subprocess.check_output(['git','rev-parse','v0.0.83^{}'],cwd=REPO).decode().strip(),'compiler_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=REPO).decode().strip(),'source_count':len(results),'equal':sum(x['equal'] for x in results),'different':sum(not x['equal'] for x in results),'results':results}
(OUT/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k!='results'}))
for x in results:
 if not x['equal']:print(json.dumps(x))
