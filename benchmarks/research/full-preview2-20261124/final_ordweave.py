"""Repeat selected OrdWeave cases with the final exact Full and separate attribution."""
import csv, gzip, hashlib, io, json, shutil, subprocess, sys, tarfile, time
from pathlib import Path

FULL, PREVIOUS, OUT = (Path(x).resolve() for x in sys.argv[1:4])
CANDIDATE = 'fe00536e7bd7f4e5128432ea4a29e54158074903'
OUT.mkdir()
meta=json.loads((PREVIOUS/'metadata.json').read_text())
papers=[x for x in meta['traces'] if x['seed']==17]
assert len(papers)==9
old_exes={x:PREVIOUS/x/'build/preview2_driver.exe' for x in ('stable41','preview12')}
for variant,path in old_exes.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest()==meta['binaries'][variant]
owcopy=PREVIOUS/'preview12'
paths={}
def stage(kind):
    folder=OUT/kind
    folder.mkdir()
    for path in owcopy.iterdir():
        if path.name not in ('build','third_party'):
            if path.is_dir():shutil.copytree(path,folder/path.name)
            else:shutil.copyfile(path,folder/path.name)
    dep=folder/'third_party/LayerKeySort'
    dep.mkdir(parents=True)
    with tarfile.open(fileobj=io.BytesIO(subprocess.check_output(
            ['git','-C',str(FULL),'archive',CANDIDATE]))) as z:z.extractall(dep,filter='data')
    return folder
def profile(src):
    p=src/'src/ordweave.c'
    s=p.read_text()
    # Set up nonoverlapping stage, publish and collection spans in the core.
    pos=s.index('#include')
    s=s[:pos]+'''#include <windows.h>
double ow_exp_clock(void) {
    LARGE_INTEGER t,f;
    QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f);
    return (double)t.QuadPart/f.QuadPart;
}
double ow_exp_time[5];
size_t ow_exp_count[5];
void ow_exp_reset(void) {
    for(size_t i=0;i<5;++i){ow_exp_time[i]=0;ow_exp_count[i]=0;}
}
void ow_exp_add(size_t category,double start) {
    ow_exp_time[category]+=ow_exp_clock()-start;
    ++ow_exp_count[category];
}
'''+s[pos:]
    a='static OwStatus stage_occurrences('
    b='static void publish_occurrences('
    assert s.count(a)==1 and s.count(b)==1
    s=s.replace(a,'static OwStatus stage_impl(').replace(b,'static void publish_impl(')
    marker='OwStatus ow_occurrence_insert('
    assert s.count(marker)==1
    s=s.replace(marker,'''static OwStatus stage_occurrences(OwDatabase *d, Occurrence *add,
    Occurrence *omit, LksManagedOrder **out) {
    double t=ow_exp_clock();
    OwStatus r=stage_impl(d,add,omit,out);
    ow_exp_add(0,t);
    return r;
}
static void publish_occurrences(OwDatabase *d,LksManagedOrder *index) {
    double t=ow_exp_clock();
    publish_impl(d,index);
    ow_exp_add(1,t);
}
'''+marker)
    for line in ('s = ow_order_insert(c->order, o, p, a ? a->handle : NULL, &o->handle);',
                 's = ow_order_move(o->collection->order, o->handle, p, a ? a->handle : NULL);',
                 's = ow_lks_status(lks_order_remove(o->collection->order, o->handle, NULL));'):
        assert s.count(line)==1
        s=s.replace(line,'{double t=ow_exp_clock(); '+line+' ow_exp_add(2,t);}')
    p.write_text(s)
    p=src/'src/wal_windows.c'
    s=p.read_text()
    marker='OwStatus ow_store_commit('
    assert s.count(marker)==1 and s.count('!FlushFileBuffers(s->wal)')==1
    s=s.replace(marker,'''extern double ow_exp_clock(void);
extern void ow_exp_add(size_t,double);
static BOOL ow_exp_flush(HANDLE h) {
    double t=ow_exp_clock();
    BOOL result=FlushFileBuffers(h);
    ow_exp_add(4,t);
    return result;
}
static OwStatus ow_exp_commit_impl(''').replace('!FlushFileBuffers(s->wal)','!ow_exp_flush(s->wal)')
    s+='''
OwStatus ow_store_commit(OwStore *s, unsigned char *f, size_t n, OwCommand *cmd,
                         uint64_t revision) {
    double t=ow_exp_clock();
    OwStatus r=ow_exp_commit_impl(s,f,n,cmd,revision);
    ow_exp_add(3,t);
    return r;
}
'''
    p.write_text(s)
    p=src/'driver.c'
    s=p.read_text()
    s=s.replace('int main(int argc,char **argv){','extern double ow_exp_time[5];\nextern size_t ow_exp_count[5];\nextern void ow_exp_reset(void);\nint main(int argc,char **argv){')
    s=s.replace('    start=now();\n    for(size_t i=0;i<steps;','    ow_exp_reset();\n    start=now();\n    for(size_t i=0;i<steps;')
    s=s.replace('double operations=now()-start;', '''double operations=now()-start;
    printf("profile");
    for(size_t k=0;k<5;++k)printf(",%.9f,%zu",ow_exp_time[k],ow_exp_count[k]);
    puts("");''')
    p.write_text(s)
