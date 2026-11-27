"""Independent regeneration of primary canonical LE uint64 trace rows.
Does not query OrdWeave, load LKS, or use subject results as ground truth.
"""
from pathlib import Path
import hashlib,json,struct
O=Path('build/reverse');cases=json.loads((O/'cases.json').read_text());out=[]
for case in cases:
 if 'trace_sha256' not in case:continue
 _,_,seed,steps,mix,_,pattern=case['command'];seed=int(seed);steps=int(steps);pattern=int(pattern);state=seed;n=32 if pattern==3 else 1024;h=hashlib.sha256()
 def rand():
  global state
  state^=(state<<13)&0xffffffff;state^=state>>17;state^=(state<<5)&0xffffffff;state&=0xffffffff
  return state
 for step in range(steps):
  x=rand()%100;i=rand()%n;j=rand()%n
  thresholds={'balanced':[25,50,75,85],'read':[10,20,35,45],'modify':[35,70,90,95],'move':[5,10,80,90]}[mix]
  kind=next((k for k,t in enumerate(thresholds) if x<t),4)
  if pattern:kind=0 if step%(128 if pattern==3 else 256)<(64 if pattern==3 else 128) else 1;i=n//2;j=n//2 if pattern==1 else n-1
  if n<(2 if pattern==3 else 512):kind=0
  if n>(256 if pattern==3 else 1536):kind=1
  p=rand()%4;h.update(struct.pack('<QQQQQ',step,kind,i,j,p))
  n+=1 if kind==0 else -1 if kind==1 else 0
 assert h.hexdigest()==case['trace_sha256'],case['case']
 out.append(dict(case=case['case'],sha256=h.hexdigest(),rows=steps,verified_independent_regeneration=True))
(O/'regenerated-traces.json').write_text(json.dumps(out,indent=2)+'\n',newline='\n');print('Independent generator verified',len(out),'trace hashes')
