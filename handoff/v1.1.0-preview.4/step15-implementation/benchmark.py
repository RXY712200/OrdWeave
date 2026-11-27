from pathlib import Path
import subprocess as sp,tarfile,io,csv,json,statistics,platform,hashlib
R=Path('D:/OrdWeave');O=R/'build/step15';src=O/'baseline';src.mkdir(exist_ok=True)
for repo,ref,dest in [(R,'cf342dcf881bc9e3518ede042baeaed25f807da7',src),(R/'third_party/LayerKeySort','f751e1ee2afd682874281d6dc495969f9eca49eb',src/'third_party/LayerKeySort')]:
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',ref]))) as t:t.extractall(dest,filter='data')
with (O/'bench-build.log').open('w') as log:
 for cmd in [['cmake','-S',str(src),'-B',str(src/'build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(src/'build'),'--target','ordweave','ordweave_history','--parallel','4'],['cmake','--build',str(R/'build/step15-release'),'--target','ordweave','--parallel','4']]:sp.run(cmd,check=True,stdout=log,stderr=sp.STDOUT)
for variant,bld in [('baseline',src/'build'),('candidate',R/'build/step15-release')]:
 for resource in (False,True):
  cmd=['gcc','-O3','-std=c17','-Iinclude',str(O/'production_bench.c')]
  if variant=='baseline':cmd+=['-DBASELINE']
  if resource:cmd += [str(R/'build/preview2/memory.c'),'-DRESOURCE','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free']
  cmd += [str(bld/'libordweave.a'),str(bld/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'{variant}{"-memory" if resource else ""}.exe')];sp.run(cmd,check=True)
rows=[];resources=[];cases=[(8192,257,0,0),(8192,257,100,0),(1024,17,64,1),(128,1,0,0)]
for case in cases:
 for repeat in range(4):
  modes=list(range(5));off=repeat%5;modes=modes[off:]+modes[:off]
  for mode in modes:
   exe=O/('baseline.exe' if mode==0 else 'candidate.exe');args=[str(x) for x in (*case,mode)];p=sp.run([str(exe),*args],capture_output=True,text=True,check=True)
   if repeat:rows.append([repeat,*p.stdout.strip().split(',')[1:]])
 for mode in (0,1,2,3,4):
  if case==cases[0]:
   exe=O/('baseline-memory.exe' if mode==0 else 'candidate-memory.exe');p=sp.run([str(exe),*map(str,(*case,mode))],capture_output=True,text=True,check=True);resources.append(dict(case=case,mode=mode,output=p.stdout))
with (O/'production-raw.csv').open('w',newline='') as f:w=csv.writer(f);w.writerow(['repeat','n','buckets','changes','interleaved','mode','setup','build','updates','reads','results']);w.writerows(rows)
summary=[]
for case in cases:
 for mode in range(5):
  selected=[r for r in rows if tuple(map(int,r[1:5]))==case and int(r[5])==mode]
  summary.append(dict(case=case,mode=mode,setup=statistics.median(float(r[6]) for r in selected),build=statistics.median(float(r[7]) for r in selected),updates=statistics.median(float(r[8]) for r in selected),reads=statistics.median(float(r[9]) for r in selected),total=statistics.median(sum(float(r[i]) for i in (7,8,9)) for r in selected)))
(O/'production-summary.json').write_text(json.dumps(summary,indent=2)+'\n');(O/'production-resources.json').write_text(json.dumps(resources,indent=2)+'\n');(O/'metadata.json').write_text(json.dumps(dict(base='cf342dcf881bc9e3518ede042baeaed25f807da7',platform=platform.platform(),compiler=sp.check_output(['gcc','--version'],text=True),binaries={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in O.glob('*.exe')}),indent=2)+'\n');print(json.dumps(summary,indent=2))
