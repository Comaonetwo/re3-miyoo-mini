"""Bounded M3SCENE3 surface records, from owner DFF data only."""
import math,struct

def rich_row(value):
    required={'p','uv','rgba','normals','material','ambient','diffuse','flags','texture','address'}
    if set(value)!=required:raise ValueError('invalid rich exporter fields')
    for name,n,width in [('p',3,3),('uv',3,2),('normals',3,3),('rgba',3,4)]:
        if len(value[name])!=n or any(len(v)!=width for v in value[name]):raise ValueError('invalid '+name)
    if len(value['material'])!=4:raise ValueError('invalid material color')
    for component in value['material']+[n for color in value['rgba'] for n in color]:
        if not isinstance(component,int) or not 0<=component<=255:raise ValueError('invalid RGBA byte')
    for name in ['p','uv','normals']:
        if any(not math.isfinite(n) or abs(n)>100000 for v in value[name] for n in v):raise ValueError('invalid surface float')
    if value['flags']&~15 or not all(math.isfinite(value[key]) and 0<=value[key]<=4 for key in ['ambient','diffuse']):raise ValueError('invalid surface flags/properties')
    # Preserve legacy row offsets for existing converters; the final dictionary
    # holds original properties used exclusively by the new format.
    colors=[]
    for color in value['rgba']:
        base=value['material'][:3]
        colors.append([base[i]*color[i]/255 if value['flags']&2 else color[i] for i in range(3)] if value['flags']&1 else base)
    average=[sum(c[i] for c in colors)/3 for i in range(3)]
    return [n for v in value['p'] for n in v]+average+[n for uv in value['uv'] for n in uv]+[value['texture'].lower(),value['address'],value]

def pack_surface(surface,normals,paint=False):
    material=surface['material'][:];flags=surface['flags']
    if paint and material[:3] in ([60,255,0],[255,0,175]):
        flags|=16 if material[:3]==[60,255,0] else 32
        material[:3]=[45,105,190]
    for normal in normals:
        length=math.sqrt(sum(v*v for v in normal))
        if length>1e-8:normal[:]=[v/length for v in normal]
    return struct.pack('<12B4B9f2fB',*(v for color in surface['rgba'] for v in color),*material,*(v for n in normals for v in n),surface['ambient'],surface['diffuse'],flags)
