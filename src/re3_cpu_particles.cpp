#include "common.h"
#include "Particle.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "core_menu.hpp"
#include "re3_cpu_host.hpp"
#include "particle_density.hpp"
#include <cstdlib>
#include <cstring>
extern CParticle gParticleArray[1000];
namespace {soft::ParticleDensity density;int overrideMode=-1;bool active=false;
bool eligible(int type){switch(type){
 case PARTICLE_FLAME:case PARTICLE_CARFLAME:case PARTICLE_SMOKE:case PARTICLE_SMOKE_SLOWMOTION:
 case PARTICLE_ENGINE_SMOKE:case PARTICLE_ENGINE_SMOKE2:case PARTICLE_CARFLAME_SMOKE:case PARTICLE_FIREBALL_SMOKE:
 case PARTICLE_STEAM:case PARTICLE_STEAM2:case PARTICLE_STEAM_NY:case PARTICLE_STEAM_NY_SLOWMOTION:case PARTICLE_ENGINE_STEAM:
 case PARTICLE_EXHAUST_FUMES:case PARTICLE_RUBBER_SMOKE:case PARTICLE_BURNINGRUBBER_SMOKE:
 case PARTICLE_RAINDROP:case PARTICLE_RAINDROP_SMALL:case PARTICLE_RAIN_SPLASH:case PARTICLE_RAIN_SPLASH_BIGGROW:case PARTICLE_RAIN_SPLASHUP:return true;
 default:return false;}}
unsigned slot(const CParticle* p){auto address=reinterpret_cast<uintptr_t>(p),start=reinterpret_cast<uintptr_t>(gParticleArray);if(address<start||address-start>=sizeof(gParticleArray)||(address-start)%sizeof(CParticle))return 1000;return unsigned((address-start)/sizeof(CParticle));}
}
void cpuParticleReductionOverride(int value){overrideMode=value;}
bool cpuParticleReducedActive(){return active;}
unsigned cpuParticleCandidates(){return density.offered;}
unsigned cpuParticleSkipped(){return density.skipped;}
bool cpuBeginParticles(){const char* env=std::getenv("GTA3_REDUCED_PARTICLES");bool requested=overrideMode>=0?overrideMode!=0:env? !std::strcmp(env,"1"):soft::CoreSettings(cpuHostSettings()).reducedParticles();
 active=requested&&!CCutsceneMgr::IsRunning()&&!CCutsceneMgr::IsCutsceneProcessing();density.offered=density.skipped=0;if(!active)return false;
 density.reset();auto c=TheCamera.GetPosition();soft::Vec3 camera{c.x,c.y,c.z};
 for(unsigned type=0;type<MAX_PARTICLES;++type){auto& system=mod_ParticleSystemManager.m_aParticles[type];if(!eligible(type)||(system.Flags&(SCREEN_TRAIL|SPEED_TRAIL|DRAWTOP2D)))continue;
  unsigned visited=0;for(auto p=system.m_pParticles;p&&visited++<1000;p=p->m_pNext){if(!p->m_nAlpha)continue;auto v=p->m_vecPosition;density.offer(slot(p),type,{v.x,v.y,v.z},camera);}}
 density.finish();return true;
}
bool cpuParticleVisible(const CParticle* p){return !active||density.visible(slot(p));}
