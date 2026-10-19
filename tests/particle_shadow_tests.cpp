#include "software_device.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 rng(3501);unsigned cases=0;uint64_t particle=0,fade=0;
 for(unsigned w:{240u,320u,640u})for(bool linear:{false,true})for(unsigned blend:{1u,2u,3u})for(unsigned ref:{0u,1u,128u})for(unsigned alpha:{0u,127u,255u}){
  Texture t;t.width=t.height=8;t.decodedOpaque=blend!=1;for(unsigned n=0;n<64;++n){t.pixels.push_back(n%4?uint16_t(rng()):0);t.alpha.push_back(t.decodedOpaque?255:uint8_t(rng()));}
  Renderer fast,slow;fast.effectSpansEnabled(true);slow.effectSpansEnabled(false);fast.additiveCacheEnabled(true);slow.additiveCacheEnabled(true);fast.blackAddEnabled(true);slow.blackAddEnabled(true);fast.clear(21943);slow.clear(21943);fast.beginHud(w);slow.beginHud(w);
  ScreenState s;s.linear=linear;s.screenDepth=true;s.zTest=true;s.zWrite=false;s.blend=blend;s.alphaRef=ref;s.addressU=s.addressV=1;
  ScreenVertex v[3]={{3,3,1,-.17f,-.19f,blend==1?0.f:173.f,blend==1?0.f:231.f,blend==1?0.f:37.f,float(alpha),.1f},{303,13,1,2.7f,-.19f,blend==1?0.f:173.f,blend==1?0.f:231.f,blend==1?0.f:37.f,float(alpha),.1f},{17,233,1,-.17f,2.79f,blend==1?0.f:173.f,blend==1?0.f:231.f,blend==1?0.f:37.f,float(alpha),.1f}};
  for(unsigned n=0;n<3;++n){fast.screenTriangle(v,&t,s);slow.screenTriangle(v,&t,s);assert(fast.pixels()==slow.pixels()&&fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels&&fast.stats().spanSkippedPixels==slow.stats().spanSkippedPixels);}particle+=fast.stats().particleEffectPixels;fade+=fast.stats().blackFadePixels;++cases;
 }
 assert(particle&&fade);
 Renderer r;SoftwareDevice d(r);Camera c;c.position={0,0,0};d.begin(c);
 auto raster=rw::Raster::create(4,4,32,rw::Raster::TEXTURE|rw::Raster::C8888);auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);assert(bytes);for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){unsigned i=(y*4+x)*4;bytes[i]=30+x*20;bytes[i+1]=y*50;bytes[i+2]=130;bytes[i+3]=x*85;}raster->unlock(0);
 rw::SetRenderStatePtr(rw::TEXTURERASTER,raster);rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::LINEAR);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::ZWRITEENABLE,0);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
 rw::cpu::Im3DVertex verts[3];verts[0].position={-2,-2,5};verts[1].position={2,-2,5};verts[2].position={0,2,5};for(unsigned i=0;i<3;++i){verts[i].setColor(0,0,0,160);verts[i].u=i==1?.973f:.017f;verts[i].v=i==2?.981f:.019f;}
 auto draw=[&](){rw::im3d::Transform(verts,3,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();};
 d.effectFiltering(false);r.clear(29143);draw();auto nearest=r.pixels();d.effectFiltering(true);r.clear(29143);draw();auto filtered=r.pixels();assert(nearest!=filtered);assert(r.stats().blackFadePixels>0);r.effectSpansEnabled(false);r.clear(29143);draw();assert(filtered==r.pixels());d.end();raster->destroy();assert(d.unsupported()==0);
 printf("PASS %u particle/black-shadow frames, depth/blending/alpha/UI; actual Im3D linear shadow differs from nearest and exact fast fade\n",cases);
}
