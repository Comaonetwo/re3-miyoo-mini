#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){uint64_t early=0;Texture t;t.width=t.height=8;t.pixels.assign(64,35197);t.alpha.assign(64,255);t.decodedOpaque=true;for(unsigned scale=1;scale<=4;++scale)for(unsigned frame=0;frame<8;++frame){Renderer a,b;a.earlyDepthEnabled(true);b.earlyDepthEnabled(false);a.renderScale(scale);b.renderScale(scale);a.clear(12243);b.clear(12243);ScreenState s;s.perspectiveColor=true;s.zTest=s.zWrite=true;s.cull=1;s.blend=1;s.addressU=s.addressV=3;
 ScreenVertex full[4]={{0,0,.8f,0,0,130,170,210,255,0},{320,0,.8f,1,0,130,170,210,255,0},{0,240,.8f,0,1,130,170,210,255,0},{320,240,.8f,1,1,130,170,210,255,0}};ScreenVertex x[3]={full[0],full[1],full[2]},y[3]={full[2],full[1],full[3]};a.screenTriangle(x,&t,s);b.screenTriangle(x,&t,s);a.screenTriangle(y,&t,s);b.screenTriangle(y,&t,s);
 for(unsigned k=0;k<320;++k){float z=k%7?.2f:.9f;ScreenVertex v[3]={{float(k%8),float(k%5),z,0,0,90,170,220,160,0},{300,20,z+.001f,1,0,200,220,100,160,0},{20,230,z,0,1,180,50,200,160,0}};s.zWrite=k%3;s.alphaRef=k%2?1:128;a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels&&a.stats().occlusionChecks==b.stats().occlusionChecks);if(k==170&&frame%2){a.clearUiDepth();b.clearUiDepth();}}
 early+=a.stats().earlyDepthTriangles;
 }assert(early>100);printf("PASS early depth across scales, opaque/fading/gradient/no-z-write, depth invalidation: exact image/depth/counters; triangles=%llu\n",(unsigned long long)early);}
