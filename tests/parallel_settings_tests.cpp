#include "core_menu.hpp"
#include <cassert>
#include <cstdio>
using namespace soft;
int main(int argc,char**argv){assert(argc==2);CoreSettings s;s.applyPreset(2);auto graphics=s.bits;
for(unsigned threads:{0u,1u,2u,3u,99u}){FILE* f=std::fopen(argv[1],"wb");assert(f);std::fprintf(f,"GTA3_CPU_SETTINGS 1\n%u\nGTA3_CUSTOM %u 0\nGTA3_WORLD_LOD 2\nGTA3_RENDER_THREADS %u\n",graphics,graphics&CoreSettings::graphicsMask,threads);std::fclose(f);auto loaded=CoreSettings::load(argv[1]);assert(!loaded.parallelRenderer()&&loaded.preset()==2);assert(loaded.save(argv[1])&&!CoreSettings::load(argv[1]).parallelRenderer());}
CoreMenu m;m.toggle();m.row=2;m.key(CoreKey::Accept);assert(!m.settings.dynamicWorld()&&!m.settings.parallelRenderer());assert(CoreSettings::valid(1u<<26)&&!CoreSettings::valid(1u<<27));std::remove(argv[1]);puts("PASS retired parallel UI, legacy thread migration to one core, adaptive menu and settings validity");}
