#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 random(1917);unsigned cached=0;
 for(unsigned scale=1;scale<=4;++scale)for(unsigned n=0;n<180;++n){
    Renderer fast,reference;fast.renderScale(scale);reference.renderScale(scale);fast.occlusionEnabled(false);reference.occlusionEnabled(false);
    uint16_t bg=uint16_t(random());fast.clear(bg);reference.clear(bg);
    Texture t;t.width=16;t.height=8;t.pixels.resize(128);t.alpha.resize(128);
    for(unsigned i=0;i<128;++i){t.pixels[i]=uint16_t(random());t.alpha[i]=n%3?255:random()%256;}
    float r=random()%256,g=random()%256,b=random()%256,a=n%4?255:random()%256;
    if(n%13==0)r=g=b=255;
    ScreenVertex v[3]={{12,8,.1f,-.25f,1.25f,r,g,b,a,0},{312,30,.5f,1.25f,1.25f,r,g,b,a,0},{21,231,.1f,-.25f,-.25f,r,g,b,a,0}};
    // Fractional but constant colour forces the original perspective-colour
    // path. Its rounded channels match the integer input throughout the triangle.
    ScreenVertex general[3];for(int i=0;i<3;++i){general[i]=v[i];for(float* f:{&general[i].r,&general[i].g,&general[i].b,&general[i].a})*f+=*f==255?-.125f:.125f;}
    if(n%7==0)for(unsigned i=0;i<3;++i)v[i].reciprocalZ=general[i].reciprocalZ=.25f;
    ScreenState s;s.zTest=s.zWrite=true;s.perspectiveColor=true;s.blend=n%6;s.alphaRef=n%2?1:128;s.addressU=1+n%4;s.addressV=1+(n/4)%4;s.linear=n%5==0;
    fast.screenTriangle(v,&t,s);reference.screenTriangle(general,&t,s);
    assert(fast.pixels()==reference.pixels());assert(fast.depth()==reference.depth());cached+=fast.stats().tintPixels;
 }
 // Independent analytic reference: constant depth makes UV affine even when
 // gradient colours still require reciprocal depth for perspective correction.
 Renderer r;Texture t;t.width=t.height=64;t.alpha.assign(4096,255);
 for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)t.pixels.push_back(rgb(x*4,y*4,160));
 ScreenVertex v[3]={{16,16,.25f,0,0,32,64,128,255,0},{144,16,.25f,1,0,224,64,128,255,0},{16,144,.25f,0,1,32,192,128,255,0}};
 ScreenState state;state.zTest=state.zWrite=state.perspectiveColor=true;state.blend=1;r.screenTriangle(v,&t,state);
 for(unsigned y=20;y<70;y+=7)for(unsigned x=20;x<70;x+=7){
  float u=(x+.5f-16)/128,vv=(y+.5f-16)/128;uint16_t pixel=t.pixels[unsigned(vv*64)*64+unsigned(u*64)];
  unsigned cr=unsigned(32+192*u+.5f),cg=unsigned(64+128*vv+.5f),cb=128,tr=pixel>>11,tg=(pixel>>5)&63,tb=pixel&31;
  assert(r.pixels()[y*320+x]==rgb(cr*((tr<<3)|(tr>>2))/255,cg*((tg<<2)|(tg>>4))/255,cb*((tb<<3)|(tb>>2))/255));
 }
 assert(cached>1000);printf("PASS 720 full-image material comparisons against general colour interpolation, %u table pixels; wrap/mirror/clamp/filter/partial alpha\n",cached);
}
