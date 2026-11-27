import subprocess as sp, pathlib as pl, io, tarfile, zipfile, json, hashlib, platform
import sys
ROOT=pl.Path(sys.argv[1]).resolve(); OUT=pl.Path(__file__).resolve().parent
BASE='32f853defc0a2e9f0ca809fb3535a90c977c7665'
OLD='70d7f0ef5857b54381b863d292a213909d87459f'; NEW='f751e1ee2afd682874281d6dc495969f9eca49eb'
def git(*args,repo=ROOT):return sp.check_output(['git','-C',str(repo),*args])
def extract(repo,rev,dest):
 dest.mkdir(parents=True,exist_ok=True)
 with tarfile.open(fileobj=io.BytesIO(git('archive',rev,repo=repo))) as t:t.extractall(dest,filter='data')
arc='aa95b240b019c518a98bfb900ec374d24a537165:benchmarks/research/full-ri-20261124/archive/'
driver=git('show',arc+'tooling/database.c')
items=json.loads(git('show',arc+'trace-manifest.json'))
selected=[x for x in items if x['seed']==17 and x['scenario']=='update-heavy' and x['n'] in (128,2048)]
assert len(selected)==2
(OUT/'traces').mkdir(exist_ok=True)
with zipfile.ZipFile(io.BytesIO(git('show',arc+'traces.zip'))) as z:
 for x in selected:
  data=z.read(x['name']);assert hashlib.sha256(data).hexdigest()==x['sha256'];(OUT/'traces'/x['name']).write_bytes(data)
protocol='''# Prospective protocol (written before new timings)
A: exact v1.0.0 runtime + Full4.1; B: identical runtime + Full4.2 Stable. No guard fix or version change. Archive generated update-heavy seed17 N128/2048, 1000 operations each; independent final order/record model and initial historical order asserted by the existing driver. Separate memory/persistent runs; setup, operations, checkpoint, reopen and history QPC seconds. One discarded warmup then three fresh measured processes, alternating A/B execution order. GCC Release, Ninja, Windows x64, serial execution. No instrumented production files. No historical/candidate timings pooled.

Correctness requires every process and receipt check to pass. Flag memory operation median regression >15% AND >1ms; flag persistent operations/checkpoint/reopen/history >25% AND >10ms for investigation, not an automatic claim of stable regression. Three repeats provide descriptive evidence only; do not infer precise IO gains. Expected: 4.2 may improve order internals, but full global index rebuild and semantic replay remain database costs. Negative/noisy results must remain visible. No peak-memory instrumentation: resource evidence limited to workload size, binary hashes and machine/toolchain metadata. Bounded trace population is not a production scale guarantee.
'''
(OUT/'PROTOCOL.md').write_text(protocol,encoding='utf-8')
meta=dict(base=BASE,A=OLD,B=NEW,traces=selected,platform=platform.platform(),cpu=platform.processor(),compiler=sp.check_output(['gcc','--version'],text=True),binaries={})
for name,pin in [('A',OLD),('B',NEW)]:
 dest=OUT/name;extract(ROOT,BASE,dest);extract(ROOT/'third_party/LayerKeySort',pin,dest/'third_party/LayerKeySort')
 (dest/'driver.c').write_bytes(driver)
 with (dest/'CMakeLists.txt').open('a') as f:f.write('\nadd_executable(preview1_driver driver.c)\ntarget_link_libraries(preview1_driver PRIVATE ordweave bcrypt)\n')
 with (OUT/f'build-{name}.log').open('w') as log:
  for cmd in [['cmake','-S',str(dest),'-B',str(dest/'build'),'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=gcc'],['cmake','--build',str(dest/'build'),'--target','preview1_driver','ordweave_history','--parallel','4']]:sp.run(cmd,stdout=log,stderr=sp.STDOUT,check=True)
 meta['binaries'][name]=hashlib.sha256((dest/'build/preview1_driver.exe').read_bytes()).hexdigest()
(OUT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
print('Isolated A/B built; prospective protocol and trace hashes recorded.')
