#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(3801);uint64_t covered=0,opaque=0;
 for(unsigned n=0;n<600;++n){Renderer a,b;b.nearestCoverageEnabled(false);b.nearestOpacityEnabled(false);a.screenCertificatesEnabled(n%19!=0);b.screenCertificatesEnabled(n%19!=0);a.occlusionEnabled(false);b.occlusionEnabled(false);a.renderScale(n%4+1);b.renderScale(n%4+1);a.batchGeneralCountersEnabled(n%2);b.batchGeneralCountersEnabled(n%2);a.modulationTablesEnabled(n%3);b.modulationTablesEnabled(n%3);a.flatAlphaEnabled(n%7==0);b.flatAlphaEnabled(n%7==0);
  Texture t;t.width=t.height=16;t.pixels.resize(256);t.alpha.resize(256);for(unsigned j=0;j<256;++j){t.pixels[j]=n%13?rng():0;t.alpha[j]=n%4?255:rng()%256;}t.decodedOpaque=(n%4)&&(n%17!=0);t.decodedUniform=n%13==0;
  uint16_t bg=rng();a.clear(bg);b.clear(bg);ScreenState s;s.perspectiveColor=true;s.zTest=true;s.zWrite=n%3;s.blend=n%5?1:0;s.addressU=s.addressV=n%2?1:3;s.alphaRef=n%3?1:128;
  for(unsigned k=0;k<5;++k){float r=rng()%256,g=rng()%256,blue=rng()%256,alpha=n%6?160:255;ScreenVertex v[3];for(unsigned j=0;j<3;++j){v[j]={float(int(rng()%1000)-300),float(int(rng()%600)-180),n%3?.02f+float(rng()%100)/100:.4f,float(int(rng()%400)-200)/100,float(int(rng()%400)-200)/100,n%2?r:float(rng()%256),n%2?g:float(rng()%256),n%2?blue:float(rng()%256),n%11?alpha:float(rng()%256),0};}
   a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels&&a.stats().perspectiveSaved==b.stats().perspectiveSaved);
  }covered+=a.stats().nearestCoveragePixels;opaque+=a.stats().nearestOpaquePixels;
 }
 assert(!pixelDiagnosticsEnabled()||(covered>100000&&opaque>100000));printf("PASS 600 layered coverage cases: exact RGB/depth/fragment/span/reciprocal counters; certified=%llu\n",(unsigned long long)covered);
}
