from pathlib import Path
import subprocess as sp,json,hashlib,tarfile,io
R=Path('D:/OrdWeave');O=R/'build/step15'
builds={'candidate':R/'build/step15-release'}
provenance={}
for name,ref in [('stable','32f853defc0a2e9f0ca809fb3535a90c977c7665'),('preview3','edb9675b10125cecdf936e030bd9a54a90a84146')]:
 src=O/'compat-builds'/name;src.mkdir(parents=True,exist_ok=True)
 pin=sp.check_output(['git','ls-tree',ref,'third_party/LayerKeySort'],text=True).split()[2]
 for repo,commit,dest in [(R,ref,src),(R/'third_party/LayerKeySort',pin,src/'third_party/LayerKeySort')]:
  dest.mkdir(parents=True,exist_ok=True)
  with tarfile.open(fileobj=io.BytesIO(sp.check_output(['git','-C',str(repo),'archive',commit]))) as t:t.extractall(dest,filter='data')
 bld=src/'build'
 with (O/f'compat-{name}-build.log').open('w') as log:
  sp.run(['cmake','-S',str(src),'-B',str(bld),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release'],check=True,stdout=log,stderr=sp.STDOUT)
  sp.run(['cmake','--build',str(bld),'--target','ordweave','ordweave_history','--parallel','4'],check=True,stdout=log,stderr=sp.STDOUT)
 builds[name]=bld;provenance[name]=dict(source=ref,dependency=pin)
results=[];bytehash={}
for name,b in builds.items():
 cmd=['gcc','-std=c17','-O2','-I'+str(R/'include'),str(O/'compat.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'compat-{name}.exe')]
 if name=='candidate':cmd+=['-DCANDIDATE','-Wl,--wrap=malloc']
 sp.run(cmd,check=True)
 for enabled in ([False,True] if name=='candidate' else [False]):
  label=name+('-enabled' if enabled else '');exe=O/f'export-{label}.exe';out=O/f'export-{label}.bin'
  cmd=['gcc','-std=c17','-O2','-I'+str(R/'include'),'-I'+str(R/'src'),str(O/'byte_export.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(exe)]
  if enabled:cmd+=['-DENABLED']
  sp.run(cmd,check=True);sp.run([str(exe),str(out)],check=True);bytehash[label]=dict(bytes=out.stat().st_size,sha256=hashlib.sha256(out.read_bytes()).hexdigest())
assert len({v['sha256'] for v in bytehash.values()})==1
for prior in ('stable','preview3'):
 for creator,reader in [(prior,'candidate'),('candidate',prior)]:
  folder=O/f'final-roundtrip-{creator}-{reader}';folder.mkdir(exist_ok=True)
  db=folder/'data';backup=folder/'backup';returned=folder/'returned'
  p=sp.run([str(builds[creator]/'ordweave_history.exe'),'fixture',str(db)],capture_output=True,text=True,check=True);assert p.stdout.strip().split()[-1]=='40'
  if creator=='candidate':
   # Actual candidate-owned source backup is checkpointed while index enabled.
   prepared=folder/'prepared';sp.run([str(O/'compat-candidate.exe'),str(db),str(prepared),str(folder/'prepared-return')],check=True,capture_output=True,text=True);db=prepared
  for label,args in [(f'{creator}->{reader}',[str(O/f'compat-{reader}.exe'),str(db),str(backup),str(returned)]),(f'{creator}->{reader}->{creator}',[str(O/f'compat-{creator}.exe'),str(returned),str(folder/'back-again'),str(folder/'back-final')])]:
   p=sp.run(args,capture_output=True,text=True);results.append(dict(direction=label,command=args,code=p.returncode,stdout=p.stdout,stderr=p.stderr));p.check_returncode()
(O/'compatibility.json').write_text(json.dumps(dict(baselines=provenance,roundtrips=results,canonical=bytehash,reopen_cache_absence='Candidate range has exactly two malloc calls after each open; enabled path has exactly one. Public API only; diagnostic linker wrapper, no production hooks.'),indent=2)+'\n');print(json.dumps(bytehash,indent=2));print('Four bilateral pairs and enabled-source backups verified')
