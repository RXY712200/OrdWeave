"""Generated database API scenarios, not captured human interactions.

Uses the preserved project's PCG32 (Melissa O'Neill Apache-2.0 adaptation),
with an independent V0.5 database stream namespace and explicit bounds.
"""
import hashlib
from pathlib import Path
from workload import PCG32

def trace_database(n,seed,scenario,path,steps=1000):
    assert n in (128,512,2048) and steps==1000
    assert seed in (17,305419896,2654435769)
    weights={'compare-heavy':(90,6,2,2),'update-heavy':(10,60,15,15),'hotspot':(20,20,40,20)}[scenario]
    def stream(purpose):
        h=hashlib.sha256(f'ist-v05-db|{n}|{seed}|{scenario}|{purpose}'.encode()).digest()
        return PCG32(int.from_bytes(h[:8],'big'),int.from_bytes(h[8:16],'big'))
    kinds,targets,anchors,directions=[stream(x) for x in ('kind','target','anchor','direction')]
    seq=list(range(n));next_id=n;ops=[];hotspot=n//2
    for _ in range(steps):
        roll=kinds.bounded(100);cumulative=0
        for kind,w in zip('CMIR',weights):
            cumulative+=w
            if roll<cumulative:break
        i=targets.bounded(len(seq));j=anchors.bounded(len(seq));after=directions.bounded(2)
        if scenario=='hotspot':
            if kind in 'MI':j=seq.index(hotspot)
            if kind=='R':
                while seq[i]==hotspot:i=targets.bounded(len(seq))
        if kind=='M':
            while i==j or j+after-(i<j+after)==i:i=targets.bounded(len(seq))
        a,b=seq[i],seq[j];expected=0
        if kind=='C':expected=(i>j)-(i<j)
        elif kind=='I':a=next_id;next_id+=1;seq.insert(j+after,a)
        elif kind=='R':seq.pop(i)
        else:seq.pop(i);seq.insert(seq.index(b)+after,a)
        ops.append(f'{kind} {a} {b} {after} {expected}\n')
    data=(f'{n} {steps} {len(seq)}\n'+''.join(ops)+' '.join(map(str,seq))+'\n').encode('ascii');Path(path).write_bytes(data)
    return dict(n=n,seed=seed,scenario=scenario,operations=steps,final_n=len(seq),sha256=hashlib.sha256(data).hexdigest())
