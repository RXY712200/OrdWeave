"""Run from repository root. No global state/configuration is changed."""
from pathlib import Path
import ctypes as C
import hashlib,json,subprocess as sp,time,shutil
R=Path.cwd();O=R/'build/reverse';P=json.loads((R/'research/lks-reverse-reliability/protocol.json').read_text())
class Counters(C.Structure):
    _fields_=[('cb',C.c_ulong),('faults',C.c_ulong)]+[(n,C.c_size_t) for n in ['peak_working','working','peak_paged','paged','peak_nonpaged','nonpaged','pagefile','peak_pagefile','private']]
k=C.windll.kernel32;k.OpenProcess.restype=C.c_void_p;k.CloseHandle.argtypes=[C.c_void_p]
ps=C.windll.psapi.GetProcessMemoryInfo;ps.argtypes=[C.c_void_p,C.c_void_p,C.c_ulong]
results=json.loads((O/"cases.json").read_text()) if (O/"cases.json").exists() else []
def run(label,args,cap):
    assert shutil.disk_usage(R).free>P['resource']['disk_floor']
    start=time.monotonic();peak=working=0;reason='';log=O/(label+'.log')
    with log.open('wb') as f:
        p=sp.Popen(args,stdout=f,stderr=sp.STDOUT,cwd=R);h=k.OpenProcess(0x410,False,p.pid)
        try:
            while p.poll() is None:
                c=Counters();c.cb=C.sizeof(c)
                if h and ps(h,C.byref(c),C.sizeof(c)):peak=max(peak,c.private);working=max(working,c.working)
                if peak>P['resource']['private_cap'] or time.monotonic()-start>cap:
                    reason='RESOURCE_OR_TIMEOUT';p.kill();break
                time.sleep(.1)
            code=p.wait()
        finally:
            if h:k.CloseHandle(h)
    item=dict(case=label,command=list(map(str,args)),code=code,result='PASS' if code==0 else reason or 'FAIL',seconds=time.monotonic()-start,peak_private_bytes=peak,peak_sampled_working_set=working,log_sha256=hashlib.sha256(log.read_bytes()).hexdigest())
    trace=O/(label+'.trace')
    if trace.exists():item.update(trace_bytes=trace.stat().st_size,trace_sha256=hashlib.file_digest(trace.open('rb'),'sha256').hexdigest())
    results.append(item);(O/'cases.json').write_text(json.dumps(results,indent=2)+'\n');print(json.dumps(item),flush=True)
    return code==0
def main():
    exe=str(O/'driver.exe')
    assert run('pilot',[exe,'memory','17','128','balanced',str(O/'pilot.trace'),'0'],600)
    for nr,no,nc in P['capacity']:
        if not run(f'capacity-{nr}-{no}-{nc}',[exe,'capacity',str(nr),str(no),str(nc)],900):break
    for c in P['memory']:
        label=f"memory-{c['seed']}";run(label,[exe,'memory',str(c['seed']),str(c['steps']),c['mix'],str(O/(label+'.trace')),'0'],600)
    for c in P['focused']:
        label=f"pattern-{c['pattern']}";run(label,[exe,'memory',str(c['seed']),str(c['steps']),'balanced',str(O/(label+'.trace')),str(c['pattern'])],600)
    run('queries',[exe,'query'],600)
    for nr,no,nc,cycles in P['persistence']:
        label=f'persistence-{cycles}';run(label,[exe,'persistence',str(O/(label+'-db')),str(nr),str(no),str(nc),str(cycles)],900)
    print('DONE',len(results),'cases',flush=True)

if __name__=="__main__":main()
