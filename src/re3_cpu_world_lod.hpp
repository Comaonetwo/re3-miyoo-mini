#pragma once
#ifndef RWCORE_H
#include <rw.h>
#endif
void cpuWorldLodReset();
rw::Geometry* cpuWorldLodGeometry(rw::Atomic*);
void cpuWorldLodAudit();
void cpuWorldLodEnabled(bool);

namespace soft {class SoftwareDevice;}
void cpuWorldLodInventory(soft::SoftwareDevice&);

void cpuWorldAppearanceOverride(int);
void cpuWorldAppearanceEnabled(bool);
uint64_t cpuWorldAppearanceUses();
bool cpuWorldAppearanceActive();
void cpuWorldAppearanceSelfTest();

void cpuWorldAppearanceInventory(soft::SoftwareDevice&);
