#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(3701);uint64_t transparent=0,fused=0,builds=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<240;++n){
  Renderer a,b;a.flatAlphaEnabled(true);a.renderScale(scale);b.renderScale(scale);b.transparentWorldEnabled(false);b.flatAlphaEnabled(false);b.leanSetupEnabled(false);a.occlusionEnabled(false);b.occlusionEnabled(false);
  a.batchGeneralCountersEnabled(n%2);b.batchGeneralCountersEnabled(n%2);
  a.modulationTablesEnabled(n%3);b.modulationTablesEnabled(n%3);
  Texture t;t.width=n%13?16:15;t.height=n%17?16:13;t.pixels.resize(t.width*t.height);t.alpha.resize(t.pixels.size());
  for(unsigned j=0;j<t.pixels.size();++j){t.pixels[j]=n%11?rng():0;t.alpha[j]=n%4?255:rng()%256;}
  t.decodedOpaque=n%4;t.decodedUniform=n%11==0;
  auto bg=uint16_t(rng());a.clear(bg);b.clear(bg);
  ScreenState s;s.zTest=true;s.zWrite=n%7==0;s.blend=n%9?1:0;s.perspectiveColor=true;s.addressU=s.addressV=n%5?1:3;s.alphaRef=n%3?1:128;s.linear=n%19==0;
  // Foreground first, then multiple sorted alpha surfaces. Occluded fragments,
  // equal-Z tests and alpha rejection must preserve both image and depth.
  ScreenVertex cover[3]={{40,30,.7f,0,0,255,255,255,255,0},{180,35,.7f,1,0,255,255,255,255,0},{60,180,.7f,0,1,255,255,255,255,0}};
  ScreenState solid=s;solid.zWrite=true;solid.blend=0;a.screenTriangle(cover,&t,solid);b.screenTriangle(cover,&t,solid);
  for(unsigned k=0;k<7;++k){float cr=rng()%256,cg=rng()%256,cb=rng()%256,ca=1+rng()%254;
   ScreenVertex v[3];for(unsigned j=0;j<3;++j){v[j].x=int(rng()%660)-170;v[j].y=int(rng()%500)-120;v[j].reciprocalZ=n%3?.01f+float(rng()%1000)/1000:.7f;v[j].u=float(int(rng()%800)-400)/100;v[j].v=float(int(rng()%800)-400)/100;v[j].r=n%2?cr:rng()%256;v[j].g=n%2?cg:rng()%256;v[j].b=n%2?cb:rng()%256;v[j].a=n%6?ca:rng()%256;v[j].depthZ=0;}
   a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);
   assert(a.pixels()==b.pixels());assert(a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels);assert(a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);
  }
  transparent+=a.stats().transparentWorldPixels;fused+=a.stats().flatAlphaPixels;builds+=a.stats().flatAlphaBuilds;
 }
 assert(transparent>100000&&fused>10000&&builds>10);
 std::printf("PASS 960 layered RGB/depth/counter comparisons; transparent=%llu fused=%llu builds=%llu\n",(unsigned long long)transparent,(unsigned long long)fused,(unsigned long long)builds);
}
