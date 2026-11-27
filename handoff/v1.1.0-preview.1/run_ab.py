from pathlib import Path
OUT=Path(__file__).resolve().parent
import subprocess as sp,json,csv,statistics,shutil
meta=json.loads((OUT/'metadata.json').read_text())
rows=[]
with (OUT/'receipts.jsonl').open('w',encoding='utf-8') as receipts:
 for case in meta['traces']:
  for mode in ('memory','persistent'):
   for repeat in range(4):
    for variant in (('A','B') if repeat%2==0 else ('B','A')):
     sandbox=OUT/f"sandbox-{case['n']}-{mode}-{repeat}-{variant}"
     argv=[str(OUT/variant/'build/preview1_driver.exe'),str(OUT/'traces'/case['name']),mode]
     if mode=='persistent':sandbox.mkdir();argv.append(str(sandbox/'data.ow'))
     p=sp.run(argv,capture_output=True,text=True,timeout=240)
     receipts.write(json.dumps(dict(n=case['n'],mode=mode,repeat=repeat,variant=variant,code=p.returncode,stdout=p.stdout,stderr=p.stderr))+'\n');receipts.flush()
     p.check_returncode();assert sum(x.startswith('receipt,') for x in p.stdout.splitlines())==1000
     fields,=[x.split(',')[1:] for x in p.stdout.splitlines() if x.startswith('summary,')]
     if repeat:rows.append([variant,case['n'],mode,repeat,*fields[:5],fields[-1]])
     if sandbox.exists():
      assert sandbox.resolve().parent==OUT.resolve() and sandbox.name.startswith('sandbox-');shutil.rmtree(sandbox)
     print(variant,case['n'],mode,repeat,flush=True)
with (OUT/'raw.csv').open('w',newline='') as f:
 w=csv.writer(f);w.writerow(['variant','n','mode','repeat','setup','operations','checkpoint','reopen','history','steps']);w.writerows(rows)
summary=[]
for n in (128,2048):
 for mode in ('memory','persistent'):
  for i,metric in enumerate(('setup','operations','checkpoint','reopen','history'),4):
   a=statistics.median(float(r[i]) for r in rows if r[0]=='A' and r[1]==n and r[2]==mode)
   b=statistics.median(float(r[i]) for r in rows if r[0]=='B' and r[1]==n and r[2]==mode)
   summary.append(dict(n=n,mode=mode,metric=metric,A=a,B=b,ratio=b/a if a else None,flag=(b-a>(.001 if mode=='memory' else .010) and b>a*(1.15 if mode=='memory' else 1.25))))
(OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
