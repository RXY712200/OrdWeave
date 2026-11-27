from pathlib import Path
import hashlib,json,subprocess as sp
O=Path(__file__).resolve().parent;P=O/'preserved-copy'
get=lambda b,o,n:int.from_bytes(b[o:o+n],'big')
manifest=(P/'manifest.ow').read_bytes();assert len(manifest)==96 and hashlib.sha256(manifest[:-32]).digest()==manifest[-32:]
identity=manifest[16:32]
meta=(P/'checkpoint.current').read_bytes();backup=(P/'checkpoint.current.bak').read_bytes();assert meta==backup and len(meta)==416
assert meta[:8]==b'OWSELECT' and get(meta,8,4)==1 and get(meta,12,4)==416 and meta[16:32]==identity and meta[40:48]==bytes(8)
assert hashlib.sha256(meta[:-32]).digest()==meta[-32:] and get(meta,32,4)==1 and get(meta,36,4)==1
assert meta[160:384]==bytes(224)
gen=get(meta,48,8);rev=get(meta,56,8);assert (gen,rev)==(89,21)
rows=[]
for cp in sorted((P/'checkpoints').iterdir()):
 b=cp.read_bytes();assert cp.name==f'{get(b,40,8):020d}.cp' and len(b)==756
 assert b[:8]==b'OWCHECK1' and get(b,8,4)==1 and get(b,12,4)==144 and get(b,16,8)==len(b) and b[24:40]==identity
 assert hashlib.sha256(b[:-32]).digest()==b[-32:] and get(b,48,8)==21
 if get(b,40,8)==gen:assert b[-32:]==meta[64:96] and b[80:112]==meta[96:128]
 r=sp.run([str(O/'inspect_checkpoint.exe'),str(cp)],capture_output=True,text=True);r.check_returncode()
 rows.append(dict(name=cp.name,generation=get(b,40,8),revision=21,bytes=len(b),code=r.returncode,assertions=r.stdout.strip(),sha256=hashlib.sha256(b).hexdigest()))
assert len(rows)==24
result=dict(current_equals_backup=True,selection_generation=gen,revision=rev,policy_retention=1,pending_files=[],checkpoint_count=len(rows),checkpoints=rows)
(O/'fixture-inspection.json').write_text(json.dumps(result,indent=2)+'\n')
print('Independent SHA/identity/length/selection checks and core import assertions passed for',len(rows),'checkpoint files.')
