#include "exact_rgb_step.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
#include <cstring>
using namespace soft;
int main(){
 assert(normalLatticeInput(0.f)&&normalLatticeInput(-0.f));
 for(float x:{1e-38f,-1e-38f,1e-30f,1e30f})assert(!normalLatticeInput(x));
 float row[3]={1,2,3},dx[3]={-.5f,.25f,0},dy[3]={0,0,0};assert(rgbRegisterSafe(row,dx,dy));dy[1]=1e-38f;assert(!rgbRegisterSafe(row,dx,dy));
 std::mt19937 rng(7719);uint64_t triangles=0,steps=0;
 for(unsigned n=0;n<600;++n){
  Renderer a,b;b.rgbRegisterEnabled(false);
  for(auto r:{&a,&b}){r->renderScale(n%4+1);r->clear(uint16_t(n*47));r->occlusionEnabled(false);r->batchGeneralCountersEnabled(n%11==0);r->nearestCoverageEnabled(n%3);r->worldChannelsEnabled(true);}
  Texture t;t.width=t.height=16;t.pixels.resize(256);t.alpha.resize(256);for(unsigned j=0;j<256;++j){t.pixels[j]=uint16_t(rng());t.alpha[j]=n%9?255:uint8_t(rng());}t.decodedOpaque=n%9!=0;
  ScreenState s;s.perspectiveColor=true;s.zTest=true;s.zWrite=n%13!=0;s.blend=n%5?1:0;s.addressU=s.addressV=n%7?1:3;s.alphaRef=n%3?1:128;
  for(unsigned k=0;k<3;++k){ScreenVertex v[3]={{-10,4,.2f,-.3f,.1f,13,233,71,255,0},{310,17,.3f,1.7f,.1f,249,7,253,255,0},{11,231,.1f,-.3f,1.6f,89,171,3,255,0}};
   for(auto& q:v){q.x+=float(int(rng()%21)-10);q.y+=float(int(rng()%21)-10);q.r=float(rng()%256);q.g=float(rng()%256);q.b=float(rng()%256);if(n%17==0)q.a=127;}
   if(n%19==0)for(auto& q:v)q.reciprocalZ=.25f;
   if(n%23==0)for(unsigned i=0;i<3;++i){v[i].r=1e-38f*float(i+1);v[i].g=2e-38f*float(i+1);v[i].b=3e-38f*float(i+1);}
   a.screenTriangle(v,&t,s);b.screenTriangle(v,&t,s);
   assert(a.pixels()==b.pixels()&&a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels);assert(a.stats().spanSkippedPixels==b.stats().spanSkippedPixels);
  }
  if(n%23==0)assert(a.stats().rgbRegisterTriangles==0&&a.stats().rgbRegisterSteps==0);
  triangles+=a.stats().rgbRegisterTriangles;steps+=a.stats().rgbRegisterSteps;assert(b.stats().rgbRegisterTriangles==0&&b.stats().rgbRegisterSteps==0);
 }
 if(worldChannelsAvailable)assert(triangles>100&&steps>100000);
 std::printf("PASS 600 paired layered RGB register cases: exact RGB/depth/core writes/spans, activated triangles=%llu steps=%llu; four resolutions, blending, coverage, opaque/masked, wrap/clamp and batch fallbacks\n",(unsigned long long)triangles,(unsigned long long)steps);
}
