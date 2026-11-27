from pathlib import Path
import subprocess as sp,json
import sys
R=Path(sys.argv[1]).resolve();O=Path(__file__).resolve().parent
builds={'old':O/'A/build','new':Path(sys.argv[2]).resolve()}
results=[]
for name,b in builds.items():
 sp.run(['cmake','--build',str(b),'--target','ordweave','ordweave_history','--parallel','4'],check=True,stdout=sp.DEVNULL)
 sp.run(['gcc','-std=c17','-O2','-I'+str(R/'include'),str(O/'compat.c'),str(b/'libordweave.a'),str(b/'third_party/LayerKeySort/liblayerkeysort.a'),'-lbcrypt','-o',str(O/f'compat-{name}.exe')],check=True)
for creator,reader in [('old','new'),('new','old')]:
 folder=O/f'compat-{creator}-to-{reader}';folder.mkdir(exist_ok=True)
 db=folder/'data';backup=folder/'backup';returned=folder/'returned'
 p=sp.run([str(builds[creator]/'ordweave_history.exe'),'fixture',str(db)],capture_output=True,text=True);p.check_returncode();assert p.stdout.strip().split()[-1]=='40'
 p=sp.run([str(O/f'compat-{reader}.exe'),str(db),str(backup),str(returned)],capture_output=True,text=True);results.append(dict(direction=f'{creator}->{reader}',code=p.returncode,stdout=p.stdout,stderr=p.stderr));p.check_returncode()
 # Read and re-check the other build's rewritten checkpoint and backup in reverse.
 p=sp.run([str(O/f'compat-{creator}.exe'),str(returned),str(folder/'back-again'),str(folder/'back-final')],capture_output=True,text=True);results.append(dict(direction=f'{creator}->{reader}->{creator}',code=p.returncode,stdout=p.stdout,stderr=p.stderr));p.check_returncode()
(O/'compatibility.json').write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results,indent=2))
