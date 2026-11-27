from pathlib import Path
import subprocess as sp,shutil,json
import sys
R=Path(sys.argv[1]).resolve();O=R/'build/eg003';S=O/'diagnostic-source'
p=S/'src/checkpoint_core.inc';s=p.read_text();s=s.replace('status = (fprintf(stderr,"SET_CORRUPT checkpoint_core.inc:%d alloc=%ld\\n",__LINE__,ow_test_checkpoint_alloc), OW_CORRUPT);\n  for (i = 0; i < nr;', 'status = OW_CORRUPT;\n  for (i = 0; i < nr;')
# Replace calls, not the original function definition, with a test-build observer.
marker='typedef struct {\n  unsigned char *p;'
helper='''static int eg003_oom_at(const char *file,int line) {
  int result=ow_checkpoint_oom();
  if(result) fprintf(stderr,"OOM %s:%d\\n",file,line);
  return result;
}
'''
s=s.replace(marker,helper+marker)
start=s.index(marker);s=s[:start]+s[start:].replace('ow_checkpoint_oom()', 'eg003_oom_at(__FILE__,__LINE__)');p.write_text(s)
p=S/'src/checkpoint_windows.inc';s=p.read_text();s=s.replace('ow_checkpoint_oom()','eg003_windows_oom_at(__FILE__,__LINE__)')
s='''#include <stdio.h>
static int eg003_windows_oom_at(const char *file,int line) {
 int result=ow_checkpoint_oom();if(result)fprintf(stderr,"OOM %s:%d\\n",file,line);return result;
}
'''+s.replace('#include <stdio.h>\n','',1);p.write_text(s)
p=S/'src/wal_windows.c';s=p.read_text();s=s.replace('      return OW_CORRUPT;', '      { fprintf(stderr,"WAL_CORRUPT line=%d alloc=%ld\\n",__LINE__,ow_test_checkpoint_alloc); return OW_CORRUPT; }');s='#include <stdio.h>\n'+s;p.write_text(s)
p=S/'tests/checkpoint.c';s=p.read_text();marker='  if (argc == 3) {'
body='''  if (argc == 4 && !strcmp(argv[1],"eg003")) {
    OK(ow_database_open(argv[2],&d));
    before=revision(d);
    ow_test_checkpoint_alloc=strtol(argv[3],NULL,10);
    OwStatus status=ow_checkpoint_create(d);
    ow_test_checkpoint_alloc=-1;
    printf("fixture boundary=%s status=%d revision=%llu\\n",argv[3],status,(unsigned long long)revision(d));
    T(revision(d)==before);
    OwStatus closed=ow_database_close(d);
    T(closed==OW_OK||closed==OW_RECOVERY_REQUIRED);
    OK(ow_database_open(argv[2],&d));T(revision(d)==before);OK(ow_database_close(d));
    return status==OW_OK||status==OW_NOMEM||status==OW_RECOVERY_REQUIRED?0:status;
  }
'''
assert marker in s;s=s.replace(marker,body+marker,1);p.write_text(s)
with (O/'build-refined.log').open('w') as log:sp.run(['cmake','--build',str(O/'diagnostic-build'),'--target','ordweave_checkpoint','--parallel','4'],check=True,stdout=log,stderr=sp.STDOUT)
exe=O/'diagnostic-build/ordweave_checkpoint.exe';results=[]
for repeat in (2,3):
 with (O/f'run-{repeat}.log').open('w') as log:p=sp.run([str(exe)],cwd=O,stdout=log,stderr=sp.STDOUT)
 results.append(dict(kind='focused-checkpoint-suite',repeat=repeat,code=p.returncode))
for boundary in range(124,129):
 dest=O/f'fixture-{boundary}';shutil.copytree(O/'preserved-copy',dest)
 with (O/f'fixture-{boundary}.log').open('w') as log:p=sp.run([str(exe),'eg003',str(dest),str(boundary)],cwd=O,stdout=log,stderr=sp.STDOUT)
 results.append(dict(kind='preserved-fixture-copy',boundary=boundary,code=p.returncode))
(O/'focused-results.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
