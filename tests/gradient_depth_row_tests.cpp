#include "soft_renderer.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
static Vec3 point(float x,float y,float z){return {(x-160)*z/180,(120-y)*z/180,z};}
static void draw(Renderer& r,const Texture& tex,float z,uint16_t tint,float left,float top,float right,float bottom){Vec3 a[3]={point(left,top,z),point(right,top,z),point(left,bottom,z)},b[3]={a[2],a[1],point(right,bottom,z)};Vec2 uv[3]={{0,0},{1,0},{0,1}},uvb[3]={{0,1},{1,0},{1,1}};Camera c;c.position={0,0,0};ShadedVertex va[3],vb[3];for(unsigned i=0;i<3;++i){float cr=100+float((tint+i*17)%155);va[i]={a[i],uv[i],cr,180,float(200-i*25),float(255-i*20)};vb[i]={b[i],uvb[i],cr,180,float(200-i*25),float(255-i*20)};}ScreenState state;state.zTest=state.zWrite=true;state.blend=1;state.alphaRef=1;state.addressU=state.addressV=1;state.perspectiveColor=true;r.shadedTriangle(va,&tex,state,c);r.shadedTriangle(vb,&tex,state,c);}
int main(){std::mt19937 rng(2302);uint64_t skipped=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<30;++n){Renderer fast,slow;fast.renderScale(scale);slow.renderScale(scale);fast.depthRowCullingEnabled(true);fast.gradientDepthRowsEnabled(true);slow.depthRowCullingEnabled(false);fast.occlusionEnabled(false);slow.occlusionEnabled(false);Texture tex;tex.width=tex.height=16;tex.pixels.resize(256);tex.alpha.resize(256);for(unsigned i=0;i<256;++i){tex.pixels[i]=rng();tex.alpha[i]=n%3==0&&i%7==0?0:255;}fast.clear(0);slow.clear(0);
  draw(fast,tex,2,65535,0,0,320,240);draw(slow,tex,2,65535,0,0,320,240);
  for(unsigned i=0;i<160;++i){if(i==100&&n%5==0){fast.clearBuffers(0,false,true);slow.clearBuffers(0,false,true);}float z=i%11==0?1.5f:i%7==0?2.001f:5+float(rng()%200)/10;uint16_t tint=rng();float x=rng()%80,y=rng()%60,w=100+rng()%130,h=80+rng()%100;draw(fast,tex,z,tint,x,y,x+w,y+h);draw(slow,tex,z,tint,x,y,x+w,y+h);}
  assert(fast.pixels()==slow.pixels());assert(fast.depth()==slow.depth());assert(fast.stats().pixels==slow.stats().pixels&&fast.stats().spanSkippedPixels==slow.stats().spanSkippedPixels&&fast.stats().perspectiveSaved==slow.stats().perspectiveSaved);skipped+=fast.stats().hiddenDepthRows;
 }assert(skipped>100);printf("PASS 120 gradient depth-row scenes incl transparent holes, close depth, foreground draws, clear-Z invalidation; exact RGB/depth; skipped rows=%llu\n",(unsigned long long)skipped);
}
