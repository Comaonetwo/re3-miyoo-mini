#include "tile_depth.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){
 std::mt19937 rng(4201);
 for(unsigned n=0;n<200;++n){unsigned w=1+rng()%640,h=1+rng()%480;
  float row=.01f+float(rng()%1000)/1000,dx=float(int(rng()%200)-100)*.00001f,dy=float(int(rng()%200)-100)*.00001f;
  unsigned x0=rng()%w,x1=x0+rng()%(w-x0),y0=rng()%h,y1=y0+rng()%(h-y0);double bound=tileDepthUpper(row,dx,dy,w-1,h-1,x0,x1,y0,y1);
  float r=row;for(unsigned y=0;y<h;++y){float z=r;for(unsigned x=0;x<w;++x){if(x>=x0&&x<=x1&&y>=y0&&y<=y1)assert(double(z)<bound);z+=dx;}r+=dy;}
 }
 uint64_t extra=0;
 for(unsigned scale=1;scale<=4;++scale){Renderer a,b;a.tileDepthEnabled(true);b.tileDepthEnabled(false);a.renderScale(scale);b.renderScale(scale);a.clear(12345);b.clear(12345);
  ScreenState s;s.perspectiveColor=true;s.zTest=s.zWrite=true;s.cull=1;s.blend=0;
  ScreenVertex wall[4]={{-20,-20,.2f,0,0,110,150,190,255,0},{340,-20,.5f,1,0,110,150,190,255,0},{-20,260,.2f,0,1,110,150,190,255,0},{340,260,.5f,1,1,110,150,190,255,0}};
  ScreenVertex q[3]={wall[0],wall[1],wall[2]},r[3]={wall[2],wall[1],wall[3]};
  for(auto p:{&a,&b}){p->screenTriangle(q,nullptr,s);p->screenTriangle(r,nullptr,s);}
  for(unsigned k=0;k<350;++k){ScreenVertex v[3]={{10,10,.2f,0,0,90,150,200,160,0},{300,10,.44f,1,0,200,70,120,160,0},{10,225,.2f,0,1,170,200,80,160,0}};
   s.blend=1;s.zWrite=k%2;s.alphaRef=k%3?1:128;
   if(k%13==0)for(auto& p:v)p.reciprocalZ+=.2f;
   a.screenTriangle(v,nullptr,s);b.screenTriangle(v,nullptr,s);
   assert(a.pixels()==b.pixels()&&a.depth()==b.depth()&&a.stats().pixels==b.stats().pixels);
   if(k==280){a.clearUiDepth();b.clearUiDepth();}
  }
  assert(a.stats().occludedTriangles>=b.stats().occludedTriangles);extra+=a.stats().occludedTriangles-b.stats().occludedTriangles;
 }
 assert(extra>0);printf("PASS 200 float recurrence bounds and layered gradient occlusion, depth invalidation, extra culls=%llu\n",(unsigned long long)extra);
}
