#!/usr/bin/env python3
"""Sequential native occlusion A/B measurements; requires the research patch."""
import argparse,fcntl,hashlib,json,os,pathlib,re,shutil,statistics,subprocess,tempfile
P=pathlib.Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['binary','game-template','audio-helper','audio-sdl','output']:
 p.add_argument('--'+name,required=True,type=P)
p.add_argument('--source-dir',type=P)
p.add_argument('--profile',choices=['bundle','fine','tiles'],default='bundle')
p.add_argument('--presets',nargs='+',choices=['medium','high'],default=['medium','high'])
p.add_argument('--cities',nargs='+',type=int,choices=[0,1,3],default=[0,1,3])
p.add_argument('--distance',type=int,choices=[100,140,180],default=100)
p.add_argument('--hud',action='store_true')
p.add_argument('--verify',action='store_true',help='Compare every frozen draw; do not interpret this run as a timing experiment.')
p.add_argument('--rounds',type=int,default=65)
p.add_argument('--draws',type=int,default=60)
a=p.parse_args()
if not 4<=a.rounds<=65 or not 30<=a.draws<=1000:p.error('rounds: 4..65; draws: 30..1000')
for name in ['binary','audio_helper','audio_sdl']:
 value=getattr(a,name).resolve()
 if not value.is_file():p.error(name+' is not a file')
 setattr(a,name,value)
a.game_template=a.game_template.resolve();a.output=a.output.resolve()
if not (a.game_template/'userfiles').is_dir():p.error('game-template must include a separate userfiles directory and a test save for startup-load-test')
if a.output.exists():p.error('output already exists; use a new directory')
if len(os.fsencode(str(a.output/'cases/3medium')))>115:p.error('output path is too long for re3; use a shorter path')
# A global host lock prevents two instances from contaminating one another.
lock=(P(tempfile.gettempdir())/'gta3-miyoo-native-benchmark.lock').open('a')
try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
except BlockingIOError:p.error('another native benchmark is running')
def sha(f):return hashlib.sha256(f.read_bytes()).hexdigest()
files=[a.binary]
if a.source_dir:
 root=a.source_dir.resolve();files+=[f for f in (root/'src').rglob('*')if f.is_file()and f.suffix in ['.cpp','.hpp','.h']]
 files += [f for f in [root/'CMakeLists.txt',root/'sources.lock.json']if f.is_file()]
fingerprint={str(f):sha(f)for f in files};a.output.mkdir(parents=True);(a.output/'fingerprint.json').write_text(json.dumps(fingerprint,indent=2)+'\n')
env={k:v for k,v in os.environ.items()if not k.startswith('GTA3_')};env.update(GTA3_AUDIO_HELPER=str(a.audio_helper),GTA3_AUDIO_SDL=str(a.audio_sdl),SDL_AUDIODRIVER='dummy',GTA3_PROFILE_BASE_FILTER='19',GTA3_FILTER_STAGES='19',GTA3_PROFILE_DRAWS=str(a.draws),GTA3_PROFILE_ROUNDS=str(a.rounds))
if a.verify:env['GTA3_PROFILE_EVERY_DRAW']='1'
env.setdefault('ASAN_OPTIONS','detect_leaks=0')
flag={'bundle':'--occlusion-bundle-profile-test','fine':'--fine-depth-profile-test','tiles':'--depth-tile-profile-test'}[a.profile]
results=[]
for city in a.cities:
 for preset in a.presets:
  name=str(city)+preset;game=a.output/'cases'/name;game.mkdir(parents=True)
  # Resource links are read by the engine. Userfiles are always real copies,
  # even if the template points to the owner's original save directory.
  for item in a.game_template.iterdir():
   if item.name=='userfiles':shutil.copytree(item,game/item.name,symlinks=False)
   else:(game/item.name).symlink_to(item.resolve(),target_is_directory=item.is_dir())
  runenv=env.copy();runenv['GTA3_PROBE_CITY']=str(city)
  flags=['--world','30',str(a.output/(name+'.ppm')),'--auto-input','--startup-load-test','--stationary-test','--bridge-shadow-test','--preset-'+preset,flag]
  if a.distance!=100:flags+=['--distance'+str(a.distance)]
  if a.hud:flags+=['--full-frame-profile-test']
  log=a.output/(name+'.log')
  with log.open('wb')as output:run=subprocess.run([str(a.binary),str(game)]+flags,env=runenv,stdout=output,stderr=subprocess.STDOUT,timeout=900)
  text=log.read_text(errors='replace');assert run.returncode==0,(name,'kernel failed; inspect '+str(log))
  expected='CUSTOM'if a.distance!=100 else preset.upper();assert re.search(r'SETTINGS scale=3 distance='+str(a.distance)+r' graphics_match='+expected,text),(name,'effective preset/distance mismatch')
  assert 'PASS early visibility: RGB/depth/fragments identical'in text,(name,'missing frozen validation')
  for signature in (['FINE DEPTH PROFILE','DEPTH TILE PROFILE']if a.profile=='bundle'else['FINE DEPTH PROFILE']if a.profile=='fine'else['DEPTH TILE PROFILE']):assert signature in text,(name,'unsupported diagnostic')
  assert not any(x in text for x in ['Kernel failed','RE3 ASSERT','CPU Device unsupported','runtime error:','ERROR: AddressSanitizer']),(name,'validation failed')
  item=dict(city=city,preset=preset,distance=a.distance,hud=a.hud,profile=a.profile)
  if a.verify:item['verified_draws']=(a.rounds*2-1)*a.draws
  else:
   rows=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall(r'EFFECT PROFILE[^\n]+',text)]
   assert len(rows)==2*(a.rounds-1),(name,'incomplete timing series')
   pairs=[(n['render_ms']/o['render_ms']-1)*100 for o in rows if not o['fast']for n in rows if n['fast']and n['round']==o['round']]
   assert len(pairs)==a.rounds-1
   item.update(off_ms=statistics.median(x['render_ms']for x in rows if not x['fast']),on_ms=statistics.median(x['render_ms']for x in rows if x['fast']),paired_median_percent=statistics.median(pairs),paired_percent=pairs)
  results.append(item);(a.output/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS',name,'verified'if a.verify else str(round(item['paired_median_percent'],2))+'% render time',flush=True)
assert fingerprint=={str(f):sha(f)for f in files},'source or binary changed while measuring'
print('PASS unchanged source/binary; resource links and private save copies remain in the output directory')
