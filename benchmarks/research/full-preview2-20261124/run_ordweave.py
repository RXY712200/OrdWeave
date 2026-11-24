"""Stage the archived, native OrdWeave reproduction under a new output root."""
import csv, gzip, hashlib, io, json, os, shutil, subprocess, sys, tarfile, zipfile
from pathlib import Path

FULL = Path(sys.argv[1]).resolve()
OW = Path(sys.argv[2]).resolve()
OUT = Path(sys.argv[3]).resolve()
OUT.mkdir()
PIN = 'b12a007c50c2f9efcbee500a14d3cf09dcc74f10'
ARC = 'aa95b240b019c518a98bfb900ec374d24a537165:benchmarks/research/full-ri-20261124/archive/'
V = {'stable41':'70d7f0ef5857b54381b863d292a213909d87459f',
     'preview12':'61e9290ec5ba47b9092bcb8ae7d61f13104ea839',
     'profile':'61e9290ec5ba47b9092bcb8ae7d61f13104ea839'}

def git(repo,*args):
    return subprocess.check_output(['git','-C',str(repo),*args])
def extract(repo,pin,folder):
    folder.mkdir(parents=True,exist_ok=True)
    with tarfile.open(fileobj=io.BytesIO(git(repo,'archive',pin))) as tar:
        tar.extractall(folder,filter='data')
def sha(data):
    return hashlib.sha256(data).hexdigest()

driver = git(OW,'show',ARC+'tooling/database.c').decode()
items = json.loads(git(OW,'show',ARC+'trace-manifest.json'))
assert len(items)==27
traces = OUT/'traces'
traces.mkdir()
with zipfile.ZipFile(io.BytesIO(git(OW,'show',ARC+'traces.zip'))) as z:
    assert sorted(z.namelist())==sorted(x['name'] for x in items)
    for x in items:
        assert Path(x['name']).name==x['name']
        payload=z.read(x['name'])
        assert sha(payload)==x['sha256']
        (traces/x['name']).write_bytes(payload)
assert V['stable41'] in git(OW,'ls-tree',PIN,'third_party/LayerKeySort').decode()
meta=dict(ordweave=PIN, archive=ARC, full=V, traces=items,
          compiler=subprocess.check_output(['gcc','--version'],text=True), binaries={})
exes={}
def instrument(src):
    path=src/'src/ordweave.c'
    s=path.read_text()
    s=s.replace('static OwStatus stage_occurrences(', 'static OwStatus ow_research_stage_impl(',1)
    marker='static void publish_occurrences('
    assert s.count(marker)==1
    s=s.replace(marker,'''static OwStatus stage_occurrences(OwDatabase *d, Occurrence *add,
    Occurrence *omit, LksManagedOrder **out) {
  double start = ow_research_clock();
  OwStatus result = ow_research_stage_impl(d,add,omit,out);
  ow_research_stage_time += ow_research_clock() - start;
  ++ow_research_stage_calls;
  return result;
}
'''+marker)
    s='''#include <windows.h>
static double ow_research_now(void) {
  LARGE_INTEGER t,f;
  QueryPerformanceCounter(&t); QueryPerformanceFrequency(&f);
  return (double)t.QuadPart/f.QuadPart;
}
static double ow_research_stage_time;
static size_t ow_research_stage_calls;
void ow_research_reset(void) { ow_research_stage_time=0; ow_research_stage_calls=0; }
double ow_research_attribution(size_t *calls) {
  *calls=ow_research_stage_calls;
  return ow_research_stage_time;
}
'''+s.replace('ow_research_clock()', 'ow_research_now()')
    path.write_text(s)
    d=driver.replace('int main(int argc,char **argv){','void ow_research_reset(void);\ndouble ow_research_attribution(size_t *);\nint main(int argc,char **argv){')
    d=d.replace('    start=now();\n    for(size_t i=0;i<steps;', '    ow_research_reset();\n    start=now();\n    for(size_t i=0;i<steps;')
    return d.replace('double operations=now()-start;', 'double operations=now()-start;size_t calls;double rebuild=ow_research_attribution(&calls);printf("profile,%.9f,%zu\\n",rebuild,calls);')

