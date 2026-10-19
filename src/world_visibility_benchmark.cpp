#include "soft_renderer.hpp"
#include <chrono>
#include <cstdio>
using namespace soft;
int main(){Texture t;t.width=t.height=16;t.pixels.assign(256,rgb(170,210,160));t.alpha.assign(256,255);
 for(bool enabled:{false,true}){Renderer r;r.occlusionEnabled(enabled);ScreenState s;s.zTest=s.zWrite=true;s.blend=1;s.alphaRef=1;s.perspectiveColor=true;
 auto start=std::chrono::steady_clock::now();for(unsigned frame=0;frame<100;++frame){r.clear(0);
 ScreenVertex wall[4]={{0,0,2,0,0,170,190,160,255,0},{320,0,2,1,0,170,190,160,255,0},{0,240,2,0,1,170,190,160,255,0},{320,240,2,1,1,170,190,160,255,0}};
 ScreenVertex a[3]={wall[0],wall[1],wall[2]},b[3]={wall[2],wall[1],wall[3]};r.screenTriangle(a,&t,s);r.screenTriangle(b,&t,s);
 for(unsigned n=0;n<2000;++n){float x=8+n%260,y=8+n%180;ScreenVertex v[3]={{x,y,.1f,0,0,170,190,160,255,0},{x+40,y,.1f,1,0,170,190,160,255,0},{x,y+30,.15f,0,1,170,190,160,255,0}};r.screenTriangle(v,&t,s);}}
 unsigned long long hash=1469598103934665603ull;for(auto p:r.pixels()){hash^=p;hash*=1099511628211ull;}
 printf("visibility=%d %.3f ms/frame rejected=%llu checks=%llu builds=%llu hash=%llx\n",enabled,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/100,(unsigned long long)r.stats().occludedTriangles,(unsigned long long)r.stats().occlusionChecks,(unsigned long long)r.stats().occlusionBuilds,hash);
 }}
