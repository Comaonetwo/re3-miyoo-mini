#include "soft_renderer.hpp"
#include <random>
#include <cassert>
#include <cstdio>
using namespace soft;
static void draw(Renderer& r,const Texture* t,unsigned blend,unsigned address,unsigned alpha,float uv){
 ScreenVertex v[4]={{10.13f,13.37f,1,uv,uv,173,201,233,float(alpha),0},{91.93f,13.37f,1,uv+1.9f,uv,173,201,233,float(alpha),0},{10.13f,78.71f,1,uv,uv+1.9f,173,201,233,float(alpha),0},{91.93f,78.71f,1,uv+1.9f,uv+1.9f,173,201,233,float(alpha),0}};
 v[2].r=v[3].r=31;v[2].g=v[3].g=63;v[2].b=v[3].b=240;v[2].a=v[3].a=alpha/2;v[2].reciprocalZ=v[3].reciprocalZ=1.25f;
 ScreenState s;s.perspectiveColor=address%2;s.linear=true;s.screenDepth=true;s.zTest=s.zWrite=true;s.blend=blend;s.alphaRef=address==4?128:blend%2;s.addressU=s.addressV=address;
 ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,t,s);r.screenTriangle(b,t,s);
}
int main(){std::mt19937 rng(2907);unsigned count=0;
 for(unsigned w:{240u,320u,640u})for(unsigned size:{1u,3u,8u})for(unsigned pattern=0;pattern<4;++pattern)for(unsigned address=1;address<=4;++address)for(unsigned blend=0;blend<=5;++blend){
  Texture t;t.width=t.height=size;t.decodedUniform=pattern<2;t.decodedOpaque=pattern%2==0;
  for(unsigned i=0;i<size*size;++i){t.pixels.push_back(t.decodedUniform?rgb(237,199,147):uint16_t(i%3==0?0:rng()));t.alpha.push_back(t.decodedOpaque?255:uint8_t(i%3==0?0:rng()%256));}
  Renderer fast,slow;fast.filterStages(31);slow.vectorFilterEnabled(false);slow.additiveCacheEnabled(false);slow.blackAddEnabled(false);fast.additiveCacheEnabled(true);fast.blackAddEnabled(true);fast.clear(rgb(30,60,90));slow.clear(rgb(30,60,90));fast.beginHud(w);slow.beginHud(w);
  for(unsigned alpha:{0u,127u,255u}){draw(fast,&t,blend,address,alpha,-.173f);draw(slow,&t,blend,address,alpha,-.173f);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);}
  ++count;
 }
 // Overlapping premultiplied layers, including zero-alpha and opaque writes.
 Renderer fast,slow;fast.filterStages(31);slow.vectorFilterEnabled(false);slow.additiveCacheEnabled(false);slow.blackAddEnabled(false);fast.additiveCacheEnabled(true);fast.blackAddEnabled(true);fast.clear(57383);slow.clear(57383);fast.beginHud(240);slow.beginHud(240);
 for(unsigned i=0;i<300;++i){unsigned alpha=i%3==0?255:i%3==1?0:rng()%256;draw(fast,nullptr,i%2,3,alpha,0);draw(slow,nullptr,i%2,3,alpha,0);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());}
 // Reuse the cache across many tint/alpha keys, opaque and translucent textures,
 // filtered and nearest sampling, and depth-tested overlapping additive draws.
 unsigned additiveCases=0;
 for(unsigned width:{240u,320u,640u})for(bool linear:{false,true})for(unsigned mode:{2u,3u}){
  Renderer cached,reference;reference.vectorFilterEnabled(false);cached.additiveCacheEnabled(true);
  cached.clear(31077);reference.clear(31077);cached.beginHud(width);reference.beginHud(width);
  Texture texture;texture.width=texture.height=8;texture.decodedOpaque=true;
  for(unsigned n=0;n<64;++n){texture.pixels.push_back(uint16_t(rng()));texture.alpha.push_back(255);}
  for(unsigned n=0;n<96;++n){
   float red=rng()%256,green=rng()%256,blue=rng()%256,alpha=n%3==0?0:n%3==1?255:rng()%256;
   ScreenVertex vertices[3]={{10,10,1,-.17f,-.1f,red,green,blue,alpha,float(n)},
    {104,10,1,2.1f,-.1f,red,green,blue,alpha,float(n)},
    {10,96,1.25f,-.17f,2.3f,red,green,blue,alpha,float(n)}};
   ScreenState state;state.linear=linear;state.screenDepth=true;state.zTest=state.zWrite=true;
   state.blend=mode;state.alphaRef=n%3==0?0:n%3==1?1:128;state.addressU=state.addressV=1+n%4;
   texture.decodedOpaque=n%5!=0;
   for(unsigned t=0;t<64;++t)texture.alpha[t]=texture.decodedOpaque?255:uint8_t(t%4?127:0);
   cached.screenTriangle(vertices,&texture,state);reference.screenTriangle(vertices,&texture,state);
   assert(cached.pixels()==reference.pixels());assert(cached.depth()==reference.depth());
   assert(cached.stats().pixels==reference.stats().pixels);++additiveCases;
  }
 }
 printf("PASS %u constant-colour additive cache comparisons (key reuse, alpha rejection, opacity fallback, filtered/nearest, RGB/depth/counts)\n",additiveCases);
 printf("PASS %u exact black/additive blend cases: RGB/depth/counts, wrap/mirror/clamp, 240/320/640, all blends, alpha 0/127/255, overlapping overlay layers\n",count);
}
