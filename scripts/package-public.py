#!/usr/bin/env python3
"""Create a fresh asset-free Onion install with user-supplied menu cover."""
import argparse,pathlib,shutil,hashlib,zipfile,struct,re,json
p=argparse.ArgumentParser();p.add_argument('kernel',type=pathlib.Path);p.add_argument('helper',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args()
project=pathlib.Path(__file__).resolve().parents[1];version=re.search(r'GTA3_PORT_VERSION\s+"([0-9.]+)"',(project/'src/port_version.h').read_text()).group(1)
for f in(a.kernel,a.helper):
 b=f.read_bytes()
 if b[:6]!=b'\x7fELF\x01\x01'or struct.unpack_from('<H',b,18)[0]!=40:raise ValueError('ARM32 ELF required: '+str(f))
b=a.kernel.read_bytes();off=struct.unpack_from('<I',b,28)[0];stride,count=struct.unpack_from('<HH',b,42)
if any(struct.unpack_from('<I',b,off+i*stride)[0]in(2,3)for i in range(count)):raise ValueError('Kernel must be static')
cover=project/'packaging-public/cover.png';b=cover.read_bytes()
if b[:8]!=b'\x89PNG\r\n\x1a\n':raise ValueError('Cover must be PNG')
w,h=struct.unpack_from('>II',b,16)
if not(0<w<=250 and 0<h<=360):raise ValueError('Cover must fit 250x360')
root=a.output
if root.exists()or pathlib.Path(str(root)+'.zip').exists():raise ValueError('Output exists; preserve old releases')
folder=root/'Roms/PORTS/Games/GTA3Core14';folder.mkdir(parents=True);(folder/'game').mkdir()
for src,name in[(a.kernel,'re3_cpu'),(a.helper,'re3_audio'),(project/'packaging-sd/re3-core-launch.sh','run.sh'),(project/'packaging-sd/re3-audio-launch.sh','audio-launch.sh')]:
 shutil.copy2(src,folder/name);(folder/name).chmod(0o755)
shutil.copytree(project/'licenses',folder/'licenses');shutil.copy2(project/'LICENSE',folder/'licenses/PORT-LICENSE.txt');shutil.copy2(project/'packaging-public/_required_files.txt',folder/'_required_files.txt')
short=root/'Roms/PORTS/Shortcuts';short.mkdir(parents=True);art=root/'Roms/PORTS/Imgs';art.mkdir()
name='GTA III (re3)';shutil.copy2(cover,art/(name+'.png'))
f=short/(name+'.port');f.write_text('''#!/bin/sh
GameName="GTA III (re3)"
GameDir="GTA3Core14"
GameExecutable="run.sh"
GameDataFile="re3_cpu"
KillAudioserver=1
PerformanceMode=0
Arguments=""
/mnt/SDCARD/Emu/PORTS/launch_standalone.sh "$GameName" "$GameDir" "$GameExecutable" "$Arguments" "$GameDataFile" "$KillAudioserver" "$PerformanceMode"
''');f.chmod(0o755)
# GameDataFile remains a basename as required by Onion's maxdepth2/-iname lookup.
# run.sh separately checks nested game files and reports missing data to Onion.
tools=root/'tools';tools.mkdir()
for name in['prepare-game.py','stream-names.json']:shutil.copy2(project/'scripts'/name,tools/name)
for name in['README.md','BUILD.md']:shutil.copy2(project/name,root/name)
(root/'VERSION').write_text(version+'\n')
files=sorted(f for f in root.rglob('*')if f.is_file());(root/'SHA256SUMS').write_text(''.join(hashlib.sha256(f.read_bytes()).hexdigest()+'  '+f.relative_to(root).as_posix()+'\n'for f in files))
with zipfile.ZipFile(str(root)+'.zip','w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():z.write(f,f.relative_to(root).as_posix())
print('Prepared install v'+version+': '+str(root)+'.zip')
