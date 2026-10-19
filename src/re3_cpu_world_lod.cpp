#include "common.h"
#include "re3_cpu_world_lod.hpp"
#include "VisibilityPlugins.h"
#include "ModelInfo.h"
#include "ModelIndices.h"
#include "Pools.h"
#include "Treadable.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "software_device.hpp"
#include "core_menu.hpp"
#include "re3_cpu_host.hpp"
#include "world_lod_policy.hpp"
#include "world_proxy_policy.hpp"
#include "Renderer.h"
#include <unordered_map>
#include <array>
#include <vector>
#include <cstdio>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
namespace {
using Pose=std::array<float,12>;
std::unordered_map<CSimpleModelInfo*,std::vector<Pose>> placements;
bool initialized=false,enabled=false;
Pose matrixValues(const CMatrix& m){Pose f;for(unsigned k=0;k<4;++k)for(unsigned j=0;j<3;++j)f[k*3+j]=m.f[k][j];return f;}
Pose matrixValues(const rw::Matrix& m){Pose f;const rw::V3d v[]={m.right,m.up,m.at,m.pos};for(unsigned k=0;k<4;++k){f[k*3]=v[k].x;f[k*3+1]=v[k].y;f[k*3+2]=v[k].z;}return f;}
bool sameMatrix(const Pose& a,const Pose& b){for(unsigned k=0;k<12;++k)if(!std::isfinite(a[k])||!std::isfinite(b[k])||std::fabs(a[k]-b[k])>.001f)return false;return true;}
bool regular(CSimpleModelInfo* m){return m&&m->GetModelType()==MITYPE_SIMPLE&&m->m_numAtomics==1&&!m->m_firstDamaged&&!m->m_isBigBuilding&&!m->m_noFade&&!m->m_drawLast&&!m->m_additive&&!m->m_noZwrite&&!m->m_isSubway
#ifdef EXTRA_MODEL_FLAGS
 &&!m->m_bIsTree
#endif
 ;}
void initialize(){auto pool=CPools::GetBuildingPool();if(!pool)return;initialized=true;
 for(int i=0;i<pool->GetSize();++i){auto b=pool->GetSlot(i);if(!b)continue;int id=b->GetModelIndex();if(id<0||id>=MODELINFOSIZE)continue;auto base=CModelInfo::GetModelInfo(id);if(!base||base->GetModelType()!=MITYPE_SIMPLE)continue;auto m=static_cast<CSimpleModelInfo*>(base);if(regular(m))placements[m].push_back(matrixValues(b->GetMatrix()));}
 std::printf("WORLD LOD static_models=%zu; original mesh simplification, no map/streaming changes\n",placements.size());
}
}
// Intermediate geometry is derived from the original mesh. Native far-model
// visibility/streaming is unchanged; roads are never registered here.
namespace {
struct WorldProxy {Pose pose;bool active=false;rw::Geometry* attempted=nullptr;unsigned retry=0;explicit WorldProxy(Pose p):pose(p){}};
std::unordered_map<CSimpleModelInfo*,std::vector<WorldProxy>> proxies;
bool proxiesReady=false,appearanceEnabled=false;int proxyOverride=-1;uint64_t proxyUses=0;
bool proxyEnabled(){return proxyOverride>=0?proxyOverride!=0:appearanceEnabled;}
void initializeProxies(){auto pool=CPools::GetBuildingPool();if(!pool)return;
 std::vector<CBaseModelInfo*> excluded;
 for(int id=0;id<MODELINFOSIZE;++id)if(IsTreeModel(id))excluded.push_back(CModelInfo::GetModelInfo(id));
 auto roads=CPools::GetTreadablePool();unsigned roadCount=0;
 for(int i=0;roads&&i<roads->GetSize();++i){auto road=roads->GetSlot(i);if(!road)continue;excluded.push_back(CModelInfo::GetModelInfo(road->GetModelIndex()));++roadCount;}
 unsigned count=0;
 for(int i=0;i<pool->GetSize();++i){auto e=pool->GetSlot(i);if(!e||e->GetIsATreadable())continue;
  auto base=CModelInfo::GetModelInfo(e->GetModelIndex());
  if(!base||base->GetModelType()!=MITYPE_SIMPLE||std::find(excluded.begin(),excluded.end(),base)!=excluded.end())continue;
  auto model=static_cast<CSimpleModelInfo*>(base);
  if(!model->m_numAtomics||model->m_firstDamaged||model->m_drawLast||model->m_additive||model->m_noZwrite||model->m_isSubway)continue;
  proxies[model].push_back(WorldProxy(matrixValues(e->GetMatrix())));++count;
 }
 proxiesReady=true;std::printf("WORLD INTERMEDIATE static_models=%zu placements=%u excluded_roads=%u; original textures and ground faces\n",proxies.size(),count,roadCount);
}
rw::Geometry* worldProxyGeometry(rw::Atomic* a){auto g=a?a->geometry:nullptr;
 if(!proxyEnabled()||!a||!g||a->clump||!a->getFrame()||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing())return g;
 if(!proxiesReady)initializeProxies();auto mi=CVisibilityPlugins::GetAtomicModelInfo(a);
 // AtomicExt also holds integer clump flags, including on detached fragments.
 // Membership must precede every dereference of this union value.
 auto it=proxies.find(mi);if(it==proxies.end())return g;
 if(mi->m_alpha!=255||g->numTriangles<96)return g;
 const auto matrix=*a->getFrame()->getLTM();const auto pose=matrixValues(matrix);WorldProxy* selected=nullptr;
 for(auto& proxy:it->second)if(sameMatrix(pose,proxy.pose)){selected=&proxy;break;}
 if(!selected)return g;
 const unsigned changed=rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKPOLYGONS|rw::Geometry::LOCKPRELIGHT|rw::Geometry::LOCKNORMALS|rw::Geometry::LOCKTEXCOORDSALL;
 if(selected->attempted==g&&selected->retry&&!(g->lockedSinceInst&changed)){--selected->retry;return g;}
 if(!soft::SoftwareDevice::worldGeometrySuitable(g))return g;
 const float minScale=std::sqrt(std::min({rw::dot(matrix.right,matrix.right),rw::dot(matrix.up,matrix.up),rw::dot(matrix.at,matrix.at)}));
 const float maxScale=std::sqrt(std::max({rw::dot(matrix.right,matrix.right),rw::dot(matrix.up,matrix.up),rw::dot(matrix.at,matrix.at)}));
 // Face protection uses model Z, so tilted placements keep their original mesh.
 if(!std::isfinite(minScale+maxScale)||minScale<1e-6f||std::fabs(matrix.right.z)>1e-5f||std::fabs(matrix.up.z)>1e-5f||std::fabs(matrix.at.x)>1e-5f||std::fabs(matrix.at.y)>1e-5f||std::fabs(rw::dot(matrix.right,matrix.up))>1e-5f)return g;
 rw::Matrix transform=matrix,inverse;transform.flags=0;rw::Matrix::invert(&inverse,&transform);
 auto c=TheCamera.GetPosition();rw::V3d source{c.x,c.y,c.z},camera;rw::V3d::transformPoints(&camera,&source,1,&inverse);
 soft::Vec3 local{camera.x,camera.y,camera.z};if(!std::isfinite(local.x+local.y+local.z))return g;
 auto low=soft::SoftwareDevice::simplifiedWorld(g,&local,30.f/minScale,true);
 if(low==g){selected->active=false;if(soft::SoftwareDevice::simplifiedWorldRejected(g,true)){selected->attempted=g;selected->retry=31;}return g;}
 soft::CoreSettings settings(cpuHostSettings());unsigned width=settings.scale()==1?320:settings.scale()==2?160:settings.scale()==3?240:80;
 auto view=rw::engine->currentCamera->viewWindow;float focal=width*.5f/view.x;
 if(!std::isfinite(focal)||focal<=0)return g;
 float estimated=soft::SoftwareDevice::simplifiedError(g)*maxScale*focal*std::sqrt(1+view.x*view.x+view.y*view.y)/1.5f;
 float required=soft::worldIntermediateDistance(selected->active,estimated);
 bool use=std::isfinite(required)&&soft::SoftwareDevice::simplifiedSafeAt(g,local,required/minScale);
 selected->active=use;if(!use)return g;
 ++proxyUses;return low;
}
}
void cpuWorldAppearanceOverride(int value){proxyOverride=value;for(auto& m:proxies)for(auto& p:m.second)p.active=false;}
void cpuWorldAppearanceEnabled(bool value){if(appearanceEnabled&&!value)for(auto& m:proxies)for(auto& p:m.second)p.active=false;appearanceEnabled=value;}
bool cpuWorldAppearanceActive(){return proxyEnabled();}
uint64_t cpuWorldAppearanceUses(){return proxyUses;}
void cpuWorldAppearanceInventory(soft::SoftwareDevice& device){if(!proxiesReady)initializeProxies();unsigned ready=0,reduced=0;uint64_t input=0,output=0;
 for(auto& entry:proxies){auto model=entry.first;for(unsigned i=0;i<model->m_numAtomics;++i){auto a=model->m_atomics[i];if(!a||!soft::SoftwareDevice::worldGeometrySuitable(a->geometry))continue;auto g=a->geometry;++ready;device.simplificationBudget(1);auto low=soft::SoftwareDevice::simplifiedWorld(g,nullptr,0,true);input+=g->numTriangles;output+=low->numTriangles;
 if(low!=g){++reduced;std::printf("INTERMEDIATE MODEL name=%s input=%d output=%d error=%.4f\n",model->GetModelName(),g->numTriangles,low->numTriangles,soft::SoftwareDevice::simplifiedError(g));}}
 }
 std::printf("INTERMEDIATE INVENTORY ready=%u reduced=%u input=%llu output=%llu bytes=%zu peak_ms=%.4f\n",ready,reduced,(unsigned long long)input,(unsigned long long)output,device.simplifiedWorldBytes(),device.simplificationPeakMs());
}
void cpuWorldAppearanceSelfTest(){
 if(!proxiesReady)initializeProxies();CRenderer::ConstructRenderList();CEntity* chosen=nullptr;
 auto pool=CPools::GetBuildingPool();for(int i=0;pool&&i<pool->GetSize()&&!chosen;++i){auto e=pool->GetSlot(i);if(!e||!e->m_rwObject||e->m_rwObject->type!=rw::Atomic::ID)continue;auto m=CModelInfo::GetModelInfo(e->GetModelIndex());if(m&&m->GetModelType()==MITYPE_SIMPLE&&proxies.count(static_cast<CSimpleModelInfo*>(m)))chosen=e;}
 if(!chosen)throw std::runtime_error("intermediate self-test needs a registered static building");
 auto a=reinterpret_cast<rw::Atomic*>(chosen->m_rwObject);auto original=a->geometry;auto model=CVisibilityPlugins::GetAtomicModelInfo(a);auto matrix=a->getFrame()->matrix;auto oldOverride=proxyOverride;auto oldUses=proxyUses;
 auto restore=[&](){CVisibilityPlugins::SetAtomicModelInfo(a,model);a->geometry=original;a->getFrame()->matrix=matrix;a->getFrame()->updateObjects();cpuWorldAppearanceOverride(oldOverride);proxyUses=oldUses;};
 try{cpuWorldAppearanceOverride(0);if(worldProxyGeometry(a)!=original)throw std::runtime_error("intermediate OFF fallback");cpuWorldAppearanceOverride(1);
 CVisibilityPlugins::SetAtomicId(a,0x200);if(worldProxyGeometry(a)!=original)throw std::runtime_error("intermediate detached clump flag fallback");CVisibilityPlugins::SetAtomicModelInfo(a,model);
 a->getFrame()->matrix.pos.x+=.1f;a->getFrame()->updateObjects();if(worldProxyGeometry(a)!=original)throw std::runtime_error("intermediate moving placement fallback");
 a->geometry=nullptr;if(worldProxyGeometry(a)!=nullptr)throw std::runtime_error("intermediate missing source fallback");a->geometry=original;
 auto roads=CPools::GetTreadablePool();unsigned checked=0;for(int i=0;roads&&i<roads->GetSize();++i){auto road=roads->GetSlot(i);if(!road)continue;++checked;auto m=CModelInfo::GetModelInfo(road->GetModelIndex());if(m&&m->GetModelType()==MITYPE_SIMPLE&&proxies.count(static_cast<CSimpleModelInfo*>(m)))throw std::runtime_error("intermediate registered a road model");}
 if(!checked)throw std::runtime_error("intermediate road exclusion test empty");restore();puts("PASS WORLD INTERMEDIATE guards: OFF, detached-clump flags, moving placement, missing source, all road models excluded; state restored");
 }catch(...){restore();throw;}
}

