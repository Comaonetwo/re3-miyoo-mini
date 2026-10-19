#include "core_menu.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace soft;
int main(int argc,char** argv){assert(argc==2);CoreMenu menu;menu.settings.applyPreset(1);menu.open=true;menu.row=3;
 const unsigned values[]={40,60,80,100,120,140,180};for(unsigned i=0;i<7;++i){menu.settings.distanceIndex(i);assert(menu.settings.distance()==values[i]&&CoreSettings::valid(menu.settings.bits));}
 menu.settings.distanceIndex(4);menu.key(CoreKey::Right);assert(menu.settings.distance()==140&&menu.settings.preset()==4);menu.key(CoreKey::Right);assert(menu.settings.distance()==180);menu.key(CoreKey::Left);assert(menu.settings.distance()==140);
 assert(menu.settings.save(argv[1]));auto saved=CoreSettings::load(argv[1]);assert(saved.distance()==140&&saved.preset()==4);saved.applyPreset(2);assert(saved.distance()==100&&saved.preset()==2);saved.applyPreset(4);assert(saved.distance()==140&&saved.preset()==4);
 for(unsigned p=0;p<4;++p){saved.applyPreset(p);assert(saved.distance()==(p==0?80:100));}
 for(unsigned old:{0u,4u,8u,12u}){CoreSettings s(old);assert(s.distance()==40+20*(old>>2));}
 assert(!CoreSettings::valid(3u<<19));assert(!CoreSettings::valid((1u<<19)|8u));assert(!CoreSettings::valid(1u<<27));assert(CoreSettings::valid((1u<<27)|(1u<<17)));std::remove(argv[1]);puts("PASS 40/60/80/100/120/140/180 menu/save/custom, legacy settings, presets and invalid combinations");}
