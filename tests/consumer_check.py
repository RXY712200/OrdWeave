import pathlib,subprocess,sys,tempfile
source=pathlib.Path(sys.argv[1]).resolve();generator=sys.argv[2];config=sys.argv[3] or 'Release'
with tempfile.TemporaryDirectory(prefix='ow-consumer-',dir='.') as root:
 build=pathlib.Path(root).resolve()
 args=['cmake','-S',str(source/'tests'/'consumer'),'-B',str(build),'-G',generator,'-DORDWEAVE_SOURCE_DIR='+str(source),'-DCMAKE_BUILD_TYPE='+config]
 if 'Visual Studio' in generator:args+=['-A','x64']
 subprocess.run(args,check=True)
 subprocess.run(['cmake','--build',str(build),'--config',config],check=True)
 subprocess.run(['ctest','--test-dir',str(build),'-C',config,'--output-on-failure'],check=True)
