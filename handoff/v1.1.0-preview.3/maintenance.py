from pathlib import Path
import subprocess as sp,shutil,csv,json,statistics
R=Path('D:/OrdWeave');O=R/'build/preview3';builds={'base':O/'base/build','candidate':R/'build/preview3-release'}
for name,b in builds.items():
 sp.run(['cmake','--build',str(b),'--target','ordweave','ordweave_history','--parallel','4'],stdout=sp.DEVNULL,check=True)
 sp.run(['gcc','-O3','-std=c17','-Iinclude',str(O/'maintenance_bench.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'maintenance-{name}.exe')],check=True)
fixtures={}
for n in (128,8192):
 p=O/f'genesis-{n}';sp.run([str(O/'maintenance-base.exe'),'create',str(p),str(n)],check=True);fixtures[f'genesis-{n}']=p
p=O/'retained';sp.run([str(builds['base']/'ordweave_history.exe'),'fixture',str(p)],check=True);fixtures['retained']=p
rows=[]
for label,fixture in fixtures.items():
 for repeat in range(4):
  for name in (('base','candidate') if repeat%2==0 else ('candidate','base')):
   case=O/f'case-{label}-{repeat}-{name}';shutil.copytree(fixture,case)
   backup=O/f'backup-{label}-{repeat}-{name}';p=sp.run([str(O/f'maintenance-{name}.exe'),'run',str(case),str(backup)],capture_output=True,text=True,check=True,timeout=180)
   if repeat:rows.append([label,name,repeat,*p.stdout.strip().split(',')])
with (O/'maintenance-raw.csv').open('w',newline='') as f:
 w=csv.writer(f);w.writerow(['fixture','variant','repeat','open','checkpoint','history','verify','backup']);w.writerows(rows)
summary=[]
for label in fixtures:
 for i,metric in enumerate(['open','checkpoint','history','verify','backup'],3):
  a=statistics.median(float(r[i]) for r in rows if r[0]==label and r[1]=='base');b=statistics.median(float(r[i]) for r in rows if r[0]==label and r[1]=='candidate');summary.append(dict(fixture=label,metric=metric,base=a,candidate=b,ratio=b/a))
(O/'maintenance-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
