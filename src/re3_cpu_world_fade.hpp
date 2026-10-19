#pragma once
#include <cstdint>
namespace rw {struct Atomic;}
class CEntity;class CSimpleModelInfo;
void cpuTimeFadeEnabled(bool);
uint64_t cpuTimeFadeUses();
void cpuIncreaseWorldAlpha(CEntity*,CSimpleModelInfo*);
void cpuTimeFadeSelfTest();
void cpuWorldFadeEnabled(bool);
uint64_t cpuWorldFadeUses();
unsigned cpuWorldFadeAlpha(rw::Atomic*,unsigned);
bool cpuWorldFadeBuilding(bool);
struct CpuWorldFadeScope {
    bool previous;
    explicit CpuWorldFadeScope(bool building):previous(cpuWorldFadeBuilding(building)){}
    ~CpuWorldFadeScope(){cpuWorldFadeBuilding(previous);}
};
