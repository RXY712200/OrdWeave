from pathlib import Path
import subprocess as sp,tarfile,io,shutil,json
R=Path('D:/OrdWeave');O=R/'build/preview3';results=[]
for name,b in [('base',O/'base/build'),('candidate',R/'build/preview3-release')]:
 s=(O/'import_bench.c').read_text().replace('int main(', 'void memory_report(const char *);\nint main(').replace('t=now();OK(ow_core_import','memory_report("before_import");t=now();OK(ow_core_import').replace('double import=now()-t;','double import=now()-t;memory_report("after_import");')
 p=O/'import_resource.c';p.write_text(s)
 sp.run(['gcc','-O3','-std=c17','-Iinclude','-Isrc',str(p),str(R/'build/preview2/memory.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free','-o',str(O/f'resource-{name}.exe')],check=True)
 for n,c,f in [(128,1,6),(8192,1,6),(8192,8,6),(2048,8,64)]:
  p=sp.run([str(O/f'resource-{name}.exe'),str(n),str(c),str(f)],capture_output=True,text=True,check=True);results.append(dict(variant=name,n=n,collections=c,fields=f,output=p.stdout))
(O/'import-resources.json').write_text(json.dumps(results,indent=2)+'\n')
# Separate source-instrumented builds; no diagnostic changes reach production.
for name in ('base','candidate'):
 src=O/f'diagnostic-{name}';src.mkdir(exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','archive','2bbf7640711e6561a5d5979f0de53a7f6ee0a1e8'],cwd=R))) as t:t.extractall(src,filter='data')
 dep=src/'third_party/LayerKeySort';dep.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','archive','HEAD'],cwd=R/'third_party/LayerKeySort'))) as t:t.extractall(dep,filter='data')
 if name=='candidate':
  for p in (R/'src').iterdir():
   if p.is_file():(src/'src'/p.name).write_bytes(p.read_bytes())
 p=src/'src/wal_windows.c';s=p.read_text();s=s.replace('static OwStatus replay_segment(', 'static OwStatus diagnostic_replay_impl(',1)
 s=s.replace('    s->scan_revision=expected;', '    diagnostic_frames++;\n    s->scan_revision=expected;')
 pos=s.index('static OwStatus diagnostic_replay_impl(')
 s=s[:pos]+'static unsigned long long diagnostic_frames;\n'+s[pos:]
 pos=s.index('static OwStatus clear_pending(')
 wrapper='''static OwStatus replay_segment(OwStore *s,OwDatabase *db,uint64_t seq,int last){
 LARGE_INTEGER a,b,f;QueryPerformanceFrequency(&f);QueryPerformanceCounter(&a);
 unsigned long long before=diagnostic_frames;int v=s->validating,m=s->semantic_scan,l=s->limit_replay;
 OwStatus status=diagnostic_replay_impl(s,db,seq,last);QueryPerformanceCounter(&b);
 fprintf(stderr,"scan,%d,%d,%d,%llu,%llu,%d,%.9f\\n",v,m,l,(unsigned long long)seq,diagnostic_frames-before,status,(double)(b.QuadPart-a.QuadPart)/f.QuadPart);return status;
}
'''
 s=s[:pos]+wrapper+s[pos:];s='#include <stdio.h>\n'+s;p.write_text(s)
 with (src/'CMakeLists.txt').open('a') as f:f.write('\nadd_executable(maintenance_probe "'+str(O/'maintenance_bench.c').replace('\\','/')+'")\ntarget_link_libraries(maintenance_probe PRIVATE ordweave bcrypt)\n')
 with (O/f'diagnostic-build-{name}.log').open('w') as log:
  for cmd in [['cmake','-S',str(src),'-B',str(src/'build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(src/'build'),'--target','maintenance_probe','--parallel','4']]:sp.run(cmd,check=True,stdout=log,stderr=sp.STDOUT)
 for label in ('genesis-8192','retained'):
  case=O/f'diagnostic-fixture-{name}-{label}';shutil.copytree(O/label,case);backup=O/f'diagnostic-backup-{name}-{label}'
  p=sp.run([str(src/'build/maintenance_probe.exe'),'run',str(case),str(backup)],capture_output=True,text=True,check=True);(O/f'scans-{name}-{label}.log').write_text(p.stderr+'timing,'+p.stdout)
print('Import allocation and operation-local replay diagnostics recorded')
