#!/usr/bin/env python3
"""Prepare personally owned classic PC GTA III data, separately from the port.
Requires Python 3 and FFmpeg on the computer, not on Miyoo. Never downloads assets.
"""
import argparse,hashlib,json,pathlib,shutil,subprocess,sys,os

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('installed_game',type=pathlib.Path,help='Installed classic PC GTA III directory (not a CD image)')
 p.add_argument('output',type=pathlib.Path,help='New output directory, normally named game')
 p.add_argument('--ffmpeg',default='ffmpeg',help='FFmpeg executable or full path')
 p.add_argument('--preconverted-audio',type=pathlib.Path,help='Reuse audio_pcm from your own working port instead of converting')
 a=p.parse_args();src=a.installed_game.resolve();out=a.output.resolve()
 if not src.is_dir():p.error('The source must be an installed classic PC game directory.')
 if out.exists():p.error('Output already exists. Use a new directory; existing data will not be overwritten.')
 if src==out or src in out.parents:p.error('Output must be outside the source game directory.')
 dirs={f.name.lower():f for f in src.iterdir()if f.is_dir()}
 needed=['models','data','text','anim','txd','audio']
 if any(n not in dirs for n in needed):p.error('Missing source folders: '+', '.join(n for n in needed if n not in dirs))
 sources={}
 for n in needed:
  for f in dirs[n].rglob('*'):
   if f.is_symlink():p.error('Symlinks are not supported in game data: '+str(f))
   if not f.is_file():continue
   rel=pathlib.PurePosixPath(n,*[s.lower()for s in f.relative_to(dirs[n]).parts]).as_posix()
   if rel in sources:p.error('Case-insensitive duplicate path: '+rel)
   sources[rel]=f
 required=['models/gta3.img','models/gta3.dir','models/coll/peds.col','data/gta3.dat','data/main.scm','anim/ped.ifp','text/american.gxt','audio/sfx.raw','audio/sfx.sdt']
 for n in required:
  if n not in sources or sources[n].stat().st_size==0:p.error('Missing or empty original file: '+n)
 names=json.loads((pathlib.Path(__file__).with_name('stream-names.json')).read_text())['names']
 if len(names)!=196:raise ValueError('Incorrect stream filename metadata')
 ffmpeg=None
 if a.preconverted_audio:
  pcm=a.preconverted_audio.resolve()
  if not pcm.is_dir():p.error('Preconverted audio directory does not exist.')
  for i in range(196):
   f=pcm/f'{i:03}.pcm'
   if not f.is_file()or f.stat().st_size==0 or f.stat().st_size%2:p.error('Missing or invalid preconverted track: '+str(f))
 else:
  ffmpeg=shutil.which(a.ffmpeg)
  if not ffmpeg:p.error('FFmpeg was not found. Install it on this computer or supply --ffmpeg PATH.')
  missing=[n for n in names if 'audio/'+n.lower()not in sources]
  if missing:p.error('Incomplete classic PC audio: '+', '.join(missing)+'. Install the full game/audio from your own media first.')
 out.mkdir(parents=True);written=0
 for n,f in sorted(sources.items()):
  if n.startswith('audio/') and n not in ('audio/sfx.raw','audio/sfx.sdt'):continue
  if pathlib.PurePosixPath(n).name in ('txd.img','txd.dir','caps.dat'):continue # PC generated texture caches are incompatible.
  dest=out/n;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,dest);written+=1
 audio=out/'audio_pcm';audio.mkdir();tracks=[]
 for i,name in enumerate(names):
  dest=audio/f'{i:03}.pcm'
  if a.preconverted_audio:shutil.copy2(pcm/dest.name,dest)
  else:subprocess.run([ffmpeg,'-v','error','-n','-i',str(sources['audio/'+name.lower()]),'-vn','-ac','1','-ar','22050','-f','s16le',str(dest)],check=True,creationflags=0x08000000 if os.name=='nt' else 0)
  size=dest.stat().st_size
  if size==0 or size%2:raise ValueError('Invalid decoded PCM: '+str(dest))
  tracks.append({'id':i,'name':name,'samples':size//2,'milliseconds':size*1000//44100,'pcm_sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
  if (i+1)%20==0 or i==195:print(f'Audio {i+1}/196',flush=True)
 (audio/'manifest.json').write_text(json.dumps({'format':'mono signed 16-bit little endian PCM','rate':22050,'tracks':tracks},indent=2)+'\n')
 print(f'Ready: {out}\nCopied {written} game files and prepared 196 audio tracks.\nKeep this directory private. Copy it into Roms/PORTS/Games/GTA3Core14/.')

if __name__=='__main__':
 try:main()
 except (OSError,ValueError,subprocess.CalledProcessError)as e:
  print('Preparation failed: '+str(e)+'\nAn incomplete output may remain; use a new output directory when retrying.',file=sys.stderr);sys.exit(1)
