"""Independent Phase 4 grammar oracle and checksummed malformed fixtures."""
import hashlib,pathlib,subprocess,sys,tempfile,shutil
exe=str(pathlib.Path(sys.argv[1]).resolve())
sha=lambda b:hashlib.sha256(b).digest()
be=lambda n,k:n.to_bytes(k,'big')
get=lambda b,o,k:int.from_bytes(b[o:o+k],'big')

def probe(root,status,restored=None,tail=None):
 p=subprocess.run([exe,'probe',str(root)],capture_output=True,text=True)
 assert p.returncode==status,(root.name,p.returncode,p.stdout,p.stderr,status)
 if not status:
  rev,used,applied=map(int,p.stdout.split());assert rev==5
  if restored is not None:assert used==restored,(root.name,used)
  if tail is not None:assert applied==tail,(root.name,applied)

import json
fixed=json.loads(pathlib.Path(__file__).with_name('checkpoint_vectors.json').read_text())
empty=b'OWCHECK1'+be(1,4)+be(144,4)+be(176,8)+bytes(range(16))+be(1,8)+bytes(96)
assert (empty+sha(empty)).hex()==fixed['empty_generation1_revision0']
cases=0
with tempfile.TemporaryDirectory(prefix='ow-checkpoint-',dir='.') as temp:
 root=pathlib.Path(temp).resolve();fixture=root/'fixture'
 subprocess.run([exe,'fixture',str(fixture)],check=True)
 cp=fixture/'checkpoints'/'00000000000000000001.cp'
 data=cp.read_bytes();manifest=(fixture/'manifest.ow').read_bytes();identity=manifest[16:32]
 wal=(fixture/'wal'/'00000000000000000001.wal').read_bytes();offset=128;anchor=bytes(32)
 for _ in range(4):
  length=get(wal,offset+8,4);anchor=wal[offset+length-40:offset+length-8];offset+=length
 ns=identity+be(1,8)+be(4,8)+be(1,8)
 snapshot=b'LKS4SNP1'+bytes(4)+be(40,4)+be(2,8)+ns+be(8,8)+be(1,8)+be(8,8)+be(2,8)
 body=be(1,8)+be(0,8)+be(1,8)+be(2,8)+be(len(snapshot),8)+snapshot+be(1,8)*3+be(2,8)+be(1,8)*2
 header=b'OWCHECK1'+be(1,4)+be(144,4)+be(144+len(body)+32,8)+identity+be(1,8)+be(4,8)+be(1,8)+be(1,8)+be(2,8)+anchor+be(1,8)+be(1,8)+be(2,8)+bytes(8)
 expected=header+body;expected+=sha(expected)
 assert data==expected,'native blob and deterministic checkpoint grammar differ'
 meta=(fixture/'checkpoint.current').read_bytes()
 assert meta==(fixture/'checkpoint.current.bak').read_bytes()
 assert meta[:8]==b'OWSELECT' and get(meta,8,4)==1 and get(meta,12,4)==416
 assert meta[16:32]==identity and get(meta,32,4)==1 and get(meta,36,4)==2
 assert get(meta,48,8)==1 and get(meta,56,8)==4 and meta[64:96]==data[-32:] and meta[96:128]==anchor
 assert sha(meta[:-32])==meta[-32:]
 probe(fixture,0,1,1)
 def case(name,mutate,status=0,used=0,tail=5,reseal=True):
  global cases
  target=root/name;shutil.copytree(fixture,target)
  path=target/'checkpoints'/'00000000000000000001.cp';b=bytearray(path.read_bytes());mutate(b)
  if reseal:b[-32:]=sha(b[:-32])
  path.write_bytes(b)
  for name in ['checkpoint.current','checkpoint.current.bak']:
   p=target/name;m=bytearray(p.read_bytes());m[64:96]=b[-32:];m[-32:]=sha(m[:-32]);p.write_bytes(m)
  probe(target,status,used,tail);cases+=1;return target
 def patch(offset,value):return lambda b:b.__setitem__(slice(offset,offset+len(value)),value)
 case('wrong-identity',patch(24,bytes(16)))
 case('wrong-anchor',patch(80,bytes(32)))
 case('bad-checksum',patch(150,b'X'),reseal=False)
 case('unsupported',patch(8,be(2,4)),9,None,None)
 case('oversize-length',patch(16,be(268435457,8)))
 case('highwater-below-live',patch(72,be(1,8)))
 case('highwater-above-revision',patch(56,be(100,8)))
 case('record-zero',patch(144,bytes(8)))
 case('collection-zero',patch(160,bytes(8)))
 case('wrong-snapshot-count',patch(168,be(1,8)))
 case('truncated-native',patch(176,be(95,8)))
 case('bad-native-magic',patch(184,b'NOPE'))
 case('wrong-native-namespace',patch(208,bytes(16)))
 case('wrong-native-association-size',patch(248,be(7,8)))
 case('duplicate-native-id',patch(272,be(1,8)))
 case('missing-native-id',patch(272,be(99,8)))
 case('unknown-record-reference',patch(296,be(99,8)))
 case('unknown-collection-reference',patch(288,be(99,8)))
 case('duplicate-occurrence',patch(304,be(1,8)))
 # Invalid selection data allows safe full-WAL fallback; unsupported does not.
 for name,version,status in [('bad-meta',0,0),('future-meta',2,9)]:
  target=root/name;shutil.copytree(fixture,target)
  for filename in ['checkpoint.current','checkpoint.current.bak']:
   p=target/filename;m=bytearray(p.read_bytes());m[8:12]=be(version,4)
   if version:m[-32:]=sha(m[:-32])
   else:m[8:12]=be(1,4);m[-1]^=1
   p.write_bytes(m)
  probe(target,status,0 if not status else None,5 if not status else None);cases+=1
 target=root/'bad-tail';shutil.copytree(fixture,target);p=target/'wal'/'00000000000000000001.wal';b=bytearray(p.read_bytes());b[-41]^=1;p.write_bytes(b);probe(target,8);cases+=1
 target=root/'missing-generation';shutil.copytree(fixture,target);(target/'checkpoints'/'00000000000000000001.cp').unlink();probe(target,0,0,5);cases+=1
 target=root/'backup-only';shutil.copytree(fixture,target);(target/'checkpoint.current').write_bytes(b'bad');probe(target,0,1,1);cases+=1
 target=root/'wrong-meta-anchor';shutil.copytree(fixture,target)
 for filename in ['checkpoint.current','checkpoint.current.bak']:
  p=target/filename;b=bytearray(p.read_bytes());b[96:128]=bytes(32);b[-32:]=sha(b[:-32]);p.write_bytes(b)
 probe(target,8);cases+=1
 # Same revision does not authorize reuse when the backup digest differs.
 for label,bad in [('duplicate-revision-valid-anchor',False),('duplicate-revision-wrong-anchor',True)]:
  target=root/label;shutil.copytree(fixture,target)
  p=target/'checkpoint.current.bak';b=bytearray(p.read_bytes());b[48:56]=be(2,8)
  if bad:b[96:128]=bytes(32)
  b[-32:]=sha(b[:-32]);p.write_bytes(b)
  probe(target,8 if bad else 0,None if bad else 0,None if bad else 5);cases+=1
print(f'{cases} checkpoint corruption/fallback cases and independent native-byte grammar verified')
