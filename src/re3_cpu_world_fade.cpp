#include "common.h"
#include "re3_cpu_world_fade.hpp"
#include "VisibilityPlugins.h"
#include "ModelInfo.h"
#include "CutsceneMgr.h"
#include "software_device.hpp"
#include "Entity.h"
#include "Timer.h"
#include "world_fade_policy.hpp"
#include <array>
#include "Pools.h"
#include "Treadable.h"
#include <stdexcept>
#include <cstdio>
namespace {bool enabled=false,building=false;uint64_t uses=0;}
namespace {bool timeEnabled=false;uint64_t timeUses=0;std::array<uint32_t,MODELINFOSIZE> advancedFrame{};std::array<bool,MODELINFOSIZE> advancedReady{};}
void cpuTimeFadeEnabled(bool value){timeEnabled=value;}
uint64_t cpuTimeFadeUses(){return timeUses;}
void cpuIncreaseWorldAlpha(CEntity* e,CSimpleModelInfo* mi){
 if(!timeEnabled||!e||!e->IsBuilding()||e->bIsBIGBuilding||mi->m_alpha>=239||soft::worldFadeStep(CTimer::GetTimeStepNonClipped())==16||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing()||mi->m_isBigBuilding||mi->m_drawLast||mi->m_additive||mi->m_noZwrite||mi->m_isSubway
#ifdef EXTRA_MODEL_FLAGS
    ||mi->m_bIsTree
#endif
   ){mi->IncreaseAlpha();return;}
 int id=e->GetModelIndex();uint32_t frame=CTimer::GetFrameCounter();
 if(id<0||id>=MODELINFOSIZE||(advancedReady[id]&&advancedFrame[id]==frame)){mi->IncreaseAlpha();return;}
 auto a=e->m_rwObject&&e->m_rwObject->type==rw::Atomic::ID?reinterpret_cast<rw::Atomic*>(e->m_rwObject):nullptr;
 if(!a||a->clump||!soft::SoftwareDevice::opaqueGeometry(a->geometry)){mi->IncreaseAlpha();return;}
 mi->m_alpha=soft::nextWorldAlpha(mi->m_alpha,CTimer::GetTimeStepNonClipped());advancedFrame[id]=frame;advancedReady[id]=true;++timeUses;
}
void cpuTimeFadeSelfTest(){
 CEntity* e=nullptr;CSimpleModelInfo* mi=nullptr;
 auto choose=[&](CEntity* candidate){if(!candidate||candidate->bIsBIGBuilding||!candidate->m_rwObject||candidate->m_rwObject->type!=rw::Atomic::ID)return;
  auto base=CModelInfo::GetModelInfo(candidate->GetModelIndex());if(!base||base->GetModelType()!=MITYPE_SIMPLE)return;auto model=static_cast<CSimpleModelInfo*>(base);
  if(model->m_isBigBuilding||model->m_drawLast||model->m_additive||model->m_noZwrite||model->m_isSubway
#ifdef EXTRA_MODEL_FLAGS
    ||model->m_bIsTree
#endif
    )return;
  auto atomic=reinterpret_cast<rw::Atomic*>(candidate->m_rwObject);if(atomic->clump||!soft::SoftwareDevice::opaqueGeometry(atomic->geometry))return;e=candidate;mi=model;
 };
 auto pool=CPools::GetBuildingPool();for(int i=0;pool&&i<pool->GetSize()&&!e;++i)choose(pool->GetSlot(i));
 auto treadables=CPools::GetTreadablePool();for(int i=0;treadables&&i<treadables->GetSize()&&!e;++i)choose(treadables->GetSlot(i));
 if(!e)throw std::runtime_error("time fade self-test has no loaded opaque building");
 auto frame=CTimer::GetFrameCounter();float raw=CTimer::GetTimeStepNonClipped();unsigned alpha=mi->m_alpha;bool oldEnabled=timeEnabled;int id=e->GetModelIndex();auto oldFrame=advancedFrame[id];bool oldReady=advancedReady[id];auto oldUses=timeUses;bool oldNoZ=mi->m_noZwrite;
 auto restore=[&](){mi->m_alpha=alpha;mi->m_noZwrite=oldNoZ;CTimer::SetFrameCounter(frame);CTimer::SetTimeStepNonClipped(raw);timeEnabled=oldEnabled;advancedFrame[id]=oldFrame;advancedReady[id]=oldReady;timeUses=oldUses;};
 try{
  timeEnabled=true;CTimer::SetTimeStepNonClipped(50.f/15);CTimer::SetFrameCounter(frame+100);advancedReady[id]=false;mi->m_alpha=0;
  cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=32)throw std::runtime_error("time fade 15 FPS increment");
  cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=48)throw std::runtime_error("time fade extra step repeated within one frame");
  CTimer::SetFrameCounter(frame+101);cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=80)throw std::runtime_error("time fade next frame");
  timeEnabled=false;cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=96)throw std::runtime_error("time fade standard fallback");
  timeEnabled=true;mi->m_noZwrite=true;CTimer::SetFrameCounter(frame+102);cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=112)throw std::runtime_error("time fade no-Z-write exclusion");
  mi->m_noZwrite=oldNoZ;CTimer::SetTimeStepNonClipped(50.f/60);cpuIncreaseWorldAlpha(e,mi);if(mi->m_alpha!=128)throw std::runtime_error("time fade 60 FPS baseline");
  restore();puts("PASS time fade real building: 15 FPS, once per model/frame, standard/no-Z-write/60 FPS fallbacks and state restore");
 }catch(...){restore();throw;}
}
void cpuWorldFadeEnabled(bool value){enabled=value;}
bool cpuWorldFadeBuilding(bool value){bool old=building;building=value;return old;}
uint64_t cpuWorldFadeUses(){return uses;}
unsigned cpuWorldFadeAlpha(rw::Atomic* a,unsigned alpha){
 if(!enabled||!building||!a||!a->geometry||a->clump||!alpha||alpha>=255||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing())return alpha;
 auto mi=CVisibilityPlugins::GetAtomicModelInfo(a);
 if(!mi||mi->GetModelType()!=MITYPE_SIMPLE||mi->m_alpha!=255||mi->m_isBigBuilding||mi->m_drawLast||mi->m_additive||mi->m_noZwrite||mi->m_isSubway
#ifdef EXTRA_MODEL_FLAGS
    ||mi->m_bIsTree
#endif
   )return alpha;
 // Retain authored masks and streamed-model fade-in. Only compress the
 // distance fade of fully loaded opaque buildings, without moving its outer edge.
 if(!soft::SoftwareDevice::opaqueGeometry(a->geometry))return alpha;
 ++uses;return std::min(255u,alpha*2);
}
