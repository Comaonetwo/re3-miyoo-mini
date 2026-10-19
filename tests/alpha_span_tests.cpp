#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
#include <cstring>
using namespace soft;
// The extra argument is only for the independently linked legacy raster.
// Normal CTest runs still require the new kernels to have been exercised.
int main(int argc,char**){bool referenceRaster=argc>1;uint64_t imageHash=1469598103934665603ull,depthHash=imageHash,countHash=imageHash;Renderer r;std::mt19937 random(4701);unsigned eligible=0;uint64_t touched=0,rows=0;
 for(unsigned n=0;n<1568;++n){
  Texture t;t.width=n%3?16:13;t.height=n%5?8:11;t.pixels.resize(t.width*t.height);t.alpha.resize(t.pixels.size());
  for(unsigned k=0;k<t.pixels.size();++k){t.pixels[k]=uint16_t(random());t.alpha[k]=n%4?uint8_t(random()):255;}
  if(n%11==0)for(auto& p:t.pixels)p=65535;if(n%13==0)for(auto& p:t.pixels)p=0;
  t.decodedOpaque=n%4==0;t.decodedUniform=n%11==0||n%13==0;
  unsigned c[4]={unsigned(random()%256),unsigned(random()%256),unsigned(random()%256),unsigned(random()%256)};
  if(n%7==0)c[0]=c[1]=c[2]=255;if(n%17==0)c[3]=255;if(n%19==0)c[0]=c[1]=c[2]=0;
  float x=float(int(random()%360)-30),y=float(int(random()%260)-20),size=float(random()%130+1);
  ScreenVertex v[3]={{x,y,.2f,-.2f,-.3f,float(c[0]),float(c[1]),float(c[2]),float(c[3]),.35f},
   {x+size,y+.25f,.2f,1.2f,-.1f,float(c[0]),float(c[1]),float(c[2]),float(c[3]),.35f},
   {x+.1f,y+size,.2f,.1f,1.3f,float(c[0]),float(c[1]),float(c[2]),float(c[3]),.35f}};
  if(n%4==1){v[1].y=v[0].y;v[1].v=v[0].v;}
  ScreenState s;s.screenDepth=n%3!=0;s.linear=n%2;s.blend=1;s.zTest=n%2;s.alphaRef=n%5==0?0:n%5==1?1:n%5==2?128:n%5==3?254:255;s.perspectiveColor=!s.screenDepth;if(!s.screenDepth&&n%2)v[1].reciprocalZ=.1f;s.addressU=n%4+1;s.addressV=(n/4)%4+1;
  // These cases must retain the general path.
  if(n%23==0)s.zWrite=true;if(n%29==0)s.perspectiveColor=true;if(n%31==0)v[1].r+=.125f;if(n%37==0)v[1].depthZ=.3f;if(n%41==0)s.blend=4;if(n%43==0)s.linear=false;if(n%47==0)v[1].reciprocalZ=.1f;
  if(n>=1440){s.screenDepth=true;s.linear=true;s.blend=1;s.zTest=s.zWrite=s.perspectiveColor=false;s.alphaRef=1;
   v[0]={20,30,.2f,-.2f,.25f,180,200,220,200,.35f};v[1]={180,30,.2f,1.2f,.25f,180,200,220,200,.35f};v[2]={20,190,.2f,-.2f,.75f,180,200,220,200,.35f};}
  auto draw=[&](bool fast){r.alphaSpansEnabled(fast);r.renderScale(n%4+1);r.clear(uint16_t(n*71));if(n%9==0)r.beginHud(640);else if(n%10==0)r.beginHud(240);r.vectorFilterEnabled(n>=1440||n%3);r.filterStages(n%6==0?31:19);r.effectSpansEnabled(n%2);r.particleSpansEnabled(n%3);r.additiveCacheEnabled(n%2);r.blackAddEnabled(n%2);r.screenCertificatesEnabled(n%3);r.clearUiDepth();
   if(n%2){ScreenState wall;wall.screenDepth=true;wall.zWrite=true;ScreenVertex q[3]={{0,0,0,0,0,200,200,200,255,.2f},{320,0,0,0,0,200,200,200,255,.8f},{0,240,0,0,0,200,200,200,255,.2f}};r.screenTriangle(q,nullptr,wall);}
   if(n<1440&&n%7==0)r.beginRadar(30,30,200,200);r.screenTriangle(v,&t,s);r.endRadar();};
  draw(false);auto rgb=r.pixels();auto depth=r.depth();auto count=r.stats().pixels;auto spans=r.stats().spanSkippedPixels;draw(true);
  if(rgb!=r.pixels()||depth!=r.depth()||count!=r.stats().pixels||spans!=r.stats().spanSkippedPixels){std::printf("FAIL sprite case %u\n",n);return 1;}
  for(auto pixel:r.pixels()){imageHash^=pixel;imageHash*=1099511628211ull;}for(float z:r.depth()){uint32_t bits;std::memcpy(&bits,&z,4);depthHash^=bits;depthHash*=1099511628211ull;}
  countHash^=r.stats().pixels;countHash*=1099511628211ull;countHash^=r.stats().spanSkippedPixels;countHash*=1099511628211ull;
  eligible+=r.stats().alphaSpanTriangles>0;touched+=r.stats().alphaSpanPixels;rows+=r.stats().filterRowPixels;
 }
 std::printf("coverage eligible=%u pixels=%llu row_pixels=%llu\n",eligible,(unsigned long long)touched,(unsigned long long)rows);if(!referenceRaster)assert(eligible>150&&(!pixelDiagnosticsEnabled()||(touched>10000&&rows>10000)));std::printf("REFERENCE HASH rgb=%llx depth=%llx counts=%llx\n",(unsigned long long)imageHash,(unsigned long long)depthHash,(unsigned long long)countHash);std::printf("PASS 1568 paired alpha surfaces, %u specialized triangles, %llu pixels; exact RGB/depth/fragments/spans across alpha/address/filter/scale/targets and fallbacks\n",eligible,(unsigned long long)touched);
}
