"""One admitted 4k native case and the archived upstream allocator sweep."""
import ctypes, gzip, hashlib, json, shutil, subprocess, sys
from pathlib import Path

OW, ROOT, DIAG, OUT = (Path(x).resolve() for x in sys.argv[1:5])
OUT.mkdir()
final=ROOT/'final'
original=ROOT/'capture'
fixtures=OUT/'larger.trace'
n, steps=4096,192
expected=list(range(n)); rows=[]; new=n
for i in range(steps):
    anchor=expected[(i*911+101)%len(expected)]
    if i%3==0:
        rows.append(f'I {new} {anchor} 0 0')
        expected.insert(expected.index(anchor),new)
        new+=1
    elif i%3==1:
        old=expected[(i*433+11)%len(expected)]
        rows.append(f'R {old} 0 0 0')
        expected.remove(old)
    else:
        old=expected[(expected.index(anchor)+17)%len(expected)]
        rows.append(f'M {old} {anchor} 1 0')
        expected.remove(old)
        expected.insert(expected.index(anchor)+1,old)
fixtures.write_text('\n'.join([f'{n} {steps} {len(expected)}',*rows,*map(str,expected)])+'\n',encoding='ascii')
driver=OUT/'large_driver.c'
source=(final/'candidate/driver.c').read_text()
assert source.count('n<=2048')==1
driver.write_text(source.replace('n<=2048','n<=4096'),encoding='utf-8')
class Memory(ctypes.Structure):
    _fields_=[('length',ctypes.c_ulong),('load',ctypes.c_ulong)]+[(s,ctypes.c_ulonglong) for s in ('total','avail','page','pageavail','virtual','virtualavail','ext')]
with gzip.open(OUT/'extra-receipts.jsonl.gz','wt',encoding='utf-8') as stream:
    def run(*command):
        argv=list(map(str,command))
        memory=Memory()
        memory.length=ctypes.sizeof(memory)
        if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):raise OSError
        record=dict(argv=argv,available=memory.avail)
        if memory.avail<640*1024**2 or shutil.disk_usage(OUT).free<512*1024**2:
            stream.write(json.dumps(dict(record,status='skipped-resource'))+'\n')
            return None
        p=subprocess.run(argv,capture_output=True,text=True,timeout=300)
        stream.write(json.dumps(dict(record,code=p.returncode,stdout=p.stdout,stderr=p.stderr))+'\n')
        stream.flush()
        p.check_returncode()
        return p.stdout
    for kind,root in (('stable41',original),('preview12',original),('candidate',final)):
        src=root/kind
        b=src/'build'
        exe=OUT/(kind+'.exe')
        if run('gcc','-std=c17','-O3','-DNDEBUG','-I'+str(src/'include'),driver,b/'libordweave.a',
               b/'third_party/LayerKeySort/liblayerkeysort.a','-lbcrypt','-o',exe) is None:continue
        db=OUT/(kind+'-db')
        db.mkdir()
        text=run(exe,fixtures,'persistent',db/'data.ow')
        if text is not None:
            assert sum(line.startswith('receipt,') for line in text.splitlines())==steps
            (OUT/(kind+'.summary')).write_text(text.splitlines()[0]+'\n')
    needle='aa95b240b019c518a98bfb900ec374d24a537165:benchmarks/research/full-ri-20261124/archive/tooling/database_oom.c'
    src=OUT/'database_oom.c'
    src.write_bytes(subprocess.check_output(['git','-C',str(OW),'show',needle]))
    staged=final/'candidate'
    exe=OUT/'database-oom.exe'
    if run('gcc','-std=c17','-O3','-DNDEBUG','-DLKS_ENABLE_ALLOC_DIAGNOSTICS',
           '-I'+str(staged/'include'),'-I'+str(staged/'third_party/LayerKeySort/src'),src,
           staged/'build/libordweave.a',DIAG,'-lbcrypt','-o',exe) is not None:
        data=run(exe)
        if data is not None:(OUT/'database-oom.csv').write_text(data,encoding='ascii')
(OUT/'fixture.sha256').write_text(hashlib.sha256(fixtures.read_bytes()).hexdigest()+'\n',encoding='ascii')
