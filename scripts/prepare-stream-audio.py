#!/usr/bin/env python3
"""Convert OWNED PC CD streamed audio offline; no game data is distributed by source."""
import argparse,pathlib,re,subprocess,json,hashlib
p=argparse.ArgumentParser();p.add_argument('re3',type=pathlib.Path);p.add_argument('owned_audio',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);p.add_argument('--ffmpeg',required=True);a=p.parse_args()
s=(a.re3/'src/audio/sampman.h').read_text();table=s[s.rindex('static char StreamedNameTable[][25] ='):];table=table[:table.index('};')]
names=re.findall(r'"AUDIO\\\\([^"\n]+)"',table);assert len(names)>100
files={f.name.lower():f for f in a.owned_audio.iterdir() if f.is_file()};a.output.mkdir(parents=True,exist_ok=True);results=[]
for i,name in enumerate(names):
 src=files.get(name.lower());dest=a.output/f'{i:03}.pcm'
 if not src:results.append(dict(id=i,name=name,missing=True));continue
 subprocess.run([a.ffmpeg,'-v','error','-y','-i',str(src),'-vn','-ac','1','-ar','22050','-f','s16le',str(dest)],check=True)
 n=dest.stat().st_size;assert n>0 and n%2==0
 results.append(dict(id=i,name=name,samples=n//2,milliseconds=n*1000//44100,source_sha256=hashlib.sha256(src.read_bytes()).hexdigest(),pcm_sha256=hashlib.sha256(dest.read_bytes()).hexdigest()))
(a.output/'manifest.json').write_text(json.dumps(dict(format='mono signed 16-bit little endian PCM',rate=22050,tracks=results),indent=2)+'\n')
print('Converted',sum('missing' not in r for r in results),'/',len(results),'tracks;',sum(f.stat().st_size for f in a.output.glob('*.pcm')),'bytes')
