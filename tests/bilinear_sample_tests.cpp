#include "bilinear_sample.hpp"
#include "soft_renderer.hpp"
#include <random>
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 random(202010);
 for(unsigned n=0;n<300000;++n){unsigned c[4];for(auto& channel:c)channel=random()%256;unsigned x=random()%256,y=random()%256;
  unsigned sum=c[0]*(256-x)*(256-y)+c[1]*x*(256-y)+c[2]*(256-x)*y+c[3]*x*y;
  assert(bilinearChannel(c[0],c[1],c[2],c[3],x,y)==(sum+32768)/65536);
 }
 Texture t;t.width=t.height=16;t.pixels.assign(256,rgb(255,120,230));t.alpha.assign(256,0);
 ScreenVertex v[3]={{10,10,1,0,0,255,255,255,255,0},{310,10,1,1,0,255,255,255,255,0},{10,230,1,0,1,255,255,255,255,0}};
 ScreenState s;s.linear=true;s.screenDepth=true;s.alphaRef=1;s.zWrite=true;Renderer r;r.clear(rgb(20,40,60));auto before=r.pixels();r.screenTriangle(v,&t,s);assert(before==r.pixels());assert(r.stats().filteredAlphaOnly>10000);for(auto z:r.depth())assert(z==0);
 // With alphaRef=0 and replacement blending, transparent RGB still matters.
 r.clear(0);s.alphaRef=0;r.screenTriangle(v,&t,s);assert(r.pixels()[50*320+50]==t.pixels[0]);assert(!r.stats().filteredAlphaOnly);assert(r.stats().filteredFlat>10000);
 puts("PASS 300000 exact bilinear comparisons; invisible alpha skips RGB, alphaRef=0 preserves transparent RGB/depth semantics");
}
