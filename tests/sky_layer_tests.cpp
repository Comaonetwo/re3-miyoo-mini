#include "soft_renderer.hpp"
#include "core_menu.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
static void sky(Renderer& r,unsigned red=220){ScreenVertex v[3]={{0,0,1,0,0,float(red),130,170,83,0},{320,0,1,0,0,float(red),130,170,83,0},{0,240,1,0,0,float(red),130,170,83,0}};ScreenState s;s.blend=1;r.screenTriangle(v,nullptr,s);}
int main(){unsigned cases=0;
 for(unsigned scale:{1u,2u,3u,4u})for(unsigned target:{120u,160u}){
  Renderer r;r.renderScale(scale);r.clear(16904);auto depth=r.depth();auto image=r.pixels();unsigned world=scale==1?320:scale==2?160:scale==3?240:80;
  bool started=r.beginSky(target);assert(started==(target<world));r.endSky();assert(r.pixels()==image&&r.depth()==depth&&r.renderScale()==scale);
  if(started){assert(r.beginSky(target));sky(r);r.endSky();assert(r.stats().skyPasses==2&&r.stats().skyCompositePixels>0);assert(r.depth()==depth);assert(r.renderScale()==scale);
   assert(r.beginSky(target));sky(r,30);r.clear(21905);assert(!r.skyActive());for(auto p:r.pixels())assert(p==21905);
   assert(r.beginSky(target));sky(r);ScreenVertex v[3]={{0,0,1,0,0,255,0,0,255,0},{320,0,1,0,0,255,0,0,255,0},{0,240,1,0,0,255,0,0,255,0}};ScreenState s;s.zWrite=true;r.screenTriangle(v,nullptr,s);assert(!r.skyActive());assert(r.pixels()[10*320+10]==rgb(255,0,0));
   r.clear(1234);assert(r.beginSky(target));sky(r);r.beginUi(true);assert(!r.skyActive()&&r.outputWidth()==640);r.clear(1234);assert(r.beginSky(target));sky(r);r.beginHud(640);assert(!r.skyActive()&&r.outputWidth()==640);
  }++cases;
 }
 CoreMenu m;m.toggle();m.row=13;for(unsigned w:{120u,160u,0u}){m.key(CoreKey::Right);assert(m.settings.skyWidth()==w&&CoreSettings::valid(m.settings.bits));}m.key(CoreKey::Left);assert(m.settings.skyWidth()==160);m.row=14;m.key(CoreKey::Accept);assert(m.settings.reducedRain());m.key(CoreKey::Accept);assert(m.settings.reducedRain()&&!m.settings.lowRain());m.key(CoreKey::Accept);assert(!m.settings.reducedRain());assert(!CoreSettings::valid(3u<<15));assert(CoreSettings(932).skyWidth()==0&&!CoreSettings(932).reducedRain());
 printf("PASS %u sky layer sizes: unchanged background/depth, cleanup, fallback, native HUD, old settings and quality controls\n",cases);
}
