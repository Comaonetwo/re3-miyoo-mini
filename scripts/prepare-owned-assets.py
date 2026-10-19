#!/usr/bin/env python3
"""Read the owner's installed PC GTA III files; export a bounded flat-color block.
Never fetches game assets. LOD/alpha/textures/collision are intentionally limited.
"""
import argparse, json, math, pathlib, struct, subprocess, tempfile, hashlib
from txd_decode import decode_txd
from surface_data import rich_row,pack_surface
p=argparse.ArgumentParser()
p.add_argument('game',type=pathlib.Path);p.add_argument('exporter',type=pathlib.Path);p.add_argument('output',type=pathlib.Path)
p.add_argument('--center',nargs=2,type=float,default=[940.,-740.]);p.add_argument('--radius',type=float,default=140.)
p.add_argument('--rich',action='store_true',help='Preserve original vertex RGBA/normals/material alpha');p.add_argument('--textures',action='store_true');p.add_argument('--texture-size',type=int,choices=[32,64,128],default=64)
a=p.parse_args();a.textures=a.textures or a.rich;a.exporter=a.exporter.resolve()
if not 10<=a.radius<=250:p.error('radius must be 10..250')
files={str(f.relative_to(a.game)).replace('\\','/').lower():f for f in a.game.rglob('*') if f.is_file()}
def get(name):return files[name.replace('\\','/').lower()]
entries={};directory=get('models/gta3.dir').read_bytes();img=get('models/gta3.img')
if len(directory)%32:raise ValueError('invalid IMG v1 directory')
for i in range(0,len(directory),32):
 offset,size,name=struct.unpack_from('<II24s',directory,i)
 entries[name.split(b'\0')[0].decode('ascii').lower()]=(offset*2048,size*2048)
model_txd={}
if a.textures:
 ide_paths=['data/default.ide']+[line[4:].strip() for line in get('data/gta3.dat').read_text().splitlines() if line.upper().startswith('IDE ')]
 for ide in ide_paths:
  active=False
  for row in get(ide).read_text().splitlines():
   row=row.strip()
   if row in ('objs','tobj'):active=True;continue
   if row=='end':active=False
   if not active or row.startswith('#'):continue
   cols=[v.strip() for v in row.split(',')]
   if len(cols)>=3:model_txd[cols[1].lower()]=cols[2].lower()
placements=[]
for line in get('data/gta3.dat').read_text(errors='strict').splitlines():
 if not line.upper().startswith('IPL '):continue
 active=False
 for row in get(line[4:].strip()).read_text().splitlines():
  row=row.strip()
  if row=='inst':active=True;continue
  if row=='end':active=False
  if not active or row.startswith('#'):continue
  v=[x.strip() for x in row.split(',')]
  if len(v)!=12:raise ValueError('unsupported IPL record')
  name=v[1].lower();nums=list(map(float,v[2:]));x,y,z=nums[:3]
  if name.startswith('lod') or (x-a.center[0])**2+(y-a.center[1])**2>a.radius**2:continue
  if name+'.dff' not in entries:raise ValueError('missing '+name+'.dff')
  placements.append((name,nums))
