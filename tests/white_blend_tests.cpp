#include "soft_renderer.hpp"
#include <random>
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 rng(4141);uint64_t applied=0;
 for(unsigned n=0;n<350;++n){Renderer a,b;a.whiteBlendEnabled(true);b.whiteBlendEnabled(false);a.occlusionEnabled(false);b.occlusionEnabled(false);a.renderScale(n%4+1);b.renderScale(n%4+1);a.modulationTablesEnabled(n%2);b.modulationTablesEnabled(n%2);a.flatAlphaEnabled(n%5==0);b.flatAlphaEnabled(n%5==0);Texture t;t.width=t.height=16;t.pixels.resize(256);t.alpha.resize(256);for(unsigned i=0;i<256;++i){t.pixels[i]=rng();t.alpha[i]=n%3?255:rng()%256;}t.decodedOpaque=n%3;ScreenState s;s.perspectiveColor=true;s.zTest=true;s.zWrite=n%2;s.blend=1;s.alphaRef=n%3?1:128;s.addressU=s.addressV=n%2?1:3;a.clear(rng());b.clear(0);b.clear(a.pixels()[0]);
 for(unsigned k=0;k<4;++k){unsigned alpha=rng()%256;ScreenVertex v[3];for(auto& p:v)p={float(int(rng()%900)-300),float(int(rng()%650)-200),.03f+float(rng()%100)/100,float(rng()%400)/100,float(rng()%400)/100,255,255,255,float(alpha),0};a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);}applied+=a.stats().whiteBlendPixels;
 }assert(!pixelDiagnosticsEnabled()||applied>10000);printf("PASS 350 white material source-over cases, texture/vertex alpha, LUTs/depth/scale: exact RGB/depth/counts, pixels=%llu\n",(unsigned long long)applied);}