with (OUT/'build.log').open('wb') as log:
    def build(*cmd):
        log.write((' '.join(map(str,cmd))+'\n').encode())
        log.flush()
        subprocess.run(list(map(str,cmd)),stdout=log,stderr=subprocess.STDOUT,check=True)
    for v in ('candidate','profile-final'):
        s=stage(v)
        if v=='profile-final':profile(s)
        b=s/'build'
        build('cmake','-S',s,'-B',b,'-G','Ninja','-DCMAKE_C_COMPILER=gcc','-DCMAKE_BUILD_TYPE=Release')
        if v=='candidate':
            build('cmake','--build',b,'--parallel','3')
            build('ctest','--test-dir',b,'--output-on-failure')
        else:build('cmake','--build',b,'--target','preview2_driver','--parallel','3')
        paths[v]=b/'preview2_driver.exe'
paths.update(old_exes)
meta.update(candidate=CANDIDATE, final_binaries={v:hashlib.sha256(p.read_bytes()).hexdigest() for v,p in paths.items()})
(OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
with gzip.open(OUT/'invocations.jsonl.gz','wt',encoding='utf-8') as receipts, (OUT/'summary.csv').open('w',newline='') as f:
    w=csv.writer(f,lineterminator='\n')
    w.writerow(['trace','mode','variant','repeat','setup','operations','checkpoint','reopen','history','n','steps',
                'stage','stage_calls','publish','publish_calls','collection','collection_calls',
                'wal','wal_calls','flush','flush_calls'])
    for i,case in enumerate(papers):
        trace=PREVIOUS/'traces'/case['name']
        assert hashlib.sha256(trace.read_bytes()).hexdigest()==case['sha256']
        for mode in ('memory','persistent'):
            for repeat in range(5):
                order=['stable41','preview12','candidate']
                offset=(i+repeat)%3
                for variant in (['profile-final'] if repeat==4 else order[offset:]+order[:offset]):
                    record=dict(trace=case['name'],mode=mode,variant=variant,repeat=repeat)
                    if shutil.disk_usage(OUT).free < 512*1024**2:
                        receipts.write(json.dumps(dict(record,status='skipped-disk'))+'\n')
                        continue
                    db=OUT/f'sandbox-{i}-{mode}-{repeat}-{variant}'
                    argv=[str(paths[variant]),str(trace),mode]
                    if mode=='persistent':
                        db.mkdir()
                        argv.append(str(db/'data.ow'))
                    record['argv']=argv
                    try:
                        r=subprocess.run(argv,capture_output=True,text=True,timeout=300)
                    except subprocess.TimeoutExpired:
                        receipts.write(json.dumps(dict(record,status='timeout'))+'\n')
                        raise
                    record.update(code=r.returncode,stdout=r.stdout,stderr=r.stderr)
                    receipts.write(json.dumps(record)+'\n')
                    receipts.flush()
                    r.check_returncode()
                    lines=r.stdout.splitlines()
                    fields,=[x.split(',')[1:] for x in lines if x.startswith('summary,')]
                    p=[x.split(',')[1:] for x in lines if x.startswith('profile,')]
                    assert sum(line.startswith('receipt,') for line in lines)==1000
                    w.writerow([case['name'],mode,variant,repeat,*fields,*(p[0] if p else ('',)*10)])
                    f.flush()
                    if db.exists():
                        assert db.resolve().parent==OUT and db.name.startswith('sandbox-')
                        shutil.rmtree(db)
