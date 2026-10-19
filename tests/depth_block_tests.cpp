#include "soft_renderer.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
static Vec3 point(float x,float y,float z){return {(x-160)*z/180,(120-y)*z/180,z};}
static void draw(Renderer& r,const Texture& tex,float z,uint16_t tint,float left,float top,float right,float bottom){Vec3 a[3]={point(left,top,z),point(right,top,z),point(left,bottom,z)},b[3]={a[2],a[1],point(right,bottom,z)};Vec2 uv[3]={{0,0},{1,0},{0,1}},uvb[3]={{0,1},{1,0},{1,1}};Camera c;c.position={0,0,0};r.texturedTriangle(a,uv,tint,tex,1,1,c,1);r.texturedTriangle(b,uvb,tint,tex,1,1,c,1);}
int main(){std::mt19937 rng(2302);uint64_t skipped=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<30;++n){Renderer fast,slow;fast.renderScale(scale);slow.renderScale(scale);fast.depthBlocksEnabled(true);slow.depthBlocksEnabled(false);fast.depthRowCullingEnabled(true);slow.depthRowCullingEnabled(true);fast.occlusionEnabled(false);slow.occlusionEnabled(false);Texture tex;tex.width=tex.height=16;tex.pixels.resize(256);tex.alpha.resize(256);for(unsigned i=0;i<256;++i){tex.pixels[i]=rng();tex.alpha[i]=n%3==0&&i%7==0?0:255;}tex.decodedOpaque=n%3!=0;fast.clear(0);slow.clear(0);
  draw(fast,tex,2,65535,0,0,170,240);draw(slow,tex,2,65535,0,0,170,240);
  for(unsigned i=0;i<160;++i){if(i==100&&n%5==0){fast.clearBuffers(0,false,true);slow.clearBuffers(0,false,true);}float z=i%11==0?1.5f:i%7==0?2.001f:5+float(rng()%200)/10;uint16_t tint=rng();float x=rng()%80,y=rng()%60,w=100+rng()%130,h=80+rng()%100;draw(fast,tex,z,tint,x,y,x+w,y+h);draw(slow,tex,z,tint,x,y,x+w,y+h);}
  assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels&&fast.stats().spanSkippedPixels==slow.stats().spanSkippedPixels&&fast.stats().perspectiveSaved==slow.stats().perspectiveSaved);assert(fast.stats().packedReciprocals==slow.stats().packedReciprocals);skipped+=fast.stats().hiddenDepthBlocks;
 }assert(skipped>100);printf("PASS 120 depth-block scenes incl transparent holes, close depth, foreground draws, clear-Z invalidation; exact RGB/depth; skipped blocks=%llu\n",(unsigned long long)skipped);
}
