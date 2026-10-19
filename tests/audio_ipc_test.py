#!/usr/bin/env python3
"""Run the real audio helper against deterministic SDL; no original game audio."""
import argparse,pathlib,tempfile,mmap,os,subprocess,struct,time
p=argparse.ArgumentParser();p.add_argument('helper',type=pathlib.Path);p.add_argument('sdl_stub',type=pathlib.Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='gta-audio-') as d:
 root=pathlib.Path(d);(root/'audio_pcm').mkdir();out=root/'out.pcm'
 (root/'audio').mkdir();(root/'audio/sfx.raw').write_bytes(struct.pack('<h',500)*22050);(root/'audio/sfx.sdt').write_bytes(struct.pack('<IIIIi',0,44100,22050,0,-1))
 for i,value in enumerate((1000,2000)):(root/'audio_pcm'/f'{i:03}.pcm').write_bytes(struct.pack('<h',value)*22050)
 with tempfile.TemporaryFile() as fd:
  fd.truncate(1248);shared=mmap.mmap(fd.fileno(),1248)
  def get(i):return struct.unpack_from('<I',shared,i*4)[0]
  def put(i,v):struct.pack_into('<I',shared,i*4,v)
  def wait(test):
   until=time.monotonic()+3
   while not test():
    assert time.monotonic()<until,'audio state timeout';time.sleep(.005)
  put(21,0x1401);put(22,127);put(23,127);put(3,127);put(4,127);put(9,127);put(17,127)
  env=os.environ.copy();env.update(GTA3_AUDIO_SDL=str(a.sdl_stub.resolve()),GTA3_AUDIO_TEST_OUTPUT=str(out))
  child=subprocess.Popen([str(a.helper.resolve()),str(fd.fileno())],cwd=root,pass_fds=[fd.fileno()],env=env)
  try:
   wait(lambda:get(0)!=0);assert get(0)==1
   def start(s,track,seek=0):
    base=5+s*8;put(base+1,track+1);put(base+2,seek);put(base+3,0);put(base,get(base)+1);wait(lambda:get(base+7)==get(base))
   def sample():
    time.sleep(.08);b=out.read_bytes();return struct.unpack_from('<h',b,len(b)-2)[0]
   start(0,0);assert sample()==1000;assert 0<get(10)<1000
   put(9,64);assert sample()==1000*64//127
   put(2,1);position=get(10);assert sample()==0;assert abs(get(10)-position)<=48
   put(2,0);put(9,127);start(1,1);assert sample()==3000
   put(16,1);assert sample()==1000
   start(0,0,900);wait(lambda:get(11)==0);assert get(10)==1000
   put(6,0);put(5,get(5)+1);wait(lambda:get(12)==get(5));assert not get(11)
   # Real helper SFX commands, mapped RAW metadata and independent effects gain.
   put(25,1);put(26,22050);put(27,127);put(28,0);put(29,0);put(30,22050);sent=time.monotonic();offset=out.stat().st_size;put(24,1);wait(lambda:get(32)==1);wait(lambda:b'\xf4\x01' in out.read_bytes()[offset:]);latency_ms=(time.monotonic()-sent)*1000;assert latency_ms<150,('synthetic effect latency',latency_ms);assert sample()==500 and get(31)
   put(22,0);assert sample()==0;put(22,127);put(26,44100);assert sample()==500
   put(25,0);put(24,2);wait(lambda:get(32)==2);assert sample()==0 and not get(31)
   put(25,1);put(28,1);put(26,22050);put(24,3);wait(lambda:get(32)==3);wait(lambda:get(31)==0)
   put(1,1);assert child.wait(timeout=3)==0
  finally:
   if child.poll() is None:child.terminate();child.wait(timeout=3)
  shared.close()
print('Synthetic SFX command-to-output %.1f ms (host SDL stub, not console hardware)' % latency_ms)
print('PASS audio: SDL ABI, two streams, mixing, volume, menu/stream pause, seek, EOF, stop, 32-channel SFX commands, RAW mapping, effects gain, resampling and shutdown; synthetic PCM only')
