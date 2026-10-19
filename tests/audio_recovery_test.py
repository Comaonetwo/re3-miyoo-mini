import pathlib,tempfile,mmap,os,subprocess,struct,time,sys
helper=pathlib.Path(sys.argv[1]).resolve();sdl=pathlib.Path(sys.argv[2]).resolve()
with tempfile.TemporaryDirectory(prefix='gta-clock-recovery-') as tmp:
 root=pathlib.Path(tmp);(root/'audio_pcm').mkdir();(root/'audio').mkdir();(root/'audio/sfx.raw').write_bytes(struct.pack('<h',500)*22050);(root/'audio/sfx.sdt').write_bytes(struct.pack('<IIIIi',0,44100,22050,0,-1));(root/'audio_pcm/000.pcm').write_bytes(struct.pack('<h',1000)*22050*15)
 with tempfile.TemporaryFile() as fd,(root/'log').open('wb') as log:
  fd.truncate(1248);data=mmap.mmap(fd.fileno(),1248)
  def put(i,v):struct.pack_into('<I',data,i*4,v)
  def get(i):return struct.unpack_from('<I',data,i*4)[0]
  put(21,0x1401);put(22,127);put(23,127);put(3,127);put(4,127);put(9,127);put(17,127)
  env=os.environ.copy();env.update(GTA3_AUDIO_SDL=str(sdl),GTA3_AUDIO_TEST_OUTPUT=str(root/'out.pcm'),GTA3_AUDIO_TEST_SLOW_FIRST='1',GTA3_AUDIO_WATCH_MS='500')
  child=subprocess.Popen([str(helper),str(fd.fileno())],cwd=root,pass_fds=[fd.fileno()],env=env,stdout=log,stderr=log)
  try:
   until=time.monotonic()+5
   while not get(0):assert time.monotonic()<until;time.sleep(.01)
   put(6,1);put(5,1)
   put(25,1);put(26,22050);put(27,127);put(28,0);put(30,22050);put(24,1)
   samples=[]
   for i in range(8):time.sleep(.5);samples.append(get(10))
   put(1,1);assert child.wait(timeout=3)==0
   text=(root/'log').read_text();assert text.count('AUDIO RESYNC complete')==1,text
   assert 'AUDIO RESYNC failed' not in text
   assert all(b>=a for a,b in zip(samples,samples[1:])),samples
   assert samples[-1]-samples[-3]>=800,('normal clock after recovery',samples)
   pcm=(root/'out.pcm').read_bytes();assert struct.unpack_from('<h',pcm,len(pcm)-2)[0]==1500
   assert get(12)==1 and get(32)==1 and get(11) and get(31)
  finally:
   if child.poll() is None:child.terminate();child.wait(timeout=3)
print('PASS half-speed SDL recovered once; PCM/SFX phase, positions, acknowledgements and output survive reopen')
