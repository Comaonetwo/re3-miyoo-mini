#include "common.h"
#include "re3_cpu_draw_order.hpp"
#include "Entity.h"
#include "ModelInfo.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "software_device.hpp"
#include <algorithm>
#include <cmath>
#include <new>
namespace {bool enabled=false;uint64_t sorted=0;}
void cpuOpaqueOrderEnabled(bool value){enabled=value;}
uint64_t cpuOpaqueOrderObjects(){return sorted;}
CpuOpaqueOrder::CpuOpaqueOrder(CEntity** list,int count):entities_(list){
 if(!enabled||!list||count<2||count>NUMVISIBLEENTITIES||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing())return;
 struct Entry {CEntity* entity;float distance;unsigned rank;};
 try{
  std::vector<Entry> entries;entries.reserve(count);slots_.reserve(count);original_.reserve(count);
  auto camera=TheCamera.GetPosition();
  for(int i=0;i<count;++i){auto e=list[i];if(!e||!e->IsBuilding()||e->bIsBIGBuilding||e->bDrawLast||e->bDistanceFade||!e->m_rwObject||e->m_rwObject->type!=rw::Atomic::ID)continue;
   auto base=CModelInfo::GetModelInfo(e->GetModelIndex());if(!base||base->GetModelType()!=MITYPE_SIMPLE)continue;auto mi=static_cast<CSimpleModelInfo*>(base);
   if(mi->m_drawLast||mi->m_noZwrite||mi->m_additive||mi->m_isSubway
#ifdef EXTRA_MODEL_FLAGS
      ||mi->m_bIsTree
#endif
     )continue;
   auto a=reinterpret_cast<rw::Atomic*>(e->m_rwObject);if(a->clump||!soft::SoftwareDevice::opaqueGeometry(a->geometry))continue;
   auto delta=e->GetPosition()-camera;float distance=delta.MagnitudeSqr();if(!std::isfinite(distance))continue;
   entries.push_back({e,distance,unsigned(i)});slots_.push_back(i);original_.push_back(e);
  }
  if(entries.size()<2){slots_.clear();original_.clear();return;}
  std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.distance<b.distance||(a.distance==b.distance&&a.rank<b.rank);});
  for(unsigned i=0;i<entries.size();++i)list[slots_[i]]=entries[i].entity;
  sorted+=entries.size();
 }catch(const std::bad_alloc&){slots_.clear();original_.clear();}
}
CpuOpaqueOrder::~CpuOpaqueOrder(){for(unsigned i=0;i<slots_.size();++i)entities_[slots_[i]]=original_[i];}
