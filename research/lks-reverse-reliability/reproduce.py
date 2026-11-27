"""Fresh isolated build/reverse directory required; never deletes fixtures.
Build with --build-only, then run.py and probes individually, or execute all.
EG003: run eg003.py separately with an extracted archived fixture path.
"""
from pathlib import Path
import subprocess as sp,sys
R=Path.cwd();S=R/'research/lks-reverse-reliability';O=R/'build/reverse';O.mkdir(parents=True,exist_ok=True)
def call(args):sp.run(list(map(str,args)),check=True,timeout=600)
for name,diag in [('reverse-prod',False),('reverse-diag',True)]:
 b=R/'build'/name
 cmd=['cmake','-S',R,'-B',b,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release']
 if diag:cmd+=['-DCMAKE_C_FLAGS=-DLKS_ENABLE_ALLOC_DIAGNOSTICS -DOW_UPSTREAM_ALLOC_TEST']
 call(cmd);call(['cmake','--build',b,'--parallel','4'])
for name in ['driver','cache_probe','typed_lifecycle','large_order','lifetime','values','highwater','gap']:
 cmd=['gcc','-O2','-std=c17','-Iinclude',S/(name+'.c'),R/'build/reverse-prod/libordweave.a',R/'build/reverse-prod/third_party/LayerKeySort/liblayerkeysort.a','-lbcrypt','-o',O/(name+'.exe')]
 if name=='cache_probe':cmd+=['-Wl,--wrap=malloc']
 call(cmd)
for name in ['native_faults','structure_probe']:
 call(['gcc','-O2','-std=c17','-DLKS_ENABLE_ALLOC_DIAGNOSTICS','-Ithird_party/LayerKeySort/include','-Ithird_party/LayerKeySort/src',S/(name+'.c'),R/'build/reverse-diag/third_party/LayerKeySort/liblayerkeysort.a','-o',O/(name+'.exe')])
if '--build-only' in sys.argv:sys.exit(0)
call([sys.executable,S/'run.py']);call([sys.executable,S/'traces.py'])
for name,args in [('typed_lifecycle',[]),('large_order',[]),('lifetime',[]),('values',[]),('gap',[]),('cache_probe',[O/'reproduce-cache-db']),('highwater',[O/'reproduce-highwater-db']),('native_faults',[]),('native_faults',['expanded']),('structure_probe',[])]:
 with (O/('reproduce-'+name+'.log')).open('w') as f:sp.run([str(O/(name+'.exe')),*map(str,args)],check=True,timeout=600,stdout=f,stderr=sp.STDOUT)
call([R/'build/reverse-diag/ordweave_failures.exe'])
call(['ctest','--test-dir',R/'build/reverse-prod','-R','^(ordweave_smoke|ordweave_checkpoint|ordweave_history|ordweave_persistence|ordweave_adversarial|ordweave_format|ordweave_checkpoint_format|ordweave_history_format|ordweave_audit_format)$','--output-on-failure'])
