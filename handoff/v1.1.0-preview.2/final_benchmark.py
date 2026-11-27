from pathlib import Path
import subprocess as sp,tarfile,io,json,csv,statistics,random,hashlib
R=Path('D:/OrdWeave');O=R/'build/preview2';T=O/'traces';T.mkdir(exist_ok=True)
for n in (128,2048):
 p=R/f'build/preview1-evidence/traces/db-{n}-17-update-heavy.trace';(T/p.name).write_bytes(p.read_bytes())
for n,scenario in [(8192,'update-heavy'),(128,'insert-remove-heavy'),(8192,'insert-remove-heavy')]:
 rng=random.Random(17);seq=list(range(n));nextid=n;ops=[]
 for i in range(1000):
  kind=rng.choices('CMIR',weights=[10,60,15,15] if scenario=='update-heavy' else [5,5,45,45])[0]
  ai=rng.randrange(len(seq));bi=rng.randrange(len(seq));a,b=seq[ai],seq[bi];after=rng.randrange(2);cmp=0
  if kind=='C':cmp=(ai>bi)-(ai<bi)
  elif kind=='I':a=nextid;nextid+=1;seq.insert(bi+after,a)
  elif kind=='R':seq.pop(ai)
  else:
   if a==b:kind='C';cmp=0
   else:seq.pop(ai);seq.insert(seq.index(b)+after,a)
  ops.append(f'{kind} {a} {b} {after} {cmp}\n')
 (T/f'db-{n}-17-{scenario}.trace').write_bytes((f'{n} 1000 {len(seq)}\n'+''.join(ops)+' '.join(map(str,seq))+'\n').encode())
driver=(R/'build/preview1-evidence/A/driver.c').read_text().replace('n<=2048','n<=8192')
meta={'base':'3c5f371ca53c62912dc3d4d63aa11dc5277af020','traces':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in T.iterdir()},'compiler':sp.check_output(['gcc','--version'],text=True)}
for variant in ('base','candidate'):
 src=O/variant;src.mkdir(exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','archive',meta['base']],cwd=R))) as a:a.extractall(src,filter='data')
 dep=src/'third_party/LayerKeySort';dep.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','archive','HEAD'],cwd=R/'third_party/LayerKeySort'))) as a:a.extractall(dep,filter='data')
 if variant=='candidate':
  for p in (R/'src').iterdir():
   if p.is_file():(src/'src'/p.name).write_bytes(p.read_bytes())
 (src/'driver.c').write_text(driver)
 with (src/'CMakeLists.txt').open('a') as f:f.write('\nadd_executable(preview2_driver driver.c)\ntarget_link_libraries(preview2_driver PRIVATE ordweave bcrypt)\n')
 with (O/f'bench-build-{variant}.log').open('w') as log:
  for cmd in [['cmake','-S',str(src),'-B',str(src/'build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(src/'build'),'--target','preview2_driver','--parallel','4']]:sp.run(cmd,check=True,stdout=log,stderr=sp.STDOUT)
rows=[]
with (O/'core-receipts.jsonl').open('w') as receipt:
 for trace in sorted(T.iterdir()):
  for repeat in range(4):
   for variant in (('base','candidate') if repeat%2==0 else ('candidate','base')):
    p=sp.run([str(O/variant/'build/preview2_driver.exe'),str(trace),'memory'],capture_output=True,text=True,check=True)
    assert sum(s.startswith('receipt,') for s in p.stdout.splitlines())==1000
    receipt.write(json.dumps(dict(trace=trace.name,variant=variant,repeat=repeat,code=p.returncode,stdout=p.stdout))+'\n')
    fields,=[s.split(',')[1:] for s in p.stdout.splitlines() if s.startswith('summary,')]
    if repeat:rows.append([trace.name,variant,repeat,*fields])
with (O/'core-raw.csv').open('w',newline='') as f:
 w=csv.writer(f);w.writerow(['trace','variant','repeat','setup','operations','checkpoint','reopen','history','n','steps']);w.writerows(rows)
summary=[]
for trace in sorted(T.iterdir()):
 a=statistics.median(float(r[4]) for r in rows if r[0]==trace.name and r[1]=='base');b=statistics.median(float(r[4]) for r in rows if r[0]==trace.name and r[1]=='candidate');summary.append(dict(trace=trace.name,base=a,candidate=b,speedup=a/b))
(O/'core-summary.json').write_text(json.dumps(summary,indent=2)+'\n');(O/'core-metadata.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(summary,indent=2))
