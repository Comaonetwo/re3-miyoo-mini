#!/usr/bin/env python3
"""Offline two-disc installer. All preparation happens on the computer.
The SD card is changed only after data preparation succeeds. No downloads.
"""
import argparse,hashlib,json,os,pathlib,queue,shutil,signal,subprocess,sys,tempfile,threading,time,uuid,zipfile
BASE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(BASE/'vendor'))
import pycdlib
class InstallError(Exception):pass
class Cancelled(InstallError):pass
CANCEL=None

def emit(percent,message):
 print('PROGRESS\t'+str(int(percent))+'\t'+message.replace('\n',' '),flush=True)
def check_cancel():
 if CANCEL is not None and CANCEL.exists():raise Cancelled('Installation cancelled. Your existing installation was kept.')
def digest(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  while True:
   b=f.read(1024*1024)
   if not b:break
   check_cancel();h.update(b)
 return h.hexdigest()
def list_iso(path):
 iso=pycdlib.PyCdlib()
 try:
  iso.open(str(path));files={}
  for parent,dirs,names in iso.walk(iso_path='/'):
   check_cancel()
   for name in names:
    full=parent.rstrip('/')+'/'+name;leaf=name.split(';')[0].lower()
    if leaf in files:files[leaf]=None # Reject ambiguous cabinet/stream names.
    else:files[leaf]=full
  return files
 except (OSError,pycdlib.pycdlibexception.PyCdlibException)as e:raise InstallError('Cannot read this ISO image. Use the original two-disc PC edition.')from e
 finally:iso.close()
def extract_iso(path,entries,out,limit=2*1024**3):
 iso=pycdlib.PyCdlib();total=0
 try:
  iso.open(str(path));out.mkdir(parents=True,exist_ok=True)
  for leaf,full in entries.items():
   check_cancel()
   if not full or pathlib.PurePosixPath(leaf).name!=leaf or any(c in leaf for c in '\\/:\x00'):raise InstallError('Unsupported or ambiguous disc layout.')
   length=iso.get_record(iso_path=full).data_length
   total+=length
   if total>limit:raise InstallError('Disc contents exceed the supported edition size.')
   iso.get_file_from_iso(str(out/leaf),iso_path=full)
 finally:iso.close()
def run(cmd,log,progress=None):
 flags=0x08000000|0x00000200 if os.name=='nt' else 0
 proc=subprocess.Popen([str(x)for x in cmd],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8',errors='replace',start_new_session=os.name!='nt',creationflags=flags)
 q=queue.Queue()
 def reader():
  for line in proc.stdout:q.put(line)
  q.put(None)
 th=threading.Thread(target=reader,daemon=True);th.start();ended=False
 try:
  while not ended or proc.poll()is None:
   check_cancel()
   try:line=q.get(timeout=.1)
   except queue.Empty:continue
   if line is None:ended=True;continue
   log.write(line);log.flush()
   if progress:progress(line)
  th.join();code=proc.wait()
  if code:raise InstallError('A preparation tool failed. See the installation log for details.')
 except BaseException:
  if proc.poll()is None:
   if os.name=='nt':subprocess.run(['taskkill','/PID',str(proc.pid),'/T','/F'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,creationflags=0x08000000)
   else:
    try:os.killpg(proc.pid,signal.SIGTERM)
    except ProcessLookupError:pass
   try:proc.wait(timeout=5)
   except subprocess.TimeoutExpired:proc.kill();proc.wait()
  raise
 finally:proc.stdout.close()
def validate_card(card,test=False):
 if not card.is_dir()or card.is_symlink():raise InstallError('Select the root of the mounted Miyoo SD card.')
 if not test:
  if os.name=='nt':
   if card!=pathlib.Path(card.anchor):raise InstallError('Select the SD drive root, not a subfolder.')
  elif not os.path.ismount(card):raise InstallError('Select the mounted SD card itself, not a subfolder.')
 if not(card/'.tmp_update').is_dir()or not(card/'Emu/PORTS/launch_standalone.sh').is_file():raise InstallError('Onion Ports was not found. Enable Ports Collection on the card first.')
 if any((card/n).is_symlink()for n in ['Roms','Roms/PORTS','Roms/PORTS/Games','Roms/PORTS/Imgs','Roms/PORTS/Shortcuts']):raise InstallError('Symlinked Ports directories are not supported.')
 return card.stat().st_dev

def copy_checked(src,dst):
 dst.parent.mkdir(parents=True,exist_ok=True)
 h=hashlib.sha256()
 with src.open('rb')as a,dst.open('xb')as b:
  while True:
   check_cancel();chunk=a.read(1024*1024)
   if not chunk:break
   b.write(chunk);h.update(chunk)
  b.flush();os.fsync(b.fileno())
 shutil.copystat(src,dst)
 if src.stat().st_size!=dst.stat().st_size or digest(dst)!=h.hexdigest():raise InstallError('Copy verification failed. Check the SD card.')

def commit_card(card,prepared,port,identity,test=False,fault=None):
 """Same-volume staged swap with rollback; keep old installation as backup."""
 check_cancel()
 manifest=json.loads((prepared/'audio_pcm/manifest.json').read_text())
 if len(manifest['tracks'])!=196:raise InstallError('Incomplete prepared audio.')
 for t in manifest['tracks']:
  if digest(prepared/'audio_pcm'/('%03d.pcm'%t['id']))!=t['pcm_sha256']:raise InstallError('Prepared audio verification failed.')
 if validate_card(card,test)!=identity:raise InstallError('The selected card changed or was disconnected.')
 need=sum(f.stat().st_size for root in(prepared,port)for f in root.rglob('*')if f.is_file())+128*1024**2
 if shutil.disk_usage(card).free<need:raise InstallError('Not enough free space on the SD card for a staged installation.')
 lock=card/'.gta3-installer.lock'
 try:lock.mkdir()
 except FileExistsError:raise InstallError('Another or unfinished installation was found on this card. Check .gta3-installer.lock before retrying.')
 stage=card/('.gta3-install-'+uuid.uuid4().hex[:10]);target=card/'Roms/PORTS/Games/GTA3Core14';backup=target.with_name('.GTA3Core14-backup-'+uuid.uuid4().hex[:10]);swapped=False;old_moved=False;aux=[];recovery_needed=False
 try:
  stage.mkdir();new=stage/'GTA3Core14';shutil.copytree(port/'Roms/PORTS/Games/GTA3Core14',new)
  (new/'game').mkdir(exist_ok=True)
  source_files=[f for f in prepared.rglob('*')if f.is_file()]
  for i,f in enumerate(source_files):
   if f.is_symlink():raise InstallError('Symlinked game files are not supported.')
   copy_checked(f,new/'game'/f.relative_to(prepared))
   if i%10==0:emit(75+18*i/max(1,len(source_files)),'Copying prepared data to the SD card...')
  # Preserve user state independently of engine binaries and original resources.
  if target.is_symlink():raise InstallError('Symlinked installations are not supported.')
  for src,dst in [(target/'settings.cfg',new/'settings.cfg'),(target/'game/userfiles',new/'game/userfiles')]+[(f,new/'game'/f.name)for f in (target/'game').glob('*.b')]:
   if src.exists():
    if src.is_symlink():raise InstallError('Symlinked user files are not supported.')
    if src.is_dir():
     if any(f.is_symlink()for f in src.rglob('*')):raise InstallError('Symlinked user files are not supported.')
     shutil.copytree(src,dst)
    else:shutil.copy2(src,dst)
  check_cancel()
  if validate_card(card,test)!=identity:raise InstallError('The SD card was disconnected.')
  target.parent.mkdir(parents=True,exist_ok=True)
  # Journal remains on the card if power is lost; the old folder is kept.
  (stage/'RECOVERY.txt').write_text('Target: '+str(target)+'\nPrevious installation: '+str(backup)+'\nPrepared installation: '+str(new)+'\n')
  emit(94,'Finishing installation. Please keep the card connected...')
  if target.exists():target.rename(backup);old_moved=True
  new.rename(target);swapped=True
  if fault:fault('after_game_swap')
  for section in ['Shortcuts','Imgs']:
   for src in(port/'Roms/PORTS'/section).iterdir():
    if not src.is_file():continue
    dest=card/'Roms/PORTS'/section/src.name;dest.parent.mkdir(parents=True,exist_ok=True)
    if dest.is_symlink():raise InstallError('Symlinked menu files are not supported.')
    previous=dest.read_bytes()if dest.exists()else None;aux.append((dest,previous))
    temp=dest.with_name(dest.name+'.gta3-tmp');shutil.copy2(src,temp);os.replace(temp,dest)
    if fault:fault('after_aux')
  # Each data file was read back and hashed before the atomic swap.
  manifest=json.loads((target/'game/audio_pcm/manifest.json').read_text())
  if len(manifest['tracks'])!=196:raise InstallError('Audio copy verification failed.')
  for t in manifest['tracks']:
   f=target/'game/audio_pcm'/('%03d.pcm'%t['id'])
   if not f.is_file()or f.stat().st_size%2:raise InstallError('Audio copy verification failed.')
  return backup if old_moved else None
 except BaseException:
  # Do not redirect rollback onto a newly mounted different volume.
  if card.exists()and card.stat().st_dev==identity:
   errors=[]
   for dest,previous in reversed(aux):
    try:
     if previous is None:dest.unlink(missing_ok=True)
     else:dest.write_bytes(previous)
    except OSError as e:errors.append(str(e))
   try:
    if swapped and target.exists():shutil.rmtree(target)
    if old_moved and backup.exists():backup.rename(target)
   except OSError as e:errors.append(str(e))
   if errors:
    recovery_needed=True
    raise InstallError('Could not complete rollback. Keep the backup at '+str(backup)+' and recovery notes at '+str(stage))
  raise
 finally:
  if card.exists()and card.stat().st_dev==identity:
   if stage.exists()and not recovery_needed:shutil.rmtree(stage,ignore_errors=True)
   if not recovery_needed:
    try:lock.rmdir()
    except OSError:pass

def install(game_iso,audio_iso,card,resources,log_path,test=False):
 check_cancel();identity=validate_card(card,test)
 if game_iso.resolve()==audio_iso.resolve():raise InstallError('Select two different ISO images: game disc and audio disc.')
 if any(not f.is_file()or f.suffix.lower()!='.iso'for f in(game_iso,audio_iso)):raise InstallError('Choose both original PC ISO images.')
 emit(2,'Checking the two discs...');g=list_iso(game_iso);a=list_iso(audio_iso)
 names=json.loads((resources/'stream-names.json').read_text())['names'];cab=json.loads((resources/'disc-layout.json').read_text())['cab_sha256']
 if not all(g.get(n)for n in cab)and all(a.get(n)for n in cab):game_iso,audio_iso=audio_iso,game_iso;g,a=a,g
 if not all(g.get(n)for n in cab):raise InstallError('The game ISO is not the supported classic two-disc PC edition.')
 missing=[n for n in set(x.lower()for x in names)if not a.get(n)]
 if missing:raise InstallError('The audio disc is incomplete or unsupported. Choose GTA3_audio.iso from the same two-disc edition.')
 exe='.exe'if os.name=='nt'else'';unshield=resources/'tools'/('unshield'+exe);ffmpeg=resources/'tools'/('ffmpeg'+exe)
 if not all(f.is_file()for f in(unshield,ffmpeg,resources/'port.zip')):raise InstallError('Installer components are missing. Extract the complete download and try again.')
 log_path.parent.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory(prefix='gta3-miyoo-')as temp,log_path.open('w',encoding='utf-8')as log:
  temp=pathlib.Path(temp)
  if shutil.disk_usage(temp).free<4*1024**3:raise InstallError('At least 4 GB of temporary space is required on this computer.')
  emit(5,'Reading the game disc...');extract_iso(game_iso,{n:g[n]for n in cab},temp/'cab')
  for n,h in cab.items():
   if digest(temp/'cab'/n)!=h:raise InstallError('This game-disc edition is not supported yet. No files were installed on the card.')
  emit(12,'Extracting the original PC game...');run([unshield,'-d',temp/'pc','x',temp/'cab/data1.cab'],log)
  pc=temp/'pc/App_Executables'
  if not pc.is_dir():raise InstallError('Unexpected game-disc layout.')
  audio=next((f for f in pc.iterdir()if f.is_dir()and f.name.lower()=='audio'),None)
  if audio is None:raise InstallError('Original sound effects were not found.')
  emit(20,'Reading music and voices from the audio disc...');extract_iso(audio_iso,{n:a[n]for n in set(x.lower()for x in names)},audio)
  def progress(line):
   if line.startswith('Audio '):
    try:done=int(line.split()[1].split('/')[0]);emit(25+45*done/196,'Preparing audio: '+str(done)+' / 196')
    except ValueError:pass
  emit(25,'Preparing game files and audio...');run([sys.executable,resources/'prepare-game.py',pc,temp/'game','--ffmpeg',ffmpeg],log,progress)
  manifest=json.loads((temp/'game/audio_pcm/manifest.json').read_text())
  if len(manifest['tracks'])!=196:raise InstallError('Incomplete audio preparation.')
  for t in manifest['tracks']:
   if digest(temp/'game/audio_pcm'/('%03d.pcm'%t['id']))!=t['pcm_sha256']:raise InstallError('Prepared audio verification failed.')
  with zipfile.ZipFile(resources/'port.zip')as z:
   for n in z.namelist():
    if n.startswith('/')or'..'in pathlib.PurePosixPath(n).parts or '\\'in n:raise InstallError('Invalid installer package.')
   z.extractall(temp/'port')
   for info in z.infolist():
    f=temp/'port'/info.filename
    if f.is_file():f.chmod((info.external_attr>>16)&0o777 or 0o644)
  emit(75,'Copying the port and game data...');backup=commit_card(card,temp/'game',temp/'port',identity,test)
  if backup:log.write('Previous installation kept at: '+str(backup)+'\n')
 emit(100,'Installed. Safely eject the card, then launch GTA III (re3) from Ports.')

def main():
 global CANCEL
 p=argparse.ArgumentParser();p.add_argument('--game',type=pathlib.Path,required=True);p.add_argument('--audio',type=pathlib.Path,required=True);p.add_argument('--card',type=pathlib.Path,required=True);p.add_argument('--resources',type=pathlib.Path,default=BASE);p.add_argument('--cancel-file',type=pathlib.Path,required=True);p.add_argument('--log',type=pathlib.Path,required=True);p.add_argument('--test-card',action='store_true',help=argparse.SUPPRESS);a=p.parse_args();CANCEL=a.cancel_file
 try:install(a.game,a.audio,a.card,a.resources,a.log,a.test_card)
 except Cancelled as e:print('CANCELLED\t'+str(e),flush=True);return 3
 except Exception as e:print('ERROR\t'+str(e).replace('\n',' ')+(' Log: '+str(a.log)if a.log.exists()else''),flush=True);return 1
 return 0
if __name__=='__main__':sys.exit(main())
