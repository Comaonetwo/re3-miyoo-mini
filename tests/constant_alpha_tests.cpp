#include "constant_alpha.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){std::mt19937 rng(4103);unsigned certified=0;for(unsigned n=0;n<15000;++n){float row[8]={},dx[8]={},dy[8]={};unsigned alpha=rng()%256,w=1+rng()%80,h=1+rng()%60;row[0]=.005f+rng()%1000/1000.f;dx[0]=float(int(rng()%201)-100)*row[0]/10000;dy[0]=float(int(rng()%201)-100)*row[0]/10000;row[6]=alpha*row[0];dx[6]=alpha*dx[0];dy[6]=alpha*dy[0];if(n%7==0)row[6]+=.6f*row[0];if(!certifyConstantAlpha(row,dx,dy,w,h,alpha))continue;++certified;float zr=row[0],ar=row[6];for(unsigned y=0;y<h;++y){float z=zr,a=ar;for(unsigned x=0;x<w;++x){assert(z>1e-8);unsigned original=unsigned(std::max(0.f,std::min(255.f,a*(1/z)+.5f)));assert(original==alpha);z+=dx[0];a+=dx[6];}zr+=dy[0];ar+=dy[6];}}
 assert(certified>10000);uint64_t applied=0;
 for(unsigned n=0;n<450;++n){Renderer a,b;b.constantAlphaEnabled(false);a.occlusionEnabled(false);b.occlusionEnabled(false);a.renderScale(n%4+1);b.renderScale(n%4+1);Texture t;t.width=t.height=16;t.pixels.resize(256);t.alpha.resize(256);for(unsigned i=0;i<256;++i){t.pixels[i]=rng();t.alpha[i]=n%3?255:rng()%256;}t.decodedOpaque=n%3;ScreenState s;s.perspectiveColor=true;s.zTest=true;s.zWrite=n%2;s.blend=n%5?1:0;s.addressU=s.addressV=n%2?1:3;s.alphaRef=n%3?1:128;uint16_t bg=rng();a.clear(bg);b.clear(bg);
 for(unsigned k=0;k<4;++k){unsigned alpha=rng()%256;ScreenVertex v[3];for(auto& p:v)p={float(int(rng()%900)-300),float(int(rng()%650)-200),.02f+float(rng()%100)/100,float(rng()%300)/100,float(rng()%300)/100,float(rng()%256),float(rng()%256),float(rng()%256),float(alpha),0};a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels&&a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);}
 applied+=a.stats().constantAlphaPixels;}
 assert(!pixelDiagnosticsEnabled()||applied>100000);printf("PASS 15000 recurrence certificates and 450 layered RGB/depth/count comparisons; certified=%u applied=%llu\n",certified,(unsigned long long)applied);}
