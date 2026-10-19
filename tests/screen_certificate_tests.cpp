#include "soft_renderer.hpp"
#include <random>
#include <cassert>
#include <cstdio>
using namespace soft;
static void draw(Renderer& r,const Texture* t,unsigned blend,unsigned address,unsigned alpha,float uv){
 ScreenVertex v[4]={{10.13f,13.37f,1,uv,uv,173,201,233,float(alpha),0},{91.93f,13.37f,1,uv+1.9f,uv,173,201,233,float(alpha),0},{10.13f,78.71f,1,uv,uv+1.9f,173,201,233,float(alpha),0},{91.93f,78.71f,1,uv+1.9f,uv+1.9f,173,201,233,float(alpha),0}};
 ScreenState s;s.linear=true;s.screenDepth=true;s.zTest=s.zWrite=true;s.blend=blend;s.alphaRef=blend%2;s.addressU=s.addressV=address;
 ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,t,s);r.screenTriangle(b,t,s);
}
int main(){std::mt19937 rng(2907);unsigned count=0;
 for(unsigned w:{240u,320u,640u})for(unsigned size:{1u,3u,8u})for(unsigned pattern=0;pattern<4;++pattern)for(unsigned address=1;address<=4;++address)for(unsigned blend=0;blend<=5;++blend){
  Texture t;t.width=t.height=size;t.decodedUniform=pattern<2;t.decodedOpaque=pattern%2==0;
  for(unsigned i=0;i<size*size;++i){t.pixels.push_back(t.decodedUniform?rgb(237,199,147):uint16_t(rng()));t.alpha.push_back(t.decodedOpaque?255:uint8_t(i%3==0?0:rng()%256));}
  Renderer fast,slow;slow.screenCertificatesEnabled(false);slow.opaqueOverlayEnabled(false);fast.clear(rgb(30,60,90));slow.clear(rgb(30,60,90));fast.beginHud(w);slow.beginHud(w);
  for(unsigned alpha:{0u,127u,255u}){draw(fast,&t,blend,address,alpha,-.173f);draw(slow,&t,blend,address,alpha,-.173f);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);}
  ++count;
 }
 // Overlapping premultiplied layers, including zero-alpha and opaque writes.
 Renderer fast,slow;slow.opaqueOverlayEnabled(false);fast.clear(57383);slow.clear(57383);fast.beginHud(240);slow.beginHud(240);
 for(unsigned i=0;i<300;++i){unsigned alpha=i%3==0?255:i%3==1?0:rng()%256;draw(fast,nullptr,i%2,3,alpha,0);draw(slow,nullptr,i%2,3,alpha,0);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());}
 printf("PASS %u screen texture cases: RGB/depth/counts, wrap/mirror/clamp, 240/320/640, all blends, alpha 0/127/255, overlapping overlay layers\n",count);
}
