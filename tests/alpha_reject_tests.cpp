#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){Renderer r;std::mt19937 rng(5793);unsigned rejected=0,zeroFallback=0;
 const float alphas[]={0,.25f,1,126,127,128,200,254,255};const unsigned refs[]={0,1,2,127,128,254,255};
 for(unsigned n=0;n<1536;++n){Texture t;t.width=n%2?13:16;t.height=16;t.pixels.resize(t.width*t.height);t.alpha.resize(t.pixels.size());for(unsigned i=0;i<t.pixels.size();++i){t.pixels[i]=uint16_t(rng());t.alpha[i]=n%3?uint8_t(rng()):255;}t.decodedOpaque=n%3==0;
  float a=alphas[n%9];ScreenVertex v[3]={{15,25,.2f,-.1f,.2f,100,180,220,a,.3f},{285,15,.1f,1.2f,.3f,100,180,220,a,.5f},{85,220,.15f,.2f,1.1f,100,180,220,a,.4f}};
  if(n%5==0){v[1].r=220;v[2].g=80;}if(n%11==0)v[1].a=255;if(n%13==0){v[0].a=v[1].a=v[2].a=0;v[1].g=90;}
  if(n%17==0){v[1].x=16;v[2].y=26;}if(n%19==0)for(auto& q:v)q.reciprocalZ=0;
  ScreenState s;s.blend=n%6;s.alphaRef=refs[(n/6)%7];s.screenDepth=n%2;s.perspectiveColor=n%3==0;s.linear=n%4==0;s.zTest=n%3!=0;s.zWrite=n%5!=0;s.addressU=n%4+1;s.addressV=(n/4)%4+1;
  auto draw=[&](bool enabled){r.alphaRejectEnabled(enabled);r.renderScale(n%4+1);r.clear(rgb(10,40,80));if(n%8==0)r.beginHud(640);else if(n%8==1)r.beginHud(240);r.filterStages(n%2?19:31);r.alphaSpansEnabled(n%3);r.effectSpansEnabled(n%2);r.particleSpansEnabled(n%4);r.smallDepthEnabled(false);r.occlusionEnabled(false);
   if(n%9==0)r.beginRadar(20,20,230,230);if(n%7==0)r.beginDeferred(2,64);r.screenTriangle(v,n%10?&t:nullptr,s);r.endDeferred();r.endRadar();};
  draw(false);auto image=r.pixels();auto depth=r.depth();auto count=r.stats().pixels;assert(r.stats().alphaRejectedTriangles==0);draw(true);
  if(image!=r.pixels()||depth!=r.depth()||count!=r.stats().pixels){printf("FAIL alpha early n=%u blend=%u ref=%u persp=%u a=%g/%g/%g\n",n,s.blend,s.alphaRef,s.perspectiveColor,v[0].a,v[1].a,v[2].a);return 1;}
  rejected+=r.stats().alphaRejectedTriangles>0;zeroFallback+=s.alphaRef==0;
 }
 assert(rejected>100&&zeroFallback>100);printf("PASS 1536 paired alpha-test rejects/fallbacks, %u active rejected triangles; exact RGB/depth/core writes, all blends/targets, perspective gradients, alphaRef0, deferred replay\n",rejected);
}
