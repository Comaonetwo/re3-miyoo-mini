#include "soft_renderer.hpp"
#include <chrono>
#include <cstdio>
using namespace soft;
int main(){Texture t;t.width=t.height=64;
 for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x){bool ink=(x>=16&&x<24&&y>=12&&y<54)||(x>=40&&x<48&&y>=12&&y<54)||(y>=28&&y<36&&x>=16&&x<48);t.pixels.push_back(rgb(255,255,255));t.alpha.push_back(ink?255:0);}
 for(unsigned width:{240u,320u,640u}){Renderer r;ScreenState s;s.blend=1;s.linear=true;s.alphaRef=1;s.screenDepth=true;s.zTest=s.zWrite=true;
 auto start=std::chrono::steady_clock::now();for(unsigned frame=0;frame<300;++frame){r.clear(rgb(30,60,90));r.beginHud(width);
  for(unsigned i=0;i<96;++i){float x=float(i%12*24+8),y=float(i/12*22+16);ScreenVertex v[4]={{x,y,2,0,0,136,170,255,255,0},{x+14,y,2,1,0,136,170,255,255,0},{x,y+16,2,0,1,136,170,255,255,0},{x+14,y+16,2,1,1,136,170,255,255,0}};
   ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,&t,s);r.screenTriangle(b,&t,s);}
  r.pixels();}
 unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}
 printf("filtered HUD %u %.4f ms/frame alpha_only=%llu uniform=%llu hash=%llx\n",width,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/300,(unsigned long long)r.stats().filteredAlphaOnly,(unsigned long long)r.stats().filteredFlat,hash);
 }}
