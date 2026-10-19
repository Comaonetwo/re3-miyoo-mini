#include "additive_bounds.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 rng(3901);uint64_t saved=0;for(unsigned n=0;n<240;++n){Texture t;t.width=t.height=16;t.pixels.assign(256,0);t.alpha.assign(256,255);t.decodedOpaque=true;for(unsigned y=4;y<12;++y)for(unsigned x=3;x<13;++x)t.pixels[y*16+x]=uint16_t(rng());if(n%13==0)for(auto& p:t.pixels)p=0;if(n%17==0)t.pixels[0]=65535;if(n%19==0)t.pixels.back()=65535;certifyInkBounds(t);
 Renderer a,b;a.additiveBoundsEnabled(true);a.additiveCacheEnabled(true);b.additiveCacheEnabled(true);a.blackAddEnabled(true);b.blackAddEnabled(true);b.additiveBoundsEnabled(false);a.renderScale(n%3+1);b.renderScale(n%3+1);a.occlusionEnabled(false);b.occlusionEnabled(false);a.clear(23911);b.clear(23911);ScreenState s;s.screenDepth=true;s.zTest=n%2;s.zWrite=false;s.blend=n%2?2:3;s.linear=n%3;s.addressU=s.addressV=n%5?3:1;s.alphaRef=n%4?1:128;float alpha=n%4?160:255;ScreenVertex v[3]={{-5,-12,1,-.27f,-.4f,173,213,117,alpha,.13f},{316,7,1,1.31f,-.31f,173,213,117,alpha,.13f},{23,237,1,-.2f,1.4f,173,213,117,alpha,.13f}};
 for(unsigned k=0;k<3;++k){a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);v[0].depthZ+=.1f;v[1].depthZ=v[2].depthZ=v[0].depthZ;}
 saved+=a.stats().additiveBoundsPixels;
 // Raster edits must rebuild the certificate, including former black edges.
 t.pixels[0]=1;certifyInkBounds(t);assert(t.inkX0==0&&t.inkY0==0);assert(!outsideInk(-1,-1,t,true));
 }assert(saved>100000);printf("PASS 240 additive frames: exact RGB/depth/fragment/span, clamp/repeat, linear/nearest, empty/edge ink; skipped=%llu\n",(unsigned long long)saved);}
