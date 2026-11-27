from pathlib import Path
import subprocess as sp,tarfile,io,csv,json,statistics
R=Path('D:/OrdWeave');O=R/'build/preview3';src=O/'base';src.mkdir(exist_ok=True)
for repo,ref,dest in [(R,'2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8',src),(R/'third_party/LayerKeySort','HEAD',src/'third_party/LayerKeySort')]:
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',ref]))) as t:t.extractall(dest,filter='data')
with (O/'build-base.log').open('w') as log:
 for cmd in [['cmake','-S',str(src),'-B',str(src/'build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(src/'build'),'--target','ordweave','ordweave_history','--parallel','4'],['cmake','--build',str(R/'build/preview3-release'),'--target','ordweave','ordweave_history','--parallel','4']]:sp.run(cmd,check=True,stdout=log,stderr=sp.STDOUT)
for name,b in [('base',src/'build'),('candidate',R/'build/preview3-release')]:
 sp.run(['gcc','-O3','-std=c17','-Iinclude','-Isrc',str(O/'import_bench.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'import-{name}.exe')],check=True)
rows=[]
for n,c,f in [(128,1,6),(8192,1,6),(8192,8,6),(2048,8,64)]:
 for repeat in range(4):
  for name in (('base','candidate') if repeat%2==0 else ('candidate','base')):
   p=sp.run([str(O/f'import-{name}.exe'),str(n),str(c),str(f)],capture_output=True,text=True,check=True)
   if repeat:rows.append([name,repeat,*p.stdout.strip().split(',')])
with (O/'import-raw.csv').open('w',newline='') as f:
 w=csv.writer(f);w.writerow(['variant','repeat','n','collections','fields','setup','export','import','bytes']);w.writerows(rows)
summary=[]
for n,c,f in [(128,1,6),(8192,1,6),(8192,8,6),(2048,8,64)]:
 a=statistics.median(float(r[7]) for r in rows if r[0]=='base' and int(r[2])==n and int(r[3])==c and int(r[4])==f);b=statistics.median(float(r[7]) for r in rows if r[0]=='candidate' and int(r[2])==n and int(r[3])==c and int(r[4])==f);summary.append(dict(n=n,collections=c,fields=f,base=a,candidate=b,improvement=1-b/a))
(O/'import-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