with (OUT/'build.log').open('wb') as buildlog:
    def build(*argv):
        buildlog.write((' '.join(map(str,argv))+'\n').encode())
        buildlog.flush()
        subprocess.run(list(map(str,argv)),stdout=buildlog,stderr=subprocess.STDOUT,check=True)
    for variant, full_pin in V.items():
        src=OUT/variant
        extract(OW,PIN,src)
        extract(FULL,full_pin,src/'third_party/LayerKeySort')
        (src/'driver.c').write_text(instrument(src) if variant=='profile' else driver)
        with (src/'CMakeLists.txt').open('a') as f:
            f.write('\nadd_executable(preview2_driver driver.c)\ntarget_link_libraries(preview2_driver PRIVATE ordweave bcrypt)\n')
        b=src/'build'
        build('cmake','-S',src,'-B',b,'-G','Ninja','-DCMAKE_C_COMPILER=gcc','-DCMAKE_BUILD_TYPE=Release')
        build('cmake','--build',b,'--target','preview2_driver','--parallel','3')
        exes[variant]=b/'preview2_driver.exe'
        meta['binaries'][variant]=sha(exes[variant].read_bytes())
        if variant!='profile':
            build('cmake','--build',b,'--parallel','3')
            build('ctest','--test-dir',b,'--output-on-failure')
(OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
with gzip.open(OUT/'invocations.jsonl.gz','wt',encoding='utf-8') as receipts, (OUT/'summary.csv').open('w',newline='') as f:
    w=csv.writer(f,lineterminator='\n')
    w.writerow(['trace','mode','variant','repeat','setup','operations','checkpoint','reopen','history','n','steps','stage_seconds','stage_calls'])
    for i, case in enumerate(items):
        for mode in ('memory','persistent'):
            for repeat in range(5):
                order=['profile'] if repeat==4 else (['stable41','preview12'] if (repeat+i)%2==0 else ['preview12','stable41'])
                for variant in order:
                    db=OUT/f'sandbox-{i}-{mode}-{repeat}-{variant}'
                    if shutil.disk_usage(OUT).free < 512*1024**2:
                        receipts.write(json.dumps(dict(trace=case['name'],variant=variant,mode=mode,repeat=repeat,status='skipped-disk'))+'\n')
                        continue
                    argv=[str(exes[variant]),str(traces/case['name']),mode]
                    if mode=='persistent':
                        db.mkdir()
                        argv.append(str(db/'data.ow'))
                    record=dict(trace=case['name'],variant=variant,mode=mode,repeat=repeat,argv=argv)
                    try:
                        r=subprocess.run(argv,capture_output=True,text=True,timeout=300)
                        record.update(code=r.returncode,stdout=r.stdout,stderr=r.stderr)
                        receipts.write(json.dumps(record)+'\n')
                        receipts.flush()
                        r.check_returncode()
                        summar,=[line.split(',')[1:] for line in r.stdout.splitlines() if line.startswith('summary,')]
                        prof=[line.split(',')[1:] for line in r.stdout.splitlines() if line.startswith('profile,')]
                        assert sum(line.startswith('receipt,') for line in r.stdout.splitlines())==1000
                        w.writerow([case['name'],mode,variant,repeat,*summar,*(prof[0] if prof else ('',''))])
                        f.flush()
                    except subprocess.TimeoutExpired:
                        receipts.write(json.dumps(dict(record,status='timeout'))+'\n')
                        receipts.flush()
                        raise
                    if db.exists():
                        assert db.resolve().parent==OUT and db.name.startswith('sandbox-')
                        shutil.rmtree(db)
