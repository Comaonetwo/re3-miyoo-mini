#pragma once
#include <cstdint>
class CEntity;
namespace soft {class SoftwareDevice;}
void cpuDetailSettings(soft::SoftwareDevice*,bool decor,bool lighting);
void cpuDetailOverride(int);
void cpuDetailReset();
uint64_t cpuDetailSkipped();
class CpuDetailScope {
 bool armed_=false,previous_=false,previousSurface_=false;
public:
 bool skip=false;
 explicit CpuDetailScope(CEntity*);
 ~CpuDetailScope();
};
