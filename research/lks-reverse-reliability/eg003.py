"""One bounded instrumented current-Stable window on preserved fixture copies."""
from pathlib import Path
import subprocess as sp,tarfile,io,shutil,json,hashlib
R=Path.cwd();O=R/'build/reverse';S=O/'eg003-source';S.mkdir(exist_ok=True)
for repo,ref,dest in [(R,'0350628f1b67353cc15283a7e1a1ae86a7004459',S),(R/'third_party/LayerKeySort','f751e1ee2afd682874281d6dc495969f9eca49eb',S/'third_party/LayerKeySort')]:
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',ref]))) as a:a.extractall(dest,filter='data')
patch={}
for name in ['checkpoint_core.inc','checkpoint_windows.inc']:
 p=S/'src'/name;s=p.read_text();before=s
 helper='''#include <stdio.h>
static int rr_oom(const char*file,int line){int hit=ow_checkpoint_oom();if(hit)fprintf(stderr,"TRIGGER %s:%d\\n",file,line);return hit;}
'''
 if name=='checkpoint_core.inc':
  at=s.index('typedef struct {');s=s[:at]+helper+s[at:].replace('ow_checkpoint_oom()','rr_oom(__FILE__,__LINE__)')
 else:s=helper+s.replace('ow_checkpoint_oom()','rr_oom(__FILE__,__LINE__)')
 p.write_text(s);patch['src/'+name]=dict(before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=hashlib.sha256(s.encode()).hexdigest())
p=S/'tests/checkpoint.c';s=p.read_text();block='''  if(argc==4&&!strcmp(argv[1],"rr-eg003")){
    OK(ow_database_open(argv[2],&d));before=revision(d);
    ow_test_checkpoint_alloc=strtol(argv[3],NULL,10);OwStatus result=ow_checkpoint_create(d);
    ow_test_checkpoint_alloc=-1;printf("boundary=%s status=%d revision=%llu\\n",argv[3],result,(unsigned long long)revision(d));T(revision(d)==before);
    OwStatus closed=ow_database_close(d);T(closed==OW_OK||closed==OW_RECOVERY_REQUIRED);
    OK(ow_database_open(argv[2],&d));T(revision(d)==before);OK(ow_database_close(d));
    return result==OW_OK||result==OW_NOMEM||result==OW_RECOVERY_REQUIRED?0:result;
  }
''';s=s.replace('  if (argc == 3) {',block+'  if (argc == 3) {',1);p.write_text(s)
B=O/'eg003-build'
with (O/'eg003-build.log').open('w') as log:
 for cmd in [['cmake','-S',str(S),'-B',str(B),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(B),'--target','ordweave_checkpoint','--parallel','4']]:sp.run(cmd,check=True,stdout=log,stderr=sp.STDOUT,timeout=600)
import sys
source=Path(sys.argv[1]).resolve() if len(sys.argv)>1 else R/'build/eg003/preserved-copy';assert source.is_dir()
inventory={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
results=[]
for boundary in range(124,129):
 dest=O/f'eg003-fixture-{boundary}';shutil.copytree(source,dest)
 with (O/f'eg003-{boundary}.log').open('w') as log:p=sp.run([str(B/'ordweave_checkpoint.exe'),'rr-eg003',str(dest),str(boundary)],stdout=log,stderr=sp.STDOUT,timeout=600)
 results.append(dict(boundary=boundary,code=p.returncode,log=f'eg003-{boundary}.log'))
assert inventory=={str(p.relative_to(source)):hashlib.sha256(p.read_bytes()).hexdigest() for p in source.rglob('*') if p.is_file()}
(O/'eg003.json').write_text(json.dumps(dict(subject='Current Stable instrumented; historical fixture already reopened before original investigation. Ordinal mapping not assumed stable.',fixture=inventory,patch=patch,results=results),indent=2)+'\n')
# Store exact instrumentation as a diagnostic-only artifact, never root source.
import difflib
diff=''
for n in ['src/checkpoint_core.inc','src/checkpoint_windows.inc','tests/checkpoint.c']:
 original=sp.check_output(['git','show','0350628:'+n]).decode();diff+=''.join(difflib.unified_diff(original.splitlines(True),(S/n).read_text().splitlines(True),fromfile='a/'+n,tofile='b/'+n))
(O/'eg003.patch').write_text(diff)
print(json.dumps(results))
