#include "soft_renderer.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){try{Renderer r;ScreenState s;s.zTest=s.zWrite=true;
 ScreenVertex v[4]={{0,0,1,0,0,255,0,0,255},{320,0,1,1,0,255,0,0,255},{0,240,1,0,1,255,0,0,255},{320,240,1,1,1,255,0,0,255}};
 for(unsigned scale:{1u,2u,3u,4u}){r.renderScale(scale);r.clear(0);ScreenVertex a[3]={v[0],v[1],v[2]},b[3]={v[2],v[1],v[3]};r.screenTriangle(a,nullptr,s);r.screenTriangle(b,nullptr,s);
  check(r.stats().pixels==(scale==3?240*180:320*240/(scale*scale)),"true reduced pixel workload");for(auto p:r.pixels())check(p==rgb(255,0,0),"nearest expansion fills all output pixels");
  r.clear(rgb(0,0,255));check(r.pixels()[0]==rgb(0,0,255),"cached frame invalidated on clear");
  Camera c;c.position={0,0,0};Vec3 tri[3]={{-2,-2,5},{2,-2,5},{0,2,5}};r.triangle(tri,rgb(0,255,0),c);check(r.pixels()[120*320+160]==rgb(0,255,0),"flat 3D projects correctly at reduced resolution");
  auto expanded=r.pixels();auto oldDepth=r.depth();r.resolveFullResolution();check(r.renderScale()==1&&r.pixels()==expanded,"world resolve preserves image for sharp HUD");
  for(int y=0;y<240;++y)for(int x=0;x<320;++x)check(r.depth()[y*320+x]==oldDepth[(y*(scale==3?180:240/scale)/240)*320+x*(scale==3?240:320/scale)/320],"world depth resolves consistently");
  ScreenVertex line[3]={{10,10,1,0,0,255,255,255,255},{11,10,1,0,0,255,255,255,255},{10,12,1,0,0,255,255,255,255}};ScreenState overlay;r.screenTriangle(line,nullptr,overlay);check(r.pixels()[10*320+10]==rgb(255,255,255)&&r.pixels()[10*320+11]!=rgb(255,255,255),"HUD retains individual 320x240 pixels");
  Vec2 uv[3]={{0,0},{1,0},{0,1}};Texture t{1,1,{rgb(255,255,255)},{64}};r.clear(0);r.texturedTriangle(tri,uv,rgb(255,0,0),t,3,3,c,0);check(r.pixels()[120*320+160]==rgb(255,0,0),"opaque alpha-always fast texture path");
  r.clear(0);r.texturedTriangle(tri,uv,rgb(255,0,0),t,3,3,c,128);check(r.pixels()[120*320+160]==0,"fast texture alpha cutoff retained");
 }
 r.renderScale(3);r.clear(rgb(0,0,255));auto before=r.pixels();r.resolveNativeUi();check(r.outputWidth()==640&&r.outputHeight()==480&&r.pixels().size()==640*480,"native UI frame dimensions");
 for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x)check(r.pixels()[y*640+x]==before[(y/2)*320+x/2],"world expands into native UI frame");
 ScreenVertex fine[3]={{10,10,1,0,0,255,255,255,255},{10.5f,10,1,0,0,255,255,255,255},{10,11,1,0,0,255,255,255,255}};ScreenState overlay;r.screenTriangle(fine,nullptr,overlay);
 check(r.pixels()[20*640+20]==rgb(255,255,255)&&r.pixels()[20*640+21]!=rgb(255,255,255),"native UI preserves individual 640x480 pixels");
 r.resolveNativeUi();check(r.outputWidth()==640,"UI resolve idempotent");r.clear(0);check(r.outputWidth()==320&&r.pixels().size()==320*240,"next world frame restores reduced target");
 // Background choices must work even after a 640 HUD, with independent sharp text.
 for(unsigned bg:{240u,320u,640u}){r.clear(rgb(20,40,60));r.beginHud(640);r.beginBackground(bg);ScreenVertex full[3]={{0,0,1,0,0,40,90,160,255},{320,0,1,0,0,40,90,160,255},{0,240,1,0,0,40,90,160,255}};r.screenTriangle(full,nullptr,overlay);r.beginUi(true);r.screenTriangle(fine,nullptr,overlay);check(r.outputWidth()==640,"menu text target remains native for every background");check(r.pixels()[20*640+20]==rgb(255,255,255)&&r.pixels()[20*640+21]!=rgb(255,255,255),"menu text stays sharp after low resolution background");}
 bool rejected=false;try{r.renderScale(5);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid scale rejected");puts("PASS 320x240 / 240x180 / 160x120 / 80x60, sample counts, expansion, depth, alpha and cache invalidation, true 640x480 UI");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
