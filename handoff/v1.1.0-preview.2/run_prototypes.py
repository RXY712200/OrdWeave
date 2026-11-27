from pathlib import Path
import subprocess as sp,csv,json,statistics
R=Path('D:/OrdWeave');O=R/'build/preview2'
base=['gcc','-std=c17','-O3','-DNDEBUG','-I'+str(R/'include'),'-I'+str(R/'src'),'-I'+str(R/'third_party/LayerKeySort/include'),str(O/'prototype.c'),str(R/'src/lks_adapter.c'),str(R/'build/preview1-release/third_party/LayerKeySort/liblayerkeysort.a')]
for resource in (False,True):
 cmd=base+([str(O/'memory.c'),'-DRESOURCE','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free'] if resource else [])+['-o',str(O/('prototype-memory.exe' if resource else 'prototype.exe'))];sp.run(cmd,check=True)
rows=[];resources=[]
for n in (128,2048,8192):
 for repeat in range(4):
  kinds='ACD';offset=repeat%3;kinds=kinds[offset:]+kinds[:offset]
  for kind in kinds:
   p=sp.run([str(O/'prototype.exe'),kind,str(n),'0'],capture_output=True,text=True,check=True,timeout=90)
   fields=p.stdout.strip().split(',');assert fields[0]=='result'
   if repeat:rows.append([kind,n,repeat,*map(float,fields[-2:])])
 for kind in 'ACD':
  for sparse in (0,1):
   # Sparse uses 128 live keys; wide gaps, no high-water-sized allocation.
   if sparse and n!=128:continue
   p=sp.run([str(O/'prototype-memory.exe'),kind,str(n),str(sparse)],capture_output=True,text=True,check=True,timeout=90)
   resources.append(dict(candidate=kind,n=n,sparse=sparse,stdout=p.stdout))
with (O/'prototype-raw.csv').open('w',newline='') as f:
 w=csv.writer(f);w.writerow(['candidate','n','repeat','setup_seconds','operations_seconds']);w.writerows(rows)
(O/'prototype-resources.json').write_text(json.dumps(resources,indent=2)+'\n')
summary=[dict(candidate=k,n=n,setup=statistics.median(r[3] for r in rows if r[0]==k and r[1]==n),operations=statistics.median(r[4] for r in rows if r[0]==k and r[1]==n)) for n in (128,2048,8192) for k in 'ACD']
(O/'prototype-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
