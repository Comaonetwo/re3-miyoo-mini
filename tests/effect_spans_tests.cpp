#include "soft_renderer.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 rng(3401);unsigned cases=0;uint64_t sprites=0,add=0;
 for(unsigned scale:{1u,2u,3u,4u})for(unsigned size:{1u,3u,8u})for(unsigned address=1;address<=4;++address)for(unsigned ref:{0u,1u,128u,255u})for(unsigned mode=0;mode<2;++mode)for(bool constant:{false,true})for(unsigned spriteBlend:{1u,2u,3u}){
  Texture t;t.width=t.height=size;t.decodedOpaque=mode==1;t.decodedUniform=mode==0;
  for(unsigned i=0;i<size*size;++i){t.pixels.push_back(mode?uint16_t(i%3?rng():0):65535);t.alpha.push_back(mode?255:uint8_t(i%4?rng()%256:0));}
  Renderer fast,slow;fast.renderScale(scale);slow.renderScale(scale);fast.effectSpansEnabled(true);slow.effectSpansEnabled(false);fast.blackAddEnabled(true);slow.blackAddEnabled(true);fast.additiveCacheEnabled(true);slow.additiveCacheEnabled(true);
  fast.clear(35143);slow.clear(35143);
  ScreenVertex v[3]={{7.37f,11.23f,.13f,-.73f,-.21f,83,197,239,255,.15f},{303.73f,17.21f,.23f,1.79f,-.21f,213,107,33,55,.25f},{59.79f,221.29f,.31f,-.73f,1.91f,31,63,233,0,.75f}};
  if(!mode)for(auto& q:v)q.reciprocalZ=.25f;
  if(constant)for(auto& q:v){q.r=83;q.g=197;q.b=239;q.a=127;}
  ScreenState s;s.blend=mode?3:spriteBlend;s.linear=!mode;s.screenDepth=!mode;s.zTest=mode;s.zWrite=false;s.alphaRef=ref;s.addressU=s.addressV=address;s.perspectiveColor=mode;
  for(unsigned n=0;n<3;++n){fast.screenTriangle(v,&t,s);slow.screenTriangle(v,&t,s);assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels);assert(fast.stats().spanSkippedPixels==slow.stats().spanSkippedPixels);}
  sprites+=fast.stats().spriteEffectPixels;add+=fast.stats().additiveWorldPixels;++cases;
 }
 assert(sprites&&add);printf("PASS %u specialized effect cases, RGB/depth/pixels/spans; sprites=%llu additive=%llu\n",cases,(unsigned long long)sprites,(unsigned long long)add);
}
