"""Independent anchored-root grammar/corruption oracle; no production serializer."""
import pathlib,sys,subprocess,tempfile,shutil,hashlib,json
exe=str(pathlib.Path(sys.argv[1]).resolve())
be=lambda n,k:n.to_bytes(k,'big')
get=lambda p,o,k:int.from_bytes(p[o:o+k],'big')
sha=lambda b:hashlib.sha256(b).digest()
fixed=json.loads(pathlib.Path(__file__).with_name('history_vectors.json').read_text())
p=b'OWROOT01'+be(1,4)+be(256,4)+bytes(range(16))+be(1,8)+bytes(8)+be(1,8)+bytes(32)+be(1,8)+be(1,8)+bytes(120)
assert (p+sha(p)).hex()==fixed['root_epoch1_genesis_baseline']
p=b'OWPOL001'+be(1,4)+be(96,4)+bytes(range(16))+bytes(32)
assert (p+sha(p)).hex()==fixed['manual_policy']
with tempfile.TemporaryDirectory(prefix='ow-roots-',dir='.') as temp:
 base=pathlib.Path(temp).resolve();fixture=base/'fixture'
 p=subprocess.run([exe,'fixture',str(fixture)],capture_output=True,text=True);assert p.returncode==0,p.stderr
 earliest,latest=map(int,p.stdout.split());root=(fixture/'recovery.current').read_bytes()
 assert root==(fixture/'recovery.current.bak').read_bytes()
 assert root[:8]==b'OWROOT01' and len(root)==256 and get(root,8,4)==1 and sha(root[:-32])==root[-32:]
 assert get(root,40,8)==earliest and get(root,152,8)==latest
 first=get(root,88,8);assert first>1
 assert not (fixture/'wal'/f'{first-1:020d}.wal').exists()
 segment=(fixture/'wal'/f'{first:020d}.wal').read_bytes()
 assert segment[16:32]==root[16:32] and get(segment,40,8)==get(root,96,8)==earliest+1 and segment[48:80]==root[104:136]
 cp=(fixture/'checkpoints'/f'{get(root,48,8):020d}.cp').read_bytes()
 assert cp[-32:]==root[56:88] and get(cp,48,8)==earliest and cp[80:112]==root[104:136]
 def probe(path,status):
  p=subprocess.run([exe,'probe',str(path)],capture_output=True,text=True)
  assert p.returncode==status,(path.name,p.returncode,p.stdout,p.stderr,status)
 probe(fixture,0)
 cases=0
 def mutate(name,offset,value,status=8,both=True,seal=True):
  global cases
  dest=base/name;shutil.copytree(fixture,dest)
  for filename in ['recovery.current','recovery.current.bak'] if both else ['recovery.current']:
   p=dest/filename;b=bytearray(p.read_bytes());b[offset:offset+len(value)]=value
   if seal:b[-32:]=sha(b[:-32])
   p.write_bytes(b)
  probe(dest,status);cases+=1
 mutate('future-root',8,be(2,4),9)
 mutate('bad-length',12,be(255,4))
 mutate('wrong-id',16,bytes(16))
 mutate('zero-epoch',32,bytes(8))
 mutate('bad-checksum',224,bytes(32),seal=False)
 mutate('wrong-segment-sequence',88,be(first+1,8),12)
 mutate('wrong-predecessor',104,bytes(32))
 mutate('missing-generation',48,be(999,8),12)
 mutate('wrong-checkpoint-digest',56,bytes(32))
 mutate('wrong-latest-digest',160,bytes(32))
 mutate('unknown-policy',136,be(9,4))
 mutate('reserved',192,b'x')
 mutate('same-epoch-conflict',144,be(6,8),both=False)
 for name,change,status in [
  ('missing-first',lambda d:(d/'wal'/f'{first:020d}.wal').unlink(),12),
  ('bad-anchor',lambda d:(d/'checkpoints'/f'{get(root,48,8):020d}.cp').write_bytes(b'corrupt'),8),
  ('missing-backup',lambda d:(d/'recovery.current.bak').unlink(),2),
  ('tail-damage',lambda d:(d/'wal'/sorted((d/'wal').glob('*.wal'))[-1].name).write_bytes(b'corrupt'),8)]:
  dest=base/name;shutil.copytree(fixture,dest);change(dest);probe(dest,status);cases+=1
 for name,offset,value,status in [('policy-future',8,be(2,4),9),('policy-wrong-id',16,bytes(16),8),('policy-invalid-n',40,bytes(8),8),('policy-reserved',48,b'x',8)]:
  dest=base/name;shutil.copytree(fixture,dest);p=dest/'history.policy';b=bytearray(p.read_bytes());b[offset:offset+len(value)]=value;b[-32:]=sha(b[:-32]);p.write_bytes(b);probe(dest,status);cases+=1
print(f'{cases} independent recovery-root malformed cases; prefix before segment {first} physically absent; retained {earliest}..{latest}')

