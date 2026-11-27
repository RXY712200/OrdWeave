from pathlib import Path
import subprocess as sp,tarfile,io,json,csv,statistics,hashlib,platform,math
R=Path('D:/OrdWeave');O=R/'build/rc1';T=O/'traces';T.mkdir(exist_ok=True)
stable='32f853defc0a2e9f0ca809fb3535a90c977c7665';pins={'A':'70d7f0ef5857b54381b863d292a213909d87459f','B':'f751e1ee2afd682874281d6dc495969f9eca49eb','C':'f751e1ee2afd682874281d6dc495969f9eca49eb'};builds={};sources={}
for n in (128,2048):
 p=R/f'build/preview1-evidence/traces/db-{n}-17-update-heavy.trace';(T/p.name).write_bytes(p.read_bytes())
for variant in ('A','B'):
 src=O/variant;src.mkdir(exist_ok=True)
 for repo,ref,dest in [(R,stable,src),(R/'third_party/LayerKeySort',pins[variant],src/'third_party/LayerKeySort')]:
  dest.mkdir(parents=True,exist_ok=True)
  with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',ref]))) as a:a.extractall(dest,filter='data')
 builds[variant]=src/'build';sources[variant]={str(p.relative_to(src)):hashlib.sha256(p.read_bytes()).hexdigest() for folder in ('src','include') for p in (src/folder).rglob('*') if p.is_file()}
 with (O/f'build-{variant}.log').open('w') as log:
  sp.run(['cmake','-S',str(src),'-B',str(builds[variant]),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True,stdout=log,stderr=sp.STDOUT)
  sp.run(['cmake','--build',str(builds[variant]),'--target','ordweave','--parallel','4'],check=True,stdout=log,stderr=sp.STDOUT)
assert sources['A']==sources['B']
builds['C']=R/'build/rc1-release'
for variant,b in builds.items():
 for driver in (['common'] if variant!='C' else ['common','index']):
  sp.run(['gcc','-O3','-std=c17','-Iinclude',str(O/f'{driver}_driver.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'{driver}-{variant}.exe')],check=True)
rows=[];receipts=[]
for n,mode in ((128,'memory'),(2048,'memory'),(128,'persistent')):
 trace=T/f'db-{n}-17-update-heavy.trace'
 for repeat in range(4):
  variants=list('ABC');offset=repeat%3;variants=variants[offset:]+variants[:offset]
  for variant in variants:
   args=[str(O/f'common-{variant}.exe'),str(trace),mode]
   if mode=='persistent':args+=[str(O/f'db-{n}-{mode}-{repeat}-{variant}')]
   p=sp.run(args,capture_output=True,text=True,check=True,timeout=90)
   receipt=[x for x in p.stdout.splitlines() if x.startswith('receipt,')];assert len(receipt)==1000
   receipts.append(dict(n=n,mode=mode,variant=variant,repeat=repeat,status=0,receipts_sha256=hashlib.sha256(('\n'.join(receipt)+'\n').encode()).hexdigest()))
   data,=[x.split(',')[1:] for x in p.stdout.splitlines() if x.startswith('summary,')]
   if repeat:rows.append([n,mode,variant,repeat,*data])
with (O/'cumulative-raw.csv').open('w',newline='') as f:w=csv.writer(f);w.writerow(['fixture_n','mode','variant','repeat','setup','operations','checkpoint','reopen','history','final_n','steps']);w.writerows(rows)
summary=[]
for n,mode in ((128,'memory'),(2048,'memory'),(128,'persistent')):
 for metric,k in [('setup',4),('operations',5),('checkpoint',6),('reopen',7),('history',8)]:
  vals={v:statistics.median(float(r[k]) for r in rows if r[:3]==[n,mode,v]) for v in 'ABC'}
  if metric not in ('setup','operations') and mode=='memory':continue
  summary.append(dict(n=n,mode=mode,metric=metric,**vals,A_to_B=vals['A']/vals['B'],B_to_C=vals['B']/vals['C'],A_to_C=vals['A']/vals['C']))
(O/'cumulative-summary.json').write_text(json.dumps(summary,indent=2)+'\n');(O/'receipts.json').write_text(json.dumps(receipts,indent=2)+'\n')
index=[];occupancy={}
for n,b,changes,interleaved in ((8192,257,0,0),(1024,19,64,1)):
 assert math.gcd(17,b)==1
 counts=[0]*b
 for i in range(n):counts[(i*17+17)%b]+=1
 occupancy[f'{n}/{b}']=dict(buckets=b,nonempty=sum(x>0 for x in counts),minimum=min(counts),maximum=max(counts),histogram=counts)
 for repeat in range(4):
  modes=[1,2] if repeat%2==0 else [2,1]
  for mode in modes:
   p=sp.run([str(O/'index-C.exe'),*map(str,(n,b,changes,interleaved,mode))],capture_output=True,text=True,check=True,timeout=30)
   if repeat:index.append([repeat,*p.stdout.strip().split(',')[1:]])
with (O/'index-raw.csv').open('w',newline='') as f:w=csv.writer(f);w.writerow(['repeat','n','buckets','changes','interleaved','mode','setup','build','updates','reads','results']);w.writerows(index)
idx=[]
for n in (8192,1024):
 for mode in (1,2):
  sel=[r for r in index if int(r[1])==n and int(r[5])==mode];idx.append(dict(n=n,mode=mode,setup=statistics.median(float(r[6]) for r in sel),build=statistics.median(float(r[7]) for r in sel),updates=statistics.median(float(r[8]) for r in sel),reads=statistics.median(float(r[9]) for r in sel),total=statistics.median(sum(float(r[k]) for k in (7,8,9)) for r in sel)))
(O/'index-summary.json').write_text(json.dumps(idx,indent=2)+'\n');(O/'occupancy.json').write_text(json.dumps(occupancy,indent=2)+'\n')
meta=dict(A_source=stable,B_source=stable,C_parent='5ab01df34bcb62a9feb1c61a443a1f08c0a56a06',C_delta='only version literal1.1.0-preview.4 to1.1.0-rc.1',pins=pins,A_B_runtime_hashes=sources['A'],traces={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in T.iterdir()},platform=platform.platform(),compiler=sp.check_output(['gcc','--version'],text=True),binaries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in O.glob('*.exe')})
(O/'campaign-metadata.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(summary,indent=2));print(json.dumps(idx,indent=2))
