import pathlib,sys,tempfile,subprocess,hashlib,shutil
exe=str(pathlib.Path(sys.argv[1]).resolve())
be=lambda n,k:n.to_bytes(k,'big')
get=lambda b,o,k:int.from_bytes(b[o:o+k],'big')
sha=lambda b:hashlib.sha256(b).digest()
def run(*args):return subprocess.run([exe,*map(str,args)],capture_output=True,text=True)
def frame(op,revision,payload,prev):
 h=b'OWTXN003'+be(136+len(payload),4)+be(3,2)+be(op,2)+be(revision,8)+be(len(payload),4)+bytes(4)+prev
 b=h+sha(h)+payload
 return b+sha(b)+b'COMMIT03'
with tempfile.TemporaryDirectory(prefix='ow-audit-',dir='.') as temp:
 base=pathlib.Path(temp).resolve();original=base/'original';p=run('fixture',original);assert p.returncode==0,p.stderr
 for case_name,bad_op,bad_payload in [('semantic-missing',2,be(99,8)),('semantic-id-gap',1,be(99,8)),('semantic-noop',4,be(1,8)+be(7,4)+b'missing'),('semantic-occurrence',9,be(99,8))]:
  bad=base/case_name;shutil.copytree(original,bad)
  walfile=bad/'wal'/'00000000000000000001.wal';wal=walfile.read_bytes();offset=128;prev=bytes(32);frames=[];anchor=None
  while offset<len(wal):
   n=get(wal,offset+8,4);f=wal[offset:offset+n];rev=get(f,16,8);op=get(f,14,2);payload=f[96:-40]
   if rev==2:op=bad_op;payload=bad_payload
   f=frame(op,rev,payload,prev);prev=f[-40:-8];frames.append(f)
   if rev==4:anchor=prev
   offset+=n
  walfile.write_bytes(wal[:128]+b''.join(frames))
  checkpoint=bad/'checkpoints'/'00000000000000000001.cp';cp=bytearray(checkpoint.read_bytes());cp[80:112]=anchor;cp[-32:]=sha(cp[:-32]);checkpoint.write_bytes(cp)
  for name in ['checkpoint.current','checkpoint.current.bak']:
   path=bad/name;m=bytearray(path.read_bytes());m[64:96]=cp[-32:];m[96:128]=anchor;m[-32:]=sha(m[:-32]);path.write_bytes(m)
  p=run('probe',bad);assert p.returncode==8,('semantically invalid pre-checkpoint commit accepted',p.returncode,p.stdout,p.stderr)
 long=base/'long-name';shutil.copytree(original,long);(long/'checkpoints'/('x'*190+'.cp')).write_bytes(b'x')
 p=run('backup',long,base/'backup');assert p.returncode==8,('unexpected checkpoint name not rejected safely',p.returncode,p.stderr)
print('independently checksummed semantic prefix and unexpected backup filename rejected')
