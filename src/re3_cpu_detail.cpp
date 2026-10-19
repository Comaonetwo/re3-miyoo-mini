#include "common.h"
#include "re3_cpu_detail.hpp"
#include "detail_policy.hpp"
#include "software_device.hpp"
#include "Entity.h"
#include "Object.h"
#include "ModelInfo.h"
#include "Camera.h"
#include "CutsceneMgr.h"
namespace {soft::SoftwareDevice* device=nullptr;bool decor=false,lighting=false;int overrideMode=-1;uint64_t skipped=0;}
void cpuDetailSettings(soft::SoftwareDevice* d,bool a,bool b){device=d;decor=a;lighting=b;}
void cpuDetailOverride(int mode){overrideMode=mode;}
void cpuDetailReset(){skipped=0;}
uint64_t cpuDetailSkipped(){return skipped;}
CpuDetailScope::CpuDetailScope(CEntity* e){
 if(!device||overrideMode==0||(overrideMode<0&&!decor&&!lighting))return;armed_=true;previous_=device->farNormalLighting();previousSurface_=device->farSurfaceLighting();device->farNormalLighting(false);device->farSurfaceLighting(false);
 bool useDecor=overrideMode<0?decor:(overrideMode&1),useLighting=overrideMode<0?lighting:(overrideMode&2);
 if((!useDecor&&!useLighting)||!e||(!e->IsBuilding()&&!e->IsObject()&&!e->IsDummy())||e->bDrawLast||e->bRenderDamaged||e->bDistanceFade||!e->m_rwObject||e->m_rwObject->type!=rw::Atomic::ID||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing())return;
 int id=e->GetModelIndex();if(id<0||id>=MODELINFOSIZE)return;auto base=CModelInfo::GetModelInfo(id);if(!base||base->GetModelType()!=MITYPE_SIMPLE)return;auto mi=static_cast<CSimpleModelInfo*>(base);
 if(mi->m_firstDamaged||mi->m_drawLast||mi->m_noZwrite||mi->m_additive||mi->m_isSubway)return;
 if(e->IsObject()){auto o=static_cast<CObject*>(e);if(o->ObjectCreatedBy!=GAME_OBJECT||!o->GetIsStatic()||o->bIsPickup||o->bHasBeenDamaged)return;}
 if(!base->GetColModel())return;float radius=e->GetBoundRadius();float distance=(e->GetBoundCentre()-TheCamera.GetPosition()).Magnitude();if(!std::isfinite(radius+distance)||radius<0)return;
 if(useDecor&&!e->bIsBIGBuilding&&!mi->m_isBigBuilding&&!base->GetNum2dEffects()&&soft::decorName(base->GetModelName())&&soft::distantDecor(radius,distance,mi->GetLargestLodDistance())){skip=true;++skipped;return;}
 // Surface-distance guard protects near faces of large models. No mesh swap.
 if(useLighting&&e->IsBuilding()&&base->GetObjectID()==-1){auto a=reinterpret_cast<rw::Atomic*>(e->m_rwObject);if(!a->clump&&soft::SoftwareDevice::opaqueGeometry(a->geometry)){device->farSurfaceLighting(true);if(distance-radius>55)device->farNormalLighting(true);}}
}
CpuDetailScope::~CpuDetailScope(){if(armed_&&device){device->farNormalLighting(previous_);device->farSurfaceLighting(previousSurface_);}}
