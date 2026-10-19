#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(2101);unsigned long long prepared=0,saved=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<320;++n){
  Renderer a,b;a.batchGeneralCountersEnabled(n%3==0);a.renderScale(scale);b.renderScale(scale);a.occlusionEnabled(false);b.occlusionEnabled(false);b.batchCountersEnabled(false);b.preparedWorldEnabled(false);b.modulationTablesEnabled(false);a.exactAdvanceEnabled(n%2);b.exactAdvanceEnabled(false);
  Texture t;t.width=n%7?16:15;t.height=n%11?8:7;t.pixels.resize(t.width*t.height);t.alpha.resize(t.pixels.size());for(unsigned j=0;j<t.pixels.size();++j){t.pixels[j]=rng();t.alpha[j]=n%3?255:rng()%256;}
  unsigned short bg=rng();a.clear(bg);b.clear(bg);
  ScreenState s;s.perspectiveColor=true;s.zTest=s.zWrite=true;s.addressU=s.addressV=n%5==0?3:1;s.blend=n%8==0?0:1;s.alphaRef=n%4==0?128:1;s.linear=n%19==0;
  for(unsigned k=0;k<5;++k){float red=rng()%256,green=rng()%256,blue=rng()%256,alpha=n%3?255:rng()%256;
   ScreenVertex v[3];for(unsigned j=0;j<3;++j){v[j].x=int(rng()%700)-190;v[j].y=int(rng()%500)-120;v[j].reciprocalZ=n%4?.01f+float(rng()%1000)/1000:.25f;v[j].u=float(int(rng()%800)-400)/100;v[j].v=float(int(rng()%800)-400)/100;v[j].r=n%2?red:rng()%256;v[j].g=n%2?green:rng()%256;v[j].b=n%2?blue:rng()%256;v[j].a=alpha;v[j].depthZ=0;}
   a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);
  }
  assert(a.pixels()==b.pixels());assert(a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);prepared+=a.stats().preparedWorldPixels;saved+=a.stats().attributeSavedAdds;
 }
 assert(prepared>100000&&saved>1000);printf("PASS 1280 layered full RGB/depth comparisons; prepared=%llu exact skipped additions=%llu\n",prepared,saved);
}
