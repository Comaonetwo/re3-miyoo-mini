#!/usr/bin/env python3
"""Sequential native A/B of early world proxies; keep fixtures/logs private."""
import argparse,fcntl,hashlib,json,os,pathlib,re,shutil,statistics,subprocess,tempfile
p=argparse.ArgumentParser(description=__doc__)
for name in ['binary','game-template','audio-helper','audio-sdl','output']:
 p.add_argument('--'+name,type=pathlib.Path,required=True)
p.add_argument('--balanced-bands',choices=['on','off'],default='on')
p.add_argument('--batch-limit',type=int,choices=[64,128,256,512,1024,2048,4096],default=512)
p.add_argument('--deferred-phases',choices=['geometry','world'],default='geometry')
p.add_argument('--world-width',type=int,choices=[0,200,280],default=0)
p.add_argument('--interleaved',action='store_true')
p.add_argument('--controls-test',action='store_true')
p.add_argument('--probe-only',action='store_true')
p.add_argument('--mode',choices=['world','particles','channels','flat','skycolor','next','skyblend','expand','pass54','rainblend','tilescan','pass55','budget1','budget2','budget3','autorain','deferred1','deferred2','deferred3'],default='world')
p.add_argument('--effect',choices=['none','fire','smoke','rain'],default='none')
p.add_argument('--probe-distance',type=float,default=16)
p.add_argument('--probe-size',type=float,default=3)
p.add_argument('--cities',nargs='+',type=int,choices=range(6),default=[0,1,2,3,4,5])
p.add_argument('--presets',nargs='+',choices=['medium','high','ultra'],default=['medium','high'])
p.add_argument('--distance',type=int,choices=[100,120,140,180],default=100)
p.add_argument('--draws',type=int,choices=range(60,1001),default=60)
p.add_argument('--rounds',type=int,choices=range(4,66),default=33)
a=p.parse_args()
if a.interleaved and a.mode not in ['rainblend','tilescan','pass55','budget1','budget2','budget3','autorain']:p.error('--interleaved requires rainblend, tilescan or pass55')
root=pathlib.Path(__file__).resolve().parents[1]
for key in ['binary','game_template','audio_helper','audio_sdl','output']:setattr(a,key,getattr(a,key).resolve())
lock=open(pathlib.Path(tempfile.gettempdir())/'gta3-world-proxy-profile.lock','w');fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
if a.output.exists():raise SystemExit('Choose a new output directory; existing results are preserved.')
for city in a.cities:
 for preset in a.presets:
  if len(str(a.output/(str(city)+preset[0])))>115:raise SystemExit('Use a shorter output path: re3 FileMgr limits fixture paths to 115 bytes.')
if not (a.game_template/'userfiles').is_dir():raise SystemExit('The game template needs its own userfiles directory.')
a.output.mkdir(parents=True)
files=[a.binary,*sorted((root/'src').glob('*.*')),root/'scripts/prepare-re3-cpu.py',pathlib.Path(__file__).resolve()]
def fingerprint():return {str(f):hashlib.sha256(f.read_bytes()).hexdigest()for f in files}
before=fingerprint();(a.output/'fingerprint.json').write_text(json.dumps(before,indent=2)+'\n')
env={k:v for k,v in os.environ.items()if not k.startswith('GTA3_')}
env.update(GTA3_PROFILE_FIXED='1',GTA3_AUDIO_HELPER=str(a.audio_helper),GTA3_AUDIO_SDL=str(a.audio_sdl),SDL_AUDIODRIVER='dummy',GTA3_WORLD_APPEARANCE_PROFILE='1',GTA3_WORLD_PROFILE_ROUNDS=str(a.rounds),GTA3_WORLD_PROFILE_DRAWS=str(a.draws))
if a.mode=='particles':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_PARTICLE_DENSITY_PROFILE']='1';env['GTA3_EFFECT_PROBE_DISTANCE']=str(a.probe_distance);env['GTA3_EFFECT_PROBE_SIZE']=str(a.probe_size)
if a.mode.startswith('deferred'):
 env['GTA3_BALANCED_BANDS']='1'if a.balanced_bands=='on'else'0'
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_DEFERRED_PROFILE']=a.mode[-1];env['GTA3_DEFERRED_LIMIT']=str(a.batch_limit);env['GTA3_DEFERRED_ALL']='1'if a.deferred_phases=='world'else'0'
if a.mode=='channels':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_WORLD_CHANNEL_PROFILE']='1'
if a.mode=='flat':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_FLAT_SPAN_PROFILE']='1'
if a.mode in ['skycolor','next']:
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_SKY_CHANNEL_PROFILE'if a.mode=='skycolor'else'GTA3_NEXT_PROFILE']='1'
if a.mode=='skyblend':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_SKY_COMPOSITE_PROFILE']='1'
if a.mode in ['expand','pass54']:
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_ROW_EXPAND_PROFILE'if a.mode=='expand'else'GTA3_PASS54_PROFILE']='1'
if a.mode=='rainblend':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_RAIN_COMPOSITE_PROFILE']='1'
if a.mode=='tilescan':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_TILE_SCAN_PROFILE']='1'
if a.mode=='pass55':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_PASS55_PROFILE']='1'
if a.mode=='autorain':
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_AUTO_RAIN_PROFILE']='1'
if a.mode.startswith('budget'):
 env.pop('GTA3_WORLD_APPEARANCE_PROFILE',None);env['GTA3_BUDGET_PROFILE']=a.mode[-1]
