#!/usr/bin/env python3
"""Optional own menu.txd + PPM capture check; no game fixtures included.
Compare clean background samples with source bilinear pixels at 640x480.
Detects the old bug where background was drawn at world resolution first.
"""
import sys,struct,math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from txd_decode import chunks,native
source=None
for ident,body in chunks(Path(sys.argv[1]).read_bytes()):
 if ident!=22:continue
 for ident,child in chunks(body):
  if ident!=21:continue
  name,image=native(next(b for i,b in chunks(child) if i==1),1024)
  if name=='mainmenu24':source=image
assert source is not None
w,h,data=source
header,dimensions,maximum,pixels=Path(sys.argv[2]).read_bytes().split(b'\n',3)
assert header==b'P6' and dimensions==b'640 480' and maximum==b'255' and len(pixels)==640*480*3
matching=total=0
for y in range(16,360,13):
 for x in list(range(8,152,7))+list(range(475,625,7)):
  if 195<=y<=228:continue # Selected menu row overlay.
  # Sprite2d uses UV +1/1024 at both ends; bilinear subtracts half a texel.
  u=(x+.5)*w/640+w/1024-.5;v=(y+.5)*h/480+h/1024-.5
  ix=math.floor(u);iy=math.floor(v);fx=int((u-ix)*256);fy=int((v-iy)*256)
  rgb=[0,0,0]
  for dy in range(2):
   for dx in range(2):
    px=max(0,min(w-1,ix+dx));py=max(0,min(h-1,iy+dy));c,a=struct.unpack_from('<HB',data,(py*w+px)*3);assert a==255
    r=c>>11;g=(c>>5)&63;b=c&31;weight=(fx if dx else 256-fx)*(fy if dy else 256-fy)
    for i,val in enumerate([(r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)]):rgb[i]+=val*weight
  r,g,b=[(value+32768)>>16 for value in rgb];c=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
  expected=[(c>>11)*255//31,((c>>5)&63)*255//63,(c&31)*255//31];actual=pixels[(y*640+x)*3:(y*640+x)*3+3]
  matching+=all(abs(a-b)<=1 for a,b in zip(actual,expected));total+=1
print(f'Native menu source {w}x{h}: {matching}/{total} clean background samples match full resolution bilinear reference')
assert matching/total>.98,'menu background still reduced, or another overlay/format is present'
