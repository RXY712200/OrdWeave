from pathlib import Path
import subprocess as sp,csv,json,statistics,platform,hashlib
import sys
R=Path(sys.argv[1]).resolve();O=Path(__file__).resolve().parent;lib=Path(sys.argv[2]).resolve()
for name in ('query','sort'):
 for resource in (False,True):
  cmd=['gcc','-O3','-std=c17','-Iinclude','-Ithird_party/LayerKeySort/include',str(O/f'{name}_prototype.c')]
  if resource:cmd += [str(O/'memory.c'),'-DRESOURCE','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free']
  cmd += [str(lib/'libordweave.a'),str(lib/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'{name}{"-resource" if resource else ""}.exe')];sp.run(cmd,check=True,cwd=R)
rows=[];resources=[]
for n in (128,8192):
 for buckets in (257,1):
  for updates in (0,100):
   for repeat in range(4):
    for indexed in ((0,1) if repeat%2==0 else (1,0)):
     args=[str(n),str(buckets),str(updates),str(indexed)];p=sp.run([str(O/'query.exe'),*args],capture_output=True,text=True,check=True)
     if repeat:rows.append([repeat,*p.stdout.strip().split(',')[1:]])
   for indexed in (0,1):
    if n==8192 and buckets==257:
     args=[str(n),str(buckets),str(updates),str(indexed)];p=sp.run([str(O/'query-resource.exe'),*args],capture_output=True,text=True,check=True);resources.append(dict(args=args,output=p.stdout))
with (O/'query-raw.csv').open('w',newline='') as f:w=csv.writer(f);w.writerow(['repeat','n','buckets','updates','indexed','setup','build','update','reads','results']);w.writerows(rows)
summary=[]
for n in (128,8192):
 for buckets in (257,1):
  for updates in (0,100):
   for indexed in (0,1):
    selected=[r for r in rows if list(map(int,r[1:5]))==[n,buckets,updates,indexed]]
    summary.append(dict(n=n,buckets=buckets,updates=updates,indexed=indexed,build=statistics.median(float(r[6]) for r in selected),update=statistics.median(float(r[7]) for r in selected),reads=statistics.median(float(r[8]) for r in selected)))
(O/'query-summary.json').write_text(json.dumps(summary,indent=2)+'\n');(O/'query-resources.json').write_text(json.dumps(resources,indent=2)+'\n')
rows=[];resources=[]
for n in (128,8192):
 for mode in range(4):
  for repeat in range(4):
   p=sp.run([str(O/'sort.exe'),str(n),str(mode)],capture_output=True,text=True,check=True)
   if repeat:rows.append([repeat,*p.stdout.strip().split(',')[1:]])
  p=sp.run([str(O/'sort-resource.exe'),str(n),str(mode)],capture_output=True,text=True,check=True);resources.append(dict(n=n,mode=mode,output=p.stdout))
with (O/'sort-raw.csv').open('w',newline='') as f:w=csv.writer(f);w.writerow(['repeat','n','mode','build','recurring64merges']);w.writerows(rows)
(O/'sort-resources.json').write_text(json.dumps(resources,indent=2)+'\n')
meta=dict(base='edb9675b10125cecdf936e030bd9a54a90a84146',pin='f751e1ee2afd682874281d6dc495969f9eca49eb',platform=platform.platform(),compiler=sp.check_output(['gcc','--version'],text=True),seed=17,notes='Query read span includes independent O(N) expected-result checking in both variants. Conservative end-to-end prototype cost, not pure lookup latency. Mixed maintenance is50 updates/50 deletions preceding reads, not interleaved traffic. Range correctness is outside timed reads.',binaries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in O.glob('*.exe')});(O/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n');print(json.dumps(summary,indent=2))
