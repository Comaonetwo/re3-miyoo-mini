#include "particle_density.hpp"
#include "core_menu.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main(int argc,char**argv){using namespace soft;assert(argc==2);ParticleDensity d;d.reset();
 for(unsigned i=0;i<100;++i)d.offer(i,1,{1,1,1},{0,0,0});d.finish();assert(!d.offered&&!d.skipped);for(unsigned i=0;i<100;++i)assert(d.visible(i));
 d.reset();for(unsigned i=0;i<100;++i)d.offer(i,1,{64,1,1},{0,0,0});d.finish();assert(d.offered==100&&d.skipped>20&&d.skipped<75);bool expected[100];for(unsigned i=0;i<100;++i)expected[i]=d.visible(i);
 for(unsigned frame=0;frame<4;++frame){d.reset();for(int i=99;i>=0;--i)d.offer(i,1,{64,1,1},{0,0,0});d.finish();for(unsigned i=0;i<100;++i)assert(d.visible(i)==expected[i]);}
 d.reset();for(unsigned i=0;i<1000;++i)d.offer(i,i/2,{64,1,1},{0,0,0});d.finish();assert(!d.skipped);for(unsigned i=0;i<1000;++i)assert(d.visible(i));
 d.reset();for(unsigned i=0;i<1000;++i)d.offer(i,i,{float(i*32),1,1},{0,0,0});d.finish();assert(!d.skipped);assert(d.visible(1000));
 d.reset();d.offer(0,1,{std::numeric_limits<float>::quiet_NaN(),0,0},{0,0,0});d.offer(1,1,{1e8f,0,0},{0,0,0});d.finish();assert(!d.offered&&!d.skipped);
 CoreMenu menu;menu.toggle();menu.settings.applyPreset(1);assert(menu.settings.reducedParticles()&&!menu.settings.experimentalWorld());menu.row=15;menu.key(CoreKey::Accept);assert(!menu.settings.reducedParticles()&&menu.settings.preset()==4);assert(menu.settings.save(argv[1]));auto saved=CoreSettings::load(argv[1]);assert(!saved.reducedParticles());for(unsigned p=0;p<4;++p){saved.applyPreset(p);assert(saved.reducedParticles()&&!saved.experimentalWorld());}saved.applyPreset(4);assert(!saved.reducedParticles());
 for(unsigned preset=0;preset<4;++preset){CoreSettings old;old.applyPreset(preset);unsigned bits=old.bits&~(1u<<22);FILE* f=std::fopen(argv[1],"wb");assert(f);std::fprintf(f,"GTA3_CPU_SETTINGS 1\n%u\nGTA3_CUSTOM %u 0\nGTA3_WORLD_LOD 2\n",bits,bits&CoreSettings::graphicsMask);std::fclose(f);auto migrated=CoreSettings::load(argv[1]);assert(migrated.reducedParticles()&&migrated.preset()==preset);}
 unsigned bits=menu.settings.bits|(1u<<21)|(1u<<22);FILE* f=std::fopen(argv[1],"wb");assert(f);std::fprintf(f,"GTA3_CPU_SETTINGS 1\n%u\nGTA3_CUSTOM %u 1\nGTA3_WORLD_LOD 2\n",bits,bits&CoreSettings::graphicsMask);std::fclose(f);auto migrated=CoreSettings::load(argv[1]);assert(!migrated.experimentalWorld()&&!(migrated.customGraphics&(1u<<21))&&migrated.reducedParticles());std::remove(argv[1]);
 puts("PASS density: near retention, stable slot ranks, minimum two per cell/type, 1000-slot bound, invalid input, settings/custom/presets and v0.48 migration");}
