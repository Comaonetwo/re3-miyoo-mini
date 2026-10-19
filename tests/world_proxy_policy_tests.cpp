#include "world_proxy_policy.hpp"
#include "core_menu.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main(int argc,char** argv){using namespace soft;assert(argc==2);
 assert(std::fabs(worldIntermediateDistance(false,0)-30)<.0001f);assert(worldIntermediateDistance(true,0)==25);
 assert(worldIntermediateDistance(false,50)>59&&worldIntermediateDistance(false,50)<61);assert(worldIntermediateDistance(true,50)==50);
 for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})assert(!std::isfinite(worldIntermediateDistance(true,bad)));
 CoreSettings original;original.applyPreset(2);unsigned bits=original.bits|(1u<<21);FILE* f=std::fopen(argv[1],"wb");assert(f);std::fprintf(f,"GTA3_CPU_SETTINGS 1\n%u\nGTA3_CUSTOM %u 1\nGTA3_WORLD_LOD 2\nGTA3_RENDER_THREADS 2\n",bits,bits&CoreSettings::graphicsMask);std::fclose(f);auto loaded=CoreSettings::load(argv[1]);assert(!loaded.experimentalWorld()&&!(loaded.customGraphics&(1u<<21))&&!loaded.parallelRenderer());assert(!CoreSettings(bits).experimentalWorld());for(unsigned p=0;p<4;++p){loaded.applyPreset(p);assert(!loaded.experimentalWorld());}std::remove(argv[1]);puts("PASS hidden world LOD stays disabled after legacy load and preset changes");}