if a.interleaved:env['GTA3_PROFILE_INTERLEAVED']='1'
if a.world_width:env['GTA3_WORLD_TEST_WIDTH']=str(a.world_width)
if a.controls_test:env['GTA3_CONTROLS_SELFTEST']='1'
if a.probe_only:env['GTA3_DENSITY_PROBE_ONLY']='1'
results=[]
for city in a.cities:
 for preset in a.presets:
  name=str(city)+preset[0];game=a.output/name
  game.mkdir()
  # Resource links stay read-only in practice; saves/settings get private copies.
  for item in a.game_template.iterdir():
   target=game/item.name
   if item.name=='userfiles':shutil.copytree(item.resolve(),target,symlinks=False)
   elif item.is_symlink():target.symlink_to(item.resolve(),target_is_directory=item.is_dir())
   elif item.is_dir():shutil.copytree(item,target,symlinks=True)
   else:shutil.copy2(item,target)
  command=[str(a.binary),str(game),'--world','30',str(a.output/(name+'.ppm')),'--auto-input','--startup-load-test','--stationary-test','--bridge-shadow-test','--preset-'+preset]
  if a.effect!='none':command+=['--'+{'fire':'fire-effect-test','smoke':'smoke-effect-test','rain':'rain-effect-test'}[a.effect]]
  if a.distance!=100:command+=['--distance'+str(a.distance)]
  local=env.copy();local['GTA3_PROBE_CITY']=str(city)
  log=a.output/(name+'.log')
  with log.open('wb')as stream:run=subprocess.run(command,env=local,stdout=stream,stderr=subprocess.STDOUT,timeout=900)
  text=log.read_text(errors='replace')
  if run.returncode or any(x in text for x in ['Kernel failed','RE3 ASSERT','CPU Device unsupported','runtime error:','ERROR: AddressSanitizer']):raise RuntimeError('Failed case '+name+'; inspect '+str(log))
  match='CUSTOM'if a.distance!=100 else preset.upper()
  assert re.search(r'SETTINGS scale=\d+ distance='+str(a.distance)+' graphics_match='+match+r'\b',text),name+' effective settings'
  if a.controls_test:assert 'PASS Miyoo native controls:'in text
  assert ('PASS auto rain: world depth/particles/RNG/streak state unchanged; visual differences allowed'if a.mode=='autorain'else'PASS budget profile: particles/RNG unchanged; visual differences allowed; state restored'if a.mode.startswith('budget')else'PASS v055 full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='pass55'else'PASS tile scan full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='tilescan'else'PASS rain composite full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='rainblend'else'PASS row expansion full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode in ['expand','pass54']else'PASS sky composite full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='skyblend'else'PASS next renderer full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode in ['skycolor','next']else'PASS flat spans full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='flat'else'PASS world channels full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode=='channels'else'PASS deferred full frame: RGB/depth/particles/RNG/fragments unchanged; state restored'if a.mode.startswith('deferred')else'PASS particle density: simulation/depth unchanged; state restored'if a.mode=='particles'else'PASS appearance profile state restored')in text
  rows=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall(('DENSITY'if a.mode=='particles'else'APPEARANCE')+' PROFILE[^\n]+',text)]
  assert len(rows)==2*a.rounds
  paired=[]
  for pair_round in range(1,a.rounds):
   off=next(v for v in rows if v['round']==pair_round and not v['fast']);on=next(v for v in rows if v['round']==pair_round and v['fast'])
   paired.append(100*((on['paired_ratio']if a.interleaved else on['render_ms']/off['render_ms'])-1))
  result=dict(automatic_control_suspended=True,metric="median adjacent-frame paired ratio"if a.interleaved else"ratio of block means",interleaved=a.interleaved,draws=a.draws,batch_limit=a.batch_limit,balanced_bands=a.balanced_bands,deferred_phases=a.deferred_phases,mode=a.mode,effect=a.effect,probe_distance=a.probe_distance,probe_size=a.probe_size,city=city,preset=preset,distance=a.distance,median_percent=statistics.median(paired),paired_percent=paired,rows=rows)
  if a.mode=='autorain':result['rain_budget']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('RAIN BUDGET[^\n]+',text)]
  if a.mode.startswith('budget'):result['budget_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('BUDGET COUNTERS[^\n]+',text)]
  if a.mode in ['tilescan','pass55']:result['tile_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('TILE SCAN COUNTERS[^\n]+',text)]
  if a.mode in ['rainblend','pass55']:result['rain_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('RAIN COMPOSITE COUNTERS[^\n]+',text)]
  if a.mode in ['expand','pass54']:result['expand_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('ROW EXPAND COUNTERS[^\n]+',text)]
  if a.mode in ['skyblend','pass54']:result['composite_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('SKY COMPOSITE COUNTERS[^\n]+',text)]
  if a.mode in ['skycolor','next']:result['sky_counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('SKY COLOR COUNTERS[^\n]+',text)]
  if a.mode in ['flat','next']:result['counters']=[dict((k,float(v))for k,v in re.findall(r'(\w+)=(-?\d+(?:\.\d+)?)',line))for line in re.findall('FLAT SPAN COUNTERS[^\n]+',text)]
  results.append(result);(a.output/'results.json').write_text(json.dumps(results,indent=2)+'\n')
  print(name,'full-frame render time',round(result['median_percent'],2),'%; negative is faster; native only',flush=True)
assert before==fingerprint(),'Source or executable changed during measurement'
print('PASS sequential run, effective settings, private userfiles, source/executable fingerprints',flush=True)
