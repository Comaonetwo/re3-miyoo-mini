"""Bounded decoder for original PC RenderWare D3D8 TXDs.
Layout follows the pinned librw src/d3d/d3d8.cpp readNativeTexture/readAsImage.
Only uncompressed PAL8/PAL4 (byte indices), BGRA8888/BGR888 and 16-bit rasters.
Unknown platforms/compression/formats fail explicitly; no game assets included.
"""
import struct

def chunks(data):
    pos=0
    while pos<len(data):
        if not any(data[pos:]):return # IMG sector padding
        if len(data)-pos<12:raise ValueError('truncated RW chunk header')
        ident,size,version=struct.unpack_from('<III',data,pos)
        if size>len(data)-pos-12:raise ValueError('RW chunk exceeds parent')
        yield ident,data[pos+12:pos+12+size]
        pos+=12+size

def expand5(v):return (v<<3)|(v>>2)
def expand6(v):return (v<<2)|(v>>4)

def native(body,cap):
    if len(body)<88:raise ValueError('truncated native texture')
    platform,address=struct.unpack_from('<II',body)
    if platform!=8:raise ValueError('only PC D3D8 TXD is supported')
    name=body[8:40].split(b'\0')[0].decode('ascii').lower()
    fmt,alpha,w,h,depth,levels,typ,compression=struct.unpack_from('<IIHHBBBB',body,72)
    if not w or not h or w>2048 or h>2048 or w&(w-1) or h&(h-1) or not 1<=levels<=12:raise ValueError('invalid texture dimensions/levels')
    if compression:raise ValueError('compressed TXD is not supported by this exporter')
    pos=88;palette=None;base=fmt&0xf00
    if fmt&0x6000:
        entries=32 if fmt&0x4000 else 256
        if len(body)-pos<entries*4:raise ValueError('truncated palette')
        palette=[tuple(body[pos+i*4:pos+i*4+4]) for i in range(entries)];pos+=entries*4
        if depth not in (4,8):raise ValueError('invalid paletted depth')
    first=None
    for level in range(levels):
        if len(body)-pos<4:raise ValueError('truncated mip size')
        size=struct.unpack_from('<I',body,pos)[0];pos+=4
        if size>len(body)-pos:raise ValueError('truncated mip data')
        if level==0:first=body[pos:pos+size]
        pos+=size
    if pos!=len(body):raise ValueError('unexpected native raster tail')
    bpp=1 if palette else depth//8
    if bpp not in (1,2,3,4) or len(first)!=w*h*bpp:raise ValueError('invalid raster byte count')
    def pixel(i):
        if palette:
            index=first[i]
            if index>=len(palette):raise ValueError('palette index out of range')
            r,g,b,a=palette[index]
        elif base in (0x500,0x600) and bpp in (3,4):
            b,g,r=first[i*bpp:i*bpp+3];a=first[i*bpp+3] if bpp==4 else 255
        elif bpp==2 and base in (0x100,0x200,0x300,0xa00):
            v=struct.unpack_from('<H',first,i*2)[0]
            if base==0x200:r,g,b,a=expand5(v>>11),expand6((v>>5)&63),expand5(v&31),255
            elif base==0x300:r,g,b,a=((v>>8)&15)*17,((v>>4)&15)*17,(v&15)*17,(v>>12)*17
            else:r,g,b,a=expand5((v>>10)&31),expand5((v>>5)&31),expand5(v&31),255 if base==0xa00 or v&0x8000 else 0
        else:raise ValueError('unsupported raster format/depth')
        return r,g,b,a if alpha else 255
    nw,nh=w,h
    while max(nw,nh)>cap:nw=max(1,nw//2);nh=max(1,nh//2)
    out=bytearray()
    for y in range(nh):
        for x in range(nw):
            r,g,b,a=pixel((y*h//nh)*w+x*w//nw)
            out+=struct.pack('<HB',((r>>3)<<11)|((g>>2)<<5)|(b>>3),a)
    return name,(nw,nh,bytes(out))

def decode_txd(data,cap=64):
    if cap not in (32,64,128):raise ValueError('texture cap must be 32,64,128')
    result={};found=False
    for ident,body in chunks(data):
        if ident!=22:continue
        found=True;expected=None;native_count=0
        for ident,child in chunks(body):
            if ident==1:
                if len(child)!=4:raise ValueError('invalid TXD dictionary struct')
                expected=struct.unpack_from('<H',child)[0]
            if ident!=21:continue
            structs=[b for i,b in chunks(child) if i==1]
            if len(structs)!=1:raise ValueError('invalid native texture struct')
            name,texture=native(structs[0],cap)
            native_count+=1
            # TexDictionary::find traverses its list from the front: first name wins.
            result.setdefault(name,texture)
        if expected!=native_count:raise ValueError('TXD texture count mismatch')
    if not found:raise ValueError('missing TXD chunk')
    return result
