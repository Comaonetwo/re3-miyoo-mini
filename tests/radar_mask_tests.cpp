#include "soft_renderer.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace soft;
struct P {float x,y;};
int main(){unsigned leaks=0,total=0;for(unsigned width:{240u,320u,640u})for(unsigned run=0;run<80;++run){Renderer r;r.renderScale(3);r.clear(rgb(10,40,70));r.beginHud(width);float factor=width/320.f,cx=42.37f+run*.013f,cy=194.71f+run*.007f,rx=28.31f,ry=33.83f;
 ScreenState mask;mask.screenDepth=true;mask.zWrite=true;mask.blend=5;mask.cull=1;
 for(int q=0;q<4;++q){float sx=(q<2?1.f:-1.f),sy=(q==0||q==3?-1.f:1.f);P fan[8];fan[0]={cx+sx*rx,cy+sy*ry};for(int j=0;j<7;++j){float a=j*float(3.14159265358979323846/12);fan[j+1]={cx+sx*rx*std::cos(a),cy+sy*ry*std::sin(a)};}
  for(int j=1;j<7;++j){P p[3]={fan[0],fan[j],fan[j+1]};ScreenVertex v[3];for(int k=0;k<3;++k){v[k]={p[k].x,p[k].y,2,0,0,255,255,255,255,0};p[k].x*=factor;p[k].y*=factor;}r.screenTriangle(v,nullptr,mask);
  }
 }
 r.clearUiDepth(); // Clip must also survive loss/overwrite of the depth mask.
 r.beginRadar(cx-rx,cy-ry,cx+rx,cy+ry);
 // Draw exactly the clipped square map with its original nominal depth.
 Texture t{1,1,{rgb(255,128,0)},{255}};ScreenState map;map.screenDepth=true;map.zTest=true;map.linear=true;map.alphaRef=1;
 ScreenVertex v[4]={{cx-rx,cy-ry,2,0,0,255,255,255,255,.0001f},{cx+rx,cy-ry,2,0,0,255,255,255,255,.0001f},{cx-rx,cy+ry,2,0,0,255,255,255,255,.0001f},{cx+rx,cy+ry,2,0,0,255,255,255,255,.0001f}};
 ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,&t,map);r.screenTriangle(b,&t,map);
 // HUD240 uses a 320-stride reduced target before compositing.
 if(width==240){for(unsigned y=0;y<180;++y)for(unsigned x=0;x<240;++x){double dx=(x+.5-cx*factor)/(rx*factor),dy=(y+.5-cy*factor)/(ry*factor);if(dx*dx+dy*dy>1.0001)assert(r.depth()[y*320+x]==0);}}
 // Native targets can be checked in the final output.
 if(width!=240){for(unsigned y=0;y<width*3/4;++y)for(unsigned x=0;x<width;++x){double dx=(x+.5-cx*factor)/(rx*factor),dy=(y+.5-cy*factor)/(ry*factor);if(dx*dx+dy*dy>1.0001){++total;if(r.pixels()[y*width+x]==rgb(255,128,0))++leaks;}}}
 r.endRadar(); // Normal HUD sprites must be able to draw outside the circle.
 ScreenState normal;normal.screenDepth=true;ScreenVertex n[3]={{0,0,2,0,0,255,255,255,255,0},{10,0,2,0,0,255,255,255,255,0},{0,10,2,0,0,255,255,255,255,0}};r.screenTriangle(n,nullptr,normal);assert(r.pixels()[0]==65535);
 }
 printf("radar mask interior samples=%u leaks=%u\n",total,leaks);assert(!leaks);
}
