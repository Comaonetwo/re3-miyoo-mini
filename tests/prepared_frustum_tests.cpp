#include "model_clusters.hpp"
#include <random>
#include <cassert>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 rng(4040);unsigned culls=0;
 for(unsigned n=0;n<250000;++n){float m[12];float scale=std::pow(10.f,float(int(n%13)-6));
  for(auto& v:m)v=float(int(rng()%2001)-1000)/317.f*scale;
  Camera camera;camera.nearZ=.1f;camera.farZ=200;camera.focal=80+rng()%250;
  float vertical=.4f+rng()%200/100.f;
  Vec3 origin={float(int(rng()%400)-200),float(int(rng()%400)-200),float(int(rng()%400)-200)};
  ModelBounds whole;whole.include(origin);whole.include({origin.x+60,origin.y+60,origin.z+60});
  ModelBounds leaf;leaf.include({origin.x+float(rng()%20),origin.y+float(rng()%20),origin.z+float(rng()%20)});leaf.include({origin.x+40,origin.y+40,origin.z+40});
  ModelFrustum reference(m,vertical,camera);PreparedModelFrustum prepared(reference,whole);
  if(prepared.outside(leaf)){++culls;assert(reference.outside(leaf));}
  unsigned mask=reference.activePlanes(whole,reference.boundMargin(whole));
  if(mask&128)assert(reference.outside(leaf));else{PreparedModelFrustum compact(reference,whole,true,mask);if(compact.outside(leaf))assert(reference.outside(leaf));if(mask==0)assert(!compact.outside(leaf));}
 }
 assert(culls>100000);
 float m[12]={-1,0,0,0,0,1,0,0,0,0,1,0};Camera camera;ModelFrustum f(m,1,camera);
 ModelBounds whole;whole.include({-100,-100,-100});whole.include({100,100,100});PreparedModelFrustum p(f,whole);
 ModelBounds near;near.include({0,0,camera.nearZ});assert(!p.outside(near));
 ModelBounds invalid=whole;invalid.maximum.x=std::numeric_limits<float>::quiet_NaN();assert(!p.outside(invalid));invalid.valid=false;assert(!p.outside(invalid));
 PreparedModelFrustum disabled(f,whole,false);ModelBounds behind;behind.include({0,0,-10});assert(!disabled.outside(behind));assert(p.outside(behind));
 puts("PASS 250000 affine/scale/cancellation bounds: every float rejection also rejected by double reference; invalid/contact retained");
}
