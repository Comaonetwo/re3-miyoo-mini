#include "soft_renderer.hpp"
#include "hidden_samples.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
int main(){
 std::mt19937 rng(433);uint64_t removed=0;
 Texture t;t.width=t.height=4;t.pixels.resize(16);t.alpha.resize(16);
 for(unsigned k=0;k<16;++k){t.pixels[k]=uint16_t(k*4567);t.alpha[k]=uint8_t(k*17);}t.decodedOpaque=false;
 for(unsigned scale=1;scale<=4;++scale){Renderer a,b;a.smallDepthEnabled(true);b.smallDepthEnabled(false);a.renderScale(scale);b.renderScale(scale);a.clear(32975);b.clear(32975);
  ScreenState s;s.zTest=s.zWrite=true;s.perspectiveColor=true;
  ScreenVertex wall[4]={{-20,-20,.2f,0,0,110,150,190,255,0},{340,-20,.5f,1,0,110,150,190,255,0},{-20,260,.2f,0,1,110,150,190,255,0},{340,260,.5f,1,1,110,150,190,255,0}};
  ScreenVertex q[3]={wall[0],wall[1],wall[2]},r[3]={wall[2],wall[1],wall[3]};
  for(auto p:{&a,&b}){p->screenTriangle(q,nullptr,s);p->screenTriangle(r,nullptr,s);}
  for(unsigned k=0;k<15000;++k){float x=float(rng()%340)-10+.5f,y=float(rng()%260)-10+.5f;float size=float(rng()%900)/100;
   ScreenVertex v[3];for(unsigned j=0;j<3;++j)v[j]={x+float(int(rng()%201)-100)/100*size,y+float(int(rng()%201)-100)/100*size,float(rng()%1000+1)/1000,float(int(rng()%100)-50)/10,float(int(rng()%100)-50)/10,float(rng()%256),float(rng()%256),float(rng()%256),float(rng()%256),0};
   s.blend=k%6;s.zWrite=k%3;s.cull=1+k%3;s.alphaRef=k%256;s.linear=k%2;s.addressU=s.addressV=1+k%4;
   const Texture* tex=k%3?&t:nullptr;a.screenTriangle(v,tex,s);b.screenTriangle(v,tex,s);
   assert(a.pixels()==b.pixels()&&a.depth()==b.depth()&&a.stats().pixels==b.stats().pixels);
   if(k%1777==0){a.clearUiDepth();b.clearUiDepth();}
  }
  // The opaque flat bridge has a different tint/modulation contract and its
  // own UV setup. Compare that route as well, including alpha masks.
  for(unsigned k=0;k<3000;++k){float x=float(rng()%320)+.5f,y=float(rng()%240)+.5f;
   ScreenVertex v[3];for(unsigned j=0;j<3;++j)v[j]={x+float(rng()%800)/100,y+float(rng()%800)/100,float(rng()%1000+1)/1000,float(int(rng()%100)-50)/10,float(int(rng()%100)-50)/10,255,255,255,255,0};
   uint16_t tint=uint16_t(rng());unsigned address=1+k%4,alpha=k%256;
   a.texturedProjectedTriangle(v,tint,t,address,address,alpha);b.texturedProjectedTriangle(v,tint,t,address,address,alpha);
   assert(a.pixels()==b.pixels()&&a.depth()==b.depth()&&a.stats().pixels==b.stats().pixels);
  }
  // Build the usual immutable minima with large draws, then exercise the
  // new small-bbox queries. A later depth clear must invalidate those minima.
  removed+=a.stats().hiddenSampleTriangles;a.clear(1234);b.clear(1234);s=ScreenState();s.zTest=s.zWrite=true;s.perspectiveColor=true;
  for(auto p:{&a,&b}){p->screenTriangle(q,nullptr,s);p->screenTriangle(r,nullptr,s);}
  s.zWrite=false;s.blend=1;
  ScreenVertex cover[3]={{0,0,.1f,0,0,150,120,80,128,0},{320,0,.1f,1,0,150,120,80,128,0},{0,240,.1f,0,1,150,120,80,128,0}};
  for(unsigned k=0;k<300;++k){a.screenTriangle(cover,nullptr,s);b.screenTriangle(cover,nullptr,s);}
  ScreenVertex small[3]={{40,40,.1f,0,0,150,120,80,128,0},{52,40,.1f,1,0,180,120,80,128,0},{40,52,.1f,0,1,150,180,80,128,0}};
  float step=12*(scale==1?1.f:scale==2?2.f:scale==3?4.f/3:4.f);small[1].x=40+step;small[2].y=40+step;
  a.screenTriangle(small,&t,s);b.screenTriangle(small,&t,s);
  assert(a.stats().smallTileTriangles>0);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());
  a.clearUiDepth();b.clearUiDepth();auto coarse=a.stats().smallTileTriangles;
  a.screenTriangle(small,&t,s);b.screenTriangle(small,&t,s);
  assert(a.stats().smallTileTriangles==coarse);assert(a.pixels()==b.pixels()&&a.depth()==b.depth());
  removed+=a.stats().hiddenSampleTriangles;
  // Unconditional and HUD depth comparisons have different semantics.
  auto checks=a.stats().hiddenSampleChecks;s.zTest=false;a.screenTriangle(q,&t,s);assert(a.stats().hiddenSampleChecks==checks);
  s.zTest=true;s.screenDepth=true;a.screenTriangle(q,&t,s);assert(a.stats().hiddenSampleChecks==checks);
 }
 assert(removed>100);printf("PASS 72000 small triangle coverage/depth/material comparisons; removed=%llu\n",(unsigned long long)removed);
}
