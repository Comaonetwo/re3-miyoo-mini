#!/usr/bin/env python3
"""Prepare a private Kuruma and collision block from owner's GTA III PC files."""
import argparse,pathlib,json,struct,subprocess,math,tempfile,hashlib
from txd_decode import decode_txd
from surface_data import rich_row,pack_surface
from col_decode import decode_col,surfaces
p=argparse.ArgumentParser();p.add_argument('game',type=pathlib.Path);p.add_argument('exporter',type=pathlib.Path)
p.add_argument('--rich',action='store_true');p.add_argument('city_manifest',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args();a.exporter=a.exporter.resolve()
files={str(f.relative_to(a.game)).lower().replace('\\','/'):f for f in a.game.rglob('*') if f.is_file()}
def get(name):return files[name.lower().replace('\\','/')]
manifest=json.loads(a.city_manifest.read_text());origin=manifest['center'];selected={(m['model'],tuple(m['position'])) for m in manifest['models']}
collisions={}
for key,path in sorted(files.items()):
 if key in ('models/coll/indust.col','models/coll/commer.col','models/coll/suburb.col','models/coll/generic.col'):
  for name,model in decode_col(path.read_bytes(),{name for name,position in selected}).items():
   if name in collisions and collisions[name]!=model:raise ValueError('COL conflict '+name)
   collisions[name]=model
collision_triangles=[];collision_models=[];missing=[]
for line in get('data/gta3.dat').read_text().splitlines():
 if not line.upper().startswith('IPL '):continue
 active=False
 for row in get(line[4:].strip()).read_text().splitlines():
  row=row.strip()
  if row=='inst':active=True;continue
  if row=='end':active=False
  if not active or row.startswith('#'):continue
  v=[s.strip() for s in row.split(',')]
  if len(v)!=12:continue
  name=v[1].lower();values=list(map(float,v[2:]));x,y,z=values[:3]
  if (name,(x,y,z)) not in selected:continue
  if name not in collisions:missing.append(name);continue
  sx,sy,sz,qx,qy,qz,qw=values[3:]
  if (sx,sy,sz)!=(1,1,1):raise ValueError('COL scale unsupported')
  length=math.sqrt(qx*qx+qy*qy+qz*qz);axis=[v/length for v in (qx,qy,qz)] if length>1e-12 else [0,0,1]
  angle=-2*math.acos(max(-1,min(1,qw)));c,s=math.cos(angle),math.sin(angle)
  def transform(v):
   u=axis;dot=sum(a*b for a,b in zip(u,v));cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
   r=[v[i]*c+cross[i]*s+u[i]*dot*(1-c) for i in range(3)]
   return r[0]+x-origin[0],r[2]+z,r[1]+y-origin[1]
  start=len(collision_triangles)
  for t in surfaces(collisions[name]):collision_triangles.append(struct.pack('<9f',*(n for v in t for n in transform(v))))
  collision_models.append(dict(model=name,triangles=len(collision_triangles)-start))
if not collision_triangles or len(collision_triangles)>100000:raise ValueError('collision triangle budget')
a.output.mkdir(parents=True,exist_ok=True);col=a.output/'chinatown.m3c'
col.write_bytes(struct.pack('<8sI',b'M3COL001',len(collision_triangles))+b''.join(collision_triangles))
# PC IMG v1 and the owner's original wheel model.
directory=get('models/gta3.dir').read_bytes();entries={}
for i in range(0,len(directory),32):
 offset,size,name=struct.unpack_from('<II24s',directory,i);entries[name.split(b'\0')[0].decode().lower()]=(offset*2048,size*2048)
with tempfile.TemporaryDirectory(prefix='gta3-kuruma-') as tmp,get('models/gta3.img').open('rb') as img:
 def extract(name):
  offset,size=entries[name];img.seek(offset);raw=img.read(size)
  if size>16*1024*1024 or len(raw)!=size:raise ValueError('invalid IMG entry')
  return raw
 body=pathlib.Path(tmp)/'kuruma.dff';body.write_bytes(extract('kuruma.dff'));out=pathlib.Path(tmp)/'body.txt'
 subprocess.run([str(a.exporter),str(body),str(out),'--rich' if a.rich else '--textures','--vehicle'],check=True)
 wheel=pathlib.Path(tmp)/'wheel.txt'
 subprocess.run([str(a.exporter),str(get('models/generic/wheels.dff')),str(wheel),'--rich' if a.rich else '--textures','--atomic','wheel_saloon_l0','--local'],check=True)
 def rows(path):
  result=[]
  for line in path.read_text().splitlines():
   if a.rich:result.append(rich_row(json.loads(line)));continue
   parts=line.split(maxsplit=18);texture,address=parts[18].rsplit(' ',1)
   result.append(list(map(float,parts[:18]))+[texture.lower(),int(address)])
  return result
 body_rows=rows(out);wheel_rows=rows(wheel);nodes={}
 for line in out.with_suffix(out.suffix+'.nodes').read_text().splitlines():
  parts=line.split();nodes[parts[0]]=list(map(float,parts[1:]))
 dictionaries={'kuruma':decode_txd(extract('kuruma.txd')),'generic':decode_txd(get('models/generic.txd').read_bytes()),'misc':decode_txd(get('models/misc.txd').read_bytes())}
 textures=[];texids={};tris=[];groups=[];component_report=[]
 def texture_id(name):
  if name=='-':return 65535
  for dic,textures_dic in dictionaries.items():
   if name not in textures_dic:continue
   key=(dic,name)
   if key not in texids:
    texids[key]=len(textures);w,h,data=textures_dic[name];textures.append(struct.pack('<II',w,h)+data)
   return texids[key]
  raise ValueError('missing vehicle texture '+name)
 components=[('body',body_rows,None)]+[(name,wheel_rows,nodes[name]) for name in ('wheel_lf_dummy','wheel_rf_dummy','wheel_lb_dummy','wheel_rb_dummy')]
 for name,rows_,matrix in components:
  start=len(tris);points=[]
  for row in rows_:
   vertices=[]
   for i in (0,3,6):
    v=row[i:i+3]
    if matrix:
     if name.startswith('wheel_l'):v[0]=-v[0]
     v=[matrix[j]+.7*sum(v[k]*matrix[3+k*3+j] for k in range(3)) for j in range(3)]
    vertices.append([v[0],v[2],v[1]])
   points+=vertices
   # Vehicle has no world lights yet. Use material/prelit with mild neutral shading.
   r,g,b=[max(0,min(255,int(v*.85))) for v in row[9:12]];color=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
   tid=texture_id(row[18]);address=row[19];u=(address>>8)&15;v=(address>>12)&15
   packed=struct.pack('<9fH6fHBB',*(c for pt in vertices for c in pt),color,*row[12:18],tid,u,v)
   if a.rich:
    normals=[]
    for n in row[20]['normals']:
     n=n[:]
     if matrix:
      if name.startswith('wheel_l'):n[0]=-n[0]
      n=[sum(n[k]*matrix[3+k*3+j] for k in range(3)) for j in range(3)]
     normals.append([n[0],n[2],n[1]])
    packed+=pack_surface(row[20],normals,paint=True)
   tris.append(packed)
  lo=[min(p[j] for p in points) for j in range(3)];hi=[max(p[j] for p in points) for j in range(3)];center=[(l+h)/2 for l,h in zip(lo,hi)];radius=math.sqrt(sum(((h-l)/2)**2 for l,h in zip(lo,hi)))
  groups.append(struct.pack('<II4f',start,len(tris)-start,*center,radius));component_report.append(dict(name=name,begin=start,count=len(tris)-start,bounds=[lo,hi]))
 car=a.output/'kuruma.m3s';car.write_bytes(struct.pack('<8sI4fII',b'M3SCENE3' if a.rich else b'M3SCENE2',len(tris),0,0,0,0,len(groups),len(textures))+b''.join(textures)+b''.join(groups)+b''.join(tris))
report=dict(format='M3SCENE3' if a.rich else 'M3SCENE2',vehicle='kuruma',vehicle_triangles=len(tris),textures=len(textures),components=component_report,collision_triangles=len(collision_triangles),collision_models=collision_models,missing_collision_models=sorted(set(missing)),sphere_approximation='8 longitude x 4 latitude',limitations=['prototype dynamics, not re3 physics','no damage/passengers','static wheels'])
(a.output/'driving.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Kuruma: {len(tris)} triangles, {len(textures)} textures, {len(groups)} components')
print(f'Collision: {len(collision_triangles)} triangles, {len(collision_models)} instances; missing models {len(set(missing))}')
