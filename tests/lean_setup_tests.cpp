#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(3713);uint64_t saved=0;
 for(unsigned n=0;n<480;++n){Renderer a,b;b.leanSetupEnabled(false);a.occlusionEnabled(false);b.occlusionEnabled(false);
  if(n%3==0){a.beginUi(true);b.beginUi(true);}else if(n%3==1){a.renderScale(3);b.renderScale(3);}else{a.beginHud(240);b.beginHud(240);}
  auto bg=uint16_t(rng());a.clear(bg);b.clear(bg);
  Texture t;t.width=16;t.height=8;t.pixels.resize(128);t.alpha.resize(128);for(unsigned j=0;j<128;++j){t.pixels[j]=rng();t.alpha[j]=rng()%256;}
  ScreenState s;s.zTest=n%3;s.zWrite=n%4;s.screenDepth=n%2;s.perspectiveColor=n%7==0;s.blend=n%6;s.alphaRef=n%4?0:128;s.linear=n%5==0;s.addressU=1+n%4;s.addressV=1+(n+1)%4;
  for(unsigned k=0;k<4;++k){float r=rng()%256,g=rng()%256,blue=rng()%256,alpha=rng()%256,z=float(rng()%256)/255;
   ScreenVertex v[3];for(unsigned j=0;j<3;++j){v[j].x=int(rng()%500)-100;v[j].y=int(rng()%380)-70;v[j].reciprocalZ=.01f+float(rng()%1000)/1000;v[j].u=float(int(rng()%400)-200)/100;v[j].v=float(int(rng()%400)-200)/100;v[j].r=n%3?r:rng()%256;v[j].g=n%3?g:rng()%256;v[j].b=n%3?blue:rng()%256;v[j].a=n%3?alpha:rng()%256;v[j].depthZ=n%5?z:float(rng()%256)/255;}
   a.screenTriangle(v,n%11?&t:nullptr,s);b.screenTriangle(v,n%11?&t:nullptr,s);
   assert(a.pixels()==b.pixels());assert(a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);
  }saved+=a.stats().setupSavedAttributes;
 }
 assert(saved>1000);printf("PASS 480 layered world/native UI/HUD cases: exact RGB/depth/counters; setup_saved=%llu\n",(unsigned long long)saved);
}