void cpuWorldLodReset(){placements.clear();initialized=false;proxies.clear();proxiesReady=false;}
void cpuWorldLodEnabled(bool value){enabled=value;}
rw::Geometry* cpuWorldLodGeometry(rw::Atomic* a){
 auto g=a?a->geometry:nullptr;auto proxy=worldProxyGeometry(a);if(proxy!=g)return proxy;if(!a||!g||RpAtomicGetClump(a)||!enabled||!a->getFrame())return g;
 soft::CoreSettings settings(cpuHostSettings());if(!settings.fastEffects()||CCutsceneMgr::IsRunning()||CCutsceneMgr::IsCutsceneProcessing())return g;
 if(!initialized)initialize();auto mi=CVisibilityPlugins::GetAtomicModelInfo(a);auto it=placements.find(mi);if(it==placements.end())return g;
 // Only static building pool instances, never props, doors, wheels, peds or
 // vehicles. Edited/replaced placement matrices use the original geometry.
 auto current=matrixValues(*a->getFrame()->getLTM());bool registered=false;for(const auto& pose:it->second)if(sameMatrix(pose,current)){registered=true;break;}if(!registered)return g;
 unsigned width=settings.scale()==1?320:settings.scale()==3?240:settings.scale()==2?160:80;
 if(g->numTriangles<96||g->numTriangles>4096||g->numVertices>8192)return g;
 auto matrix=*a->getFrame()->getLTM();float minScale=std::sqrt(std::min({rw::dot(matrix.right,matrix.right),rw::dot(matrix.up,matrix.up),rw::dot(matrix.at,matrix.at)})),maxScale=std::sqrt(std::max({rw::dot(matrix.right,matrix.right),rw::dot(matrix.up,matrix.up),rw::dot(matrix.at,matrix.at)}));
 if(!std::isfinite(minScale+maxScale)||minScale<1e-6f||std::fabs(rw::dot(matrix.right,matrix.up))>1e-5f||std::fabs(rw::dot(matrix.right,matrix.at))>1e-5f||std::fabs(rw::dot(matrix.up,matrix.at))>1e-5f)return g;
 float focal=width*.5f/rw::engine->currentCamera->viewWindow.x;if(!std::isfinite(focal)||focal<=0)return g;
 auto window=rw::engine->currentCamera->viewWindow;float corner=std::sqrt(1+window.x*window.x+window.y*window.y);
 float nearDistance=std::max(12.f,.35f*maxScale*focal*corner*1.3f/1.5f)/minScale;
 matrix.flags=0;rw::Matrix inverse;rw::Matrix::invert(&inverse,&matrix);auto worldCamera=TheCamera.GetPosition();rw::V3d camera;rw::V3d source={worldCamera.x,worldCamera.y,worldCamera.z};rw::V3d::transformPoints(&camera,&source,1,&inverse);soft::Vec3 localCamera{camera.x,camera.y,camera.z};if(!std::isfinite(camera.x+camera.y+camera.z))return g;
 auto low=soft::SoftwareDevice::simplifiedWorld(g,&localCamera,std::max(12.f,20.f*width/240.f)/minScale);
 nearDistance=std::max(12.f,soft::SoftwareDevice::simplifiedError(g)*maxScale*focal*corner*1.3f/1.5f)/minScale;
 return low!=g&&soft::SoftwareDevice::simplifiedSafeAt(g,localCamera,nearDistance)?low:g;
}
void cpuWorldLodAudit(){if(!initialized)initialize();std::printf("WORLD LOD audit static_models=%zu\n",placements.size());}

void cpuWorldLodInventory(soft::SoftwareDevice& device){if(!initialized)initialize();unsigned ready=0,reduced=0;unsigned long long input=0,output=0;for(auto& entry:placements){auto mi=entry.first;if(!mi->m_atomics[0])continue;auto g=mi->m_atomics[0]->geometry;if(!g||!soft::SoftwareDevice::worldGeometrySuitable(g))continue;++ready;device.simplificationBudget(1);auto low=soft::SoftwareDevice::simplifiedWorld(g);input+=g->numTriangles;output+=low->numTriangles;if(low!=g){++reduced;std::printf("SIMPLIFY MODEL name=%s input=%d output=%d vertices=%d->%d\n",mi->GetModelName(),g->numTriangles,low->numTriangles,g->numVertices,low->numVertices);}}std::printf("SIMPLIFY INVENTORY ready=%u reduced=%u input=%llu output=%llu bytes=%zu peak_ms=%.4f\n",ready,reduced,input,output,device.simplifiedWorldBytes(),device.simplificationPeakMs());}
