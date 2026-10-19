"""Bounded original PC COL v1 reader; layout from re3 FileLoader LoadCollisionModel.
Returns real triangle surfaces, boxes and sphere primitives, never game assets.
"""
import struct,math

class Reader:
    def __init__(self,body):self.body=body;self.pos=0
    def unpack(self,fmt):
        fmt='<'+fmt;n=struct.calcsize(fmt)
        if self.pos+n>len(self.body):raise ValueError('truncated COL')
        result=struct.unpack_from(fmt,self.body,self.pos);self.pos+=n;return result
    def count(self):
        n=self.unpack('h2x')[0]
        if n<0:raise ValueError('negative COL count')
        return n

def decode_col(data,wanted=None):
    pos=0;models={}
    while pos<len(data):
        if len(data)-pos<8:raise ValueError('truncated COLL header')
        ident,n=struct.unpack_from('<4sI',data,pos)
        if ident!=b'COLL' or n<64 or n>len(data)-pos-8:raise ValueError('unsupported/invalid COLL record')
        body=data[pos+8:pos+8+n];name=body[:24].split(b'\0')[0].decode('ascii').lower()
        if wanted is not None and name not in wanted:pos+=8+n;continue
        r=Reader(body[24:]);bounds=r.unpack('10f')
        triangles=[];boxes=[];spheres=[]
        for _ in range(r.count()):
            radius,x,y,z,surface,piece=r.unpack('4fBB2x')
            if radius<0:raise ValueError('invalid sphere radius')
            spheres.append((radius,(x,y,z)))
        for _ in range(r.count()):r.unpack('6f') # suspension lines are not surfaces
        for _ in range(r.count()):
            v=r.unpack('6fBB2x');lo=v[:3];hi=v[3:6]
            if any(a>b for a,b in zip(lo,hi)):raise ValueError(f'invalid COL box in {name}: {lo} {hi}')
            boxes.append((lo,hi))
        vertices=[r.unpack('3f') for _ in range(r.count())]
        for _ in range(r.count()):
            a,b,c,surface,piece=r.unpack('iiiBB2x')
            if min(a,b,c)<0 or max(a,b,c)>=len(vertices):raise ValueError('COL vertex reference out of range')
            triangles.append((vertices[a],vertices[b],vertices[c]))
        if r.pos!=len(r.body):raise ValueError('unexpected COL tail')
        values=list(bounds)+[v for point in vertices for v in point]
        values += [v for lo,hi in boxes for point in (lo,hi) for v in point]
        values += [v for radius,point in spheres for v in (radius,*point)]
        if any(not math.isfinite(v) or abs(v)>100000 for v in values):raise ValueError('invalid COL coordinate')
        if name in models and models[name]!=(triangles,boxes,spheres):raise ValueError('conflicting COL model '+name)
        models[name]=(triangles,boxes,spheres);pos+=8+n
    return models

def surfaces(model):
    triangles,boxes,spheres=model
    yield from triangles
    for lo,hi in boxes:
        points=[(x,y,z) for z in (lo[2],hi[2]) for y in (lo[1],hi[1]) for x in (lo[0],hi[0])]
        for a,b,c,d in ((0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)):
            yield points[a],points[b],points[c];yield points[a],points[c],points[d]
    # Sphere collision approximated with 8 longitudinal slices / 4 latitude bands.
    # Box/mesh surfaces are exact. The sphere approximation is intentionally recorded.
    for radius,center in spheres:
        def point(i,j):
            lat=-math.pi/2+i*math.pi/4;angle=j*math.pi/4
            return center[0]+radius*math.cos(lat)*math.cos(angle),center[1]+radius*math.cos(lat)*math.sin(angle),center[2]+radius*math.sin(lat)
        for i in range(4):
            for j in range(8):
                a,b,c,d=point(i,j),point(i,j+1),point(i+1,j+1),point(i+1,j)
                if i:yield a,b,c
                if i<3:yield a,c,d
