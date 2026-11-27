from pathlib import Path
import subprocess as sp,tarfile,io,json,hashlib,re,shutil
import sys
R=Path(sys.argv[1]).resolve();O=R/'build/eg003';O.mkdir(exist_ok=True)
fixture=Path(sys.argv[2]).resolve()
inventory={str(p.relative_to(fixture)).replace('\\','/'):{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in fixture.rglob('*') if p.is_file()}
(O/'fixture-inventory.json').write_text(json.dumps(inventory,indent=2)+'\n')
shutil.copytree(fixture,O/'preserved-copy',dirs_exist_ok=True)
src=O/'diagnostic-source';src.mkdir(exist_ok=True)
for repo,rev,dest in [(R,'26704b68f2cb61e6edb5746294e924aceb08d5ab',src),(R/'third_party/LayerKeySort','f751e1ee2afd682874281d6dc495969f9eca49eb',src/'third_party/LayerKeySort')]:
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',rev]))) as t:t.extractall(dest,filter='data')
for name in ['checkpoint_windows.inc','checkpoint_core.inc']:
 p=src/'src'/name;s=p.read_text()
 # Wrap every executed CORRUPT assignment/return in these two owning includes.
 s=s.replace('return OW_CORRUPT;',f'{{ fprintf(stderr,"CORRUPT {name}:%d alloc=%ld\\n",__LINE__,ow_test_checkpoint_alloc); return OW_CORRUPT; }}')
 s=s.replace('status = OW_CORRUPT;',f'status = (fprintf(stderr,"SET_CORRUPT {name}:%d alloc=%ld\\n",__LINE__,ow_test_checkpoint_alloc), OW_CORRUPT);')
 # Preserve declarations: initial status is not itself a taken corruption branch.
 s=s.replace(f'OwStatus status = (fprintf(stderr,"SET_CORRUPT {name}:%d alloc=%ld\\n",__LINE__,ow_test_checkpoint_alloc), OW_CORRUPT);','OwStatus status = OW_CORRUPT;')
 if name=='checkpoint_windows.inc':
  s=s.replace('  free(bytes);\n  ow_store_leave(s);\n  return status;','  if (ow_test_checkpoint_alloc >= 0) fprintf(stderr,"CREATE_DONE status=%d alloc=%ld gen=%llu\\n",status,ow_test_checkpoint_alloc,(unsigned long long)gen);\n  free(bytes);\n  ow_store_leave(s);\n  return status;')
  # Every outward stage failure gets the source line before its existing goto.
  s=s.replace('goto done;', 'do { if(status != OW_OK) fprintf(stderr,"CP_EXIT line=%d status=%d alloc=%ld\\n",__LINE__,status,ow_test_checkpoint_alloc); goto done; } while(0);')
 p.write_text('#include <stdio.h>\n'+s)
p=src/'tests/checkpoint.c';s=p.read_text();s=s.replace('    ow_test_checkpoint_alloc = i;','    fprintf(stderr,"BOUNDARY %u\\n",i);\n    ow_test_checkpoint_alloc = i;');p.write_text(s)
with (O/'build.log').open('w') as log:
 for cmd in [['cmake','-S',str(src),'-B',str(O/'diagnostic-build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],['cmake','--build',str(O/'diagnostic-build'),'--target','ordweave_checkpoint','--parallel','4']]:sp.run(cmd,stdout=log,stderr=sp.STDOUT,check=True)
print('Preserved original fixture; instrumented isolated checkpoint diagnostic built.')
