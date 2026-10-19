#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
#include <random>
using namespace soft;
static void same(Renderer& a,Renderer& b){assert(a.pixels()==b.pixels());assert(a.depth()==b.depth());assert(a.stats().pixels==b.stats().pixels);}
int main(){Renderer reference,candidate;reference.sparseHudEnabled(false);std::mt19937 rng(8931);uint64_t full=0,sparse=0,resolves=0;
 for(unsigned frame=0;frame<240;++frame){unsigned widths[]={80,160,200,240,280,320};uint16_t bg=uint16_t(rng());for(auto r:{&reference,&candidate}){r->renderWorldWidth(widths[frame%6]);r->clear(bg);r->beginHud(240);}
  same(reference,candidate); // Empty HUD must not reveal a previous frame.
  for(unsigned n=0;n<24;++n){float x=float(int(rng()%330)-15)+.17f,y=float(int(rng()%260)-10)+.23f,w=float(rng()%90+1),h=float(rng()%40+1);unsigned a=unsigned(rng()%256);ScreenState s;s.screenDepth=true;s.blend=n%7==0?5:n%3?1:0;s.zWrite=n%7==0;s.zTest=n%4==0;s.alphaRef=n%3?1:0;
   ScreenVertex v[3]={{x,y,1,0,0,100,170,220,float(a),.5f},{x+w,y,1,1,0,100,170,220,float(a),.5f},{x,y+h,1,0,1,100,170,220,float(a),.5f}};
   Texture t{2,2,{0,65535,12345,54321},{0,128,200,255}};s.linear=n%2;
   for(auto r:{&reference,&candidate}){if(n%5==0)r->beginRadar(15,120,100,230);r->screenTriangle(v,n%3?&t:nullptr,s);r->endRadar();if(n==12)r->beginHud(240);}
   if(n%5==0)same(reference,candidate); // Repeated resolve during one HUD pass.
  }
  same(reference,candidate);full+=reference.stats().hudCompositeSamples;sparse+=candidate.stats().hudCompositeSamples;resolves+=candidate.stats().hudSparseResolves;
  if(frame%4==0){for(auto r:{&reference,&candidate})r->beginUi(true);same(reference,candidate);}
  if(frame%4==1){ScreenState s;s.screenDepth=true;s.blend=2;ScreenVertex v[3]={{0,0,1,0,0,40,50,60,128,.5f},{80,0,1,0,0,40,50,60,128,.5f},{0,80,1,0,0,40,50,60,128,.5f}};for(auto r:{&reference,&candidate})r->screenTriangle(v,nullptr,s);same(reference,candidate);}
  if(frame%4==2){for(auto r:{&reference,&candidate}){r->clearBuffers(bg,false,true);r->beginHud(240);}same(reference,candidate);}
  // Diagnostic mode may change between frames; bounds must remain valid.
  candidate.sparseHudEnabled(frame%11!=0);
 }
 assert(resolves>100&&sparse<full);printf("PASS 240 moving/disappearing HUD frames, dynamic world widths, empty/repeated resolves, masks, depth, menu/fallback/clear/toggle; full=%llu sparse=%llu resolves=%llu exact RGB/depth/core writes\n",(unsigned long long)full,(unsigned long long)sparse,(unsigned long long)resolves);
}