triangles=[];groups=[];report=[];cache={};dictionaries={};texture_ids={};texture_records=[];texture_report=[];missing=set();textured_triangles=0
with tempfile.TemporaryDirectory(prefix='gta3-owner-') as tmp, img.open('rb') as archive:
 def dictionary(name):
  if name in dictionaries:return dictionaries[name]
  if name+'.txd' in entries:
   offset,size=entries[name+'.txd'];archive.seek(offset);raw=archive.read(size)
  elif 'models/'+name+'.txd' in files:raw=get('models/'+name+'.txd').read_bytes()
  else:raise ValueError('missing TXD dictionary '+name)
  dictionaries[name]=decode_txd(raw,a.texture_size);return dictionaries[name]
 def texture_id(model,texture):
  if texture=='-':return 65535
  txd=model_txd[model]
  for key in (txd,'generic'):
   dic=dictionary(key)
   if texture not in dic:continue
   pair=(key,texture)
   if pair not in texture_ids:
    w,h,raw=dic[texture];texture_ids[pair]=len(texture_records)
    texture_records.append(struct.pack('<II',w,h)+raw)
    texture_report.append(dict(dictionary=key,name=texture,width=w,height=h))
   return texture_ids[pair]
  missing.add((model,txd,texture));return 65535
 for name,nums in placements:
  if name not in cache:
   offset,size=entries[name+'.dff']
   if size>16*1024*1024 or offset+size>img.stat().st_size:raise ValueError('invalid IMG range')
   archive.seek(offset);raw=archive.read(size)
   path=pathlib.Path(tmp)/'model.dff';out=pathlib.Path(tmp)/'model.txt';path.write_bytes(raw)
   subprocess.run([str(a.exporter),str(path),str(out)]+(['--rich'] if a.rich else ['--textures'] if a.textures else []),check=True)
   cache[name]=[]
   for l in out.read_text().splitlines():
    if a.rich:cache[name].append(rich_row(json.loads(l)));continue
    fields=l.split()
    if a.textures:
     values=l.split(maxsplit=18)
     if len(values)!=19:raise ValueError(f'invalid textured exporter record in {name}')
     texture,address=values[18].rsplit(' ',1)
     cache[name].append(list(map(float,values[:18]))+[texture.lower(),int(address)])
    else:cache[name].append(list(map(float,fields)))
  x,y,z,sx,sy,sz,qx,qy,qz,qw=nums
  # re3 FileLoader: rotate around normalized axis by -2*acos(w), then translate.
  # The original loader reads but ignores IPL scale; all selected records use 1.
  if (sx,sy,sz)!=(1.,1.,1.):raise ValueError('non-unit IPL scale')
  qlen=math.sqrt(qx*qx+qy*qy+qz*qz);theta=-2*math.acos(max(-1.,min(1.,qw)))
  axis=[v/qlen for v in (qx,qy,qz)] if qlen>1e-12 else [0,0,1]
  ct,st=math.cos(theta),math.sin(theta)
  def transform(v):
   dot=sum(a*b for a,b in zip(axis,v));u=axis
   cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
   r=[v[i]*ct+cross[i]*st+u[i]*dot*(1-ct) for i in range(3)]
   return [r[0]+x-a.center[0],r[2]+z,r[1]+y-a.center[1]] # Z-up -> Y-up
  start=len(triangles);points=[]
  for row in cache[name]:
   vs=[transform(row[j:j+3]) for j in (0,3,6)];points.extend(vs)
   ab=[vs[1][j]-vs[0][j] for j in range(3)];ac=[vs[2][j]-vs[0][j] for j in range(3)]
   normal=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]]
   length=math.sqrt(sum(v*v for v in normal))
   # Two-sided directional flat lighting; no invented textures.
   light=.55+.45*abs(sum(v*l for v,l in zip(normal,[.35,.85,.39])))/length if length>1e-8 else .55
   r,g,b=[max(0,min(255,int(v*light))) for v in row[9:12]]
   color=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
   packed=struct.pack('<9fH',*(v for pt in vs for v in pt),color)
   if a.textures:
    tid=texture_id(name,row[18]);mode=row[19];u=(mode>>8)&15;v=(mode>>12)&15
    if u>4 or v>4:raise ValueError('unsupported texture address mode')
    packed+=struct.pack('<6fHBB',*row[12:18],tid,u,v)
    if tid!=65535:textured_triangles+=1
   if a.rich:
    zero=transform([0,0,0]);normals=[[transform(n)[j]-zero[j] for j in range(3)] for n in row[20]["normals"]]
    packed+=pack_surface(row[20],normals)
   triangles.append(packed)
  if not points:continue
  lo=[min(v[j] for v in points) for j in range(3)];hi=[max(v[j] for v in points) for j in range(3)]
  center=[(l+h)/2 for l,h in zip(lo,hi)];radius=math.sqrt(sum(((h-l)/2)**2 for l,h in zip(lo,hi)))
  groups.append(struct.pack('<II4f',start,len(triangles)-start,*center,radius))
  report.append(dict(model=name,triangles=len(triangles)-start,position=[x,y,z]))
if not triangles or len(triangles)>100000:raise ValueError('scene outside 1..100000 triangle budget')
# Start on the Chinatown road, facing north. Relative to the chosen district center.
spawn=[900.772-a.center[0],16.,-830-a.center[1],0.]
a.output.parent.mkdir(parents=True,exist_ok=True)
if len(texture_records)>512:raise ValueError('texture count budget exceeded')
texture_data=(struct.pack('<I',len(texture_records))+b''.join(texture_records)) if a.textures else b''
magic=b'M3SCENE3' if a.rich else b'M3SCENE2' if a.textures else b'M3SCENE1'
a.output.write_bytes(struct.pack('<8sI4fI',magic,len(triangles),*spawn,len(groups))+texture_data+b''.join(groups)+b''.join(triangles))
manifest=dict(format=magic.decode(),surface_properties=a.rich,textures=texture_report,textured_triangles=textured_triangles,missing_textures=sorted(missing),source='Owner-provided PC GTA III CD',center=a.center,radius=a.radius,spawn=spawn,
 triangles=len(triangles),objects=len(groups),bytes=a.output.stat().st_size,sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),
 limitations=['nearest 64px textures and binary alpha' if a.textures else 'flat colors, no textures','highest triangle-count map LOD','no gameplay or collision'],models=report)
a.output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
if a.textures:print(f'Textures: {len(texture_records)}; textured triangles: {textured_triangles}; missing references: {len(missing)}')
print(f'Prepared {len(groups)} objects / {len(triangles)} triangles / {a.output.stat().st_size} bytes')
