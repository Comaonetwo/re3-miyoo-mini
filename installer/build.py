#!/usr/bin/env python3
"""Build native macOS universal and Windows x64 installers on macOS.
First run build_components.py; dependencies are checksum pinned.
"""
from pathlib import Path
import argparse,shutil,subprocess,plistlib,zipfile,json,os,hashlib
p=argparse.ArgumentParser();p.add_argument('--work',required=True,type=Path);p.add_argument('--zig',required=True,type=Path);p.add_argument('--port-zip',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
project=Path(__file__).resolve().parents[1];w=a.work.resolve();out=a.output.resolve();zig=a.zig.resolve();out.mkdir(parents=True,exist_ok=True)
version=__import__('re').search(r'GTA3_PORT_VERSION\s+"([0-9.]+)"',(project/'src/port_version.h').read_text()).group(1)
env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(w/'zig-cache');env['CLANG_MODULE_CACHE_PATH']=str(w/'clang-cache')
for arch in['arm64','x86_64']:
 target=w/('MacInstaller-'+arch);subprocess.run(['xcrun','swiftc','-swift-version','5','-parse-as-library','-module-cache-path',str(w/('swift-cache-'+arch)),'-Xcc','-fmodules-cache-path='+str(w/('swift-cache-'+arch)),'-target',arch+'-apple-macosx13.0',str(project/'installer/native/MacInstaller.swift'),'-o',str(target)],env=env,check=True)
subprocess.run(['lipo','-create',str(w/'MacInstaller-arm64'),str(w/'MacInstaller-x86_64'),'-output',str(w/'MacInstaller-universal')],check=True)
rc=w/'ui-manifest.rc';rc.write_text('1 24 "'+str(project/'installer/native/windows.manifest')+'"\n');resource=w/'ui-manifest.res'
subprocess.run([str(zig),'rc','/fo',str(resource),'--',str(rc)],env=env,check=True)
subprocess.run([str(zig),'cc','-target','x86_64-windows-gnu','-O2','-municode','-mwindows',str(project/'installer/native/WindowsInstaller.c'),str(resource),'-o',str(w/'WindowsInstaller.exe'),'-lcomctl32','-lcomdlg32','-lshell32','-lole32','-luuid','-lgdi32'],env=env,check=True)

def backend(dst,tag):
 dst.mkdir(parents=True)
 (dst/'VERSION').write_text(version+'\n')
 for name,src in [('backend.py',project/'installer/backend.py'),('prepare-game.py',project/'scripts/prepare-game.py'),('stream-names.json',project/'scripts/stream-names.json'),('disc-layout.json',project/'installer/disc-layout.json'),('port.zip',a.port_zip)]:shutil.copy2(src,dst/name)
 shutil.copytree(project/'installer/vendor',dst/'vendor',ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
 shutil.copytree(w/('runtime-'+tag)/'python',dst/'runtime',symlinks=True)
 runtime=dst/'runtime';lib=runtime/('Lib'if tag=='win-x64'else'lib/python3.13')
 for f in[runtime/'include',runtime/'libs',runtime/'share',runtime/'tcl',runtime/'Scripts',lib/'site-packages',lib/'test',lib/'idlelib',lib/'tkinter',lib/'ensurepip',lib/'turtledemo']:
  if f.is_dir():shutil.rmtree(f)
 if tag!='win-x64':
  for f in(runtime/'bin').iterdir():
   if f.name!='python3.13':f.unlink()
  for f in list((runtime/'lib').iterdir()):
   if any(n in f.name for n in['tk','tcl','itcl','thread','pkgconfig']):
    if f.is_dir():shutil.rmtree(f)
    else:f.unlink()
 else:
  for pattern in['*tk*','*tcl*']:
   for f in(runtime/'DLLs').glob(pattern):f.unlink()
 for f in list(runtime.rglob('__pycache__')):shutil.rmtree(f,ignore_errors=True)
 (dst/'tools').mkdir()
 for name in['ffmpeg','unshield']:shutil.copy2(w/('tools-'+tag)/(name+('.exe'if tag=='win-x64'else'')),dst/'tools'/(name+('.exe'if tag=='win-x64'else'')))
 notices=dst/'licenses';notices.mkdir()
 for f in(project/'installer/third_party').iterdir():
  if f.is_dir()and f.name.endswith('-licenses'):shutil.copytree(f,notices/f.name)
  elif f.is_file():shutil.copy2(f,notices/f.name)
 shutil.copy2(project/'LICENSE',notices/'INSTALLER-LICENSE.txt');shutil.copy2(project/'installer/NOTICE.md',notices/'NOTICE.md');shutil.copy2(project/'installer/dependencies.json',notices/'dependencies.json');shutil.copy2(project/'installer/build_components.py',notices/'build_components.py')

mac=out/'macOS';mac.mkdir();app=mac/'GTA III Installer.app';contents=app/'Contents';(contents/'MacOS').mkdir(parents=True);res=contents/'Resources';b=res/'backend';backend(b,'mac-arm64');shutil.copy2(w/'MacInstaller-universal',contents/'MacOS/GTAInstaller')
# Create a universal Python interpreter and library. Pure Python files are shared.
for f in(b/'runtime').rglob('*'):
 if not f.is_file()or f.is_symlink():continue
 with f.open('rb')as stream:magic=stream.read(4)
 other=w/'runtime-mac-x64/python'/f.relative_to(b/'runtime')
 if magic in[b'\xcf\xfa\xed\xfe',b'\xce\xfa\xed\xfe']and other.is_file():
  temp=f.with_name(f.name+'.fat');subprocess.run(['lipo','-create',str(f),str(other),'-output',str(temp)],check=True);temp.replace(f)
for name in['ffmpeg','unshield']:subprocess.run(['lipo','-create',str(w/'tools-mac-arm64'/name),str(w/'tools-mac-x64'/name),'-output',str(b/'tools'/name)],check=True)
info={'CFBundleExecutable':'GTAInstaller','CFBundleIdentifier':'org.re3.miyoo.installer','CFBundleName':'GTA III Installer','CFBundleDisplayName':'GTA III Installer','CFBundlePackageType':'APPL','CFBundleShortVersionString':version,'CFBundleVersion':'1','LSMinimumSystemVersion':'13.0','NSHighResolutionCapable':True,'NSRemovableVolumesUsageDescription':'Install the port and your own game data on the selected Miyoo SD card.'}
(contents/'Info.plist').write_bytes(plistlib.dumps(info));subprocess.run(['codesign','--force','--deep','--sign','-',str(app)],check=True);subprocess.run(['codesign','--verify','--deep','--strict',str(app)],check=True)
shutil.copy2(project/'installer/README.md',mac/'README.md')
win=out/'Windows';win.mkdir();backend(win/'backend','win-x64');shutil.copy2(w/'WindowsInstaller.exe',win/'GTA III Installer.exe');shutil.copy2(project/'installer/README.md',win/'README.md')
# macOS zip preserves signatures/modes; Windows zip is a normal portable folder.
maczip=out/('gta3-installer-v'+version+'-macos.zip');winzip=out/('gta3-installer-v'+version+'-windows-x64.zip')
assert not maczip.exists()and not winzip.exists()
subprocess.run(['ditto','-c','-k','--sequesterRsrc','--keepParent',str(mac),str(maczip)],check=True)
with zipfile.ZipFile(winzip,'w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
 for f in sorted(win.rglob('*')):
  if f.is_file():z.write(f,'GTA III Installer/'+f.relative_to(win).as_posix())
for archive in[maczip,winzip]:
 with zipfile.ZipFile(archive)as z:assert z.testzip()is None
print(json.dumps({f.name:{'bytes':f.stat().st_size,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()}for f in[maczip,winzip]},indent=2))
