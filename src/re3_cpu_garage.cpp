#include "re3_cpu_garage.hpp"
#include "common.h"
#include "Garages.h"
#include "Timer.h"
#include "World.h"
#include "PlayerPed.h"
#include "Streaming.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
// Restore references only. Never override mission locks, timers or saved cars.
bool refreshMissingDoors(CGarage& garage){
 if(!garage.IsUsed()||garage.m_eGarageType==GARAGE_CRUSHER||garage.m_pDoor1||garage.m_pDoor2)return false;
 garage.RefreshDoorPointers(true);
 if(!garage.m_pDoor1&&!garage.m_pDoor2)return false;
 garage.UpdateDoorsHeight();return true;
}
struct Observation {uint32 lastRetry=0,lastStatus=0;unsigned signature=~0u;};
Observation observed[NUM_GARAGES];
}
void cpuGarageMonitor(){
 auto player=FindPlayerPed();if(!player)return;
 const CVector position=player->GetPosition();const uint32 now=CTimer::GetTimeInMilliseconds();
 for(unsigned i=0;i<NUM_GARAGES;++i){auto& garage=CGarages::aGarages[i];auto& info=observed[i];
  if(!garage.IsUsed()||garage.m_eGarageType==GARAGE_CRUSHER||Abs(position.x-garage.GetGarageCenterX())>45||Abs(position.y-garage.GetGarageCenterY())>45){info.signature=~0u;continue;}
  if((int32)(now-info.lastRetry)<0)info.lastRetry=now;
  if(!garage.m_pDoor1&&!garage.m_pDoor2&&now-info.lastRetry>=1000){
   info.lastRetry=now;bool restored=refreshMissingDoors(garage);
   std::printf("GARAGE retry id=%u type=%u restored=%d state=%u position=%.3f\n",i,unsigned(garage.m_eGarageType),restored,unsigned(garage.m_eGarageState),garage.m_fDoorPos);
  }
  unsigned doors=(garage.m_pDoor1?1:0)|(garage.m_pDoor2?2:0);
  unsigned signature=unsigned(garage.m_eGarageState)|(unsigned(garage.m_bDeactivated)<<8)|(doors<<9);
  // Read-only explanation of the native hideout gate; never force it open.
  bool hideout=garage.m_eGarageType>=GARAGE_HIDEOUT_ONE&&garage.m_eGarageType<=GARAGE_HIDEOUT_THREE;
  if(hideout&&garage.m_eGarageState==GS_FULLYCLOSED&&((int32)(now-info.lastStatus)<0||now-info.lastStatus>=2000)){
   info.lastStatus=now;bool inCar=FindPlayerVehicle()!=nullptr;float squared=garage.CalcDistToGarageRectangleSquared(position.x,position.y);
   int stored=CGarages::CountCarsInHideoutGarage(garage.m_eGarageType),capacity=CGarages::FindMaxNumStoredCarsForGarage(garage.m_eGarageType);
   const char* reason=garage.m_bDeactivated?"DISABLED":squared>=(inCar?100.f:1.7f*1.7f)?"OUT_OF_RANGE":inCar&&stored>=capacity?"FULL":stored?"RESTORE_PENDING":"CHECK_TRANSITION";
   std::printf("GARAGE gate id=%u reason=%s range=%.2f in_car=%d stored=%d capacity=%d requests=%d stream_bytes=%zu loaded_vehicles=%d\n",i,reason,std::sqrt(squared),inCar,stored,capacity,CStreaming::ms_numModelsRequested,CStreaming::ms_memoryUsed,CStreaming::ms_numVehiclesLoaded);
  }
  if(signature!=info.signature){info.signature=signature;
   std::printf("GARAGE nearby id=%u type=%u state=%u disabled=%d doors=%u door_pos=%.3f player=%.1f,%.1f\n",i,unsigned(garage.m_eGarageType),unsigned(garage.m_eGarageState),garage.m_bDeactivated,doors,garage.m_fDoorPos,position.x,position.y);
  }
 }
}
void cpuGarageTest(){
 unsigned tested=0;
 for(auto& garage:CGarages::aGarages){
  if(!garage.IsUsed()||garage.m_eGarageType==GARAGE_CRUSHER||(!garage.m_pDoor1&&!garage.m_pDoor2))continue;
  const unsigned state=garage.m_eGarageState;const bool disabled=garage.m_bDeactivated;
  const float position=garage.m_fDoorPos;const uint32 action=garage.m_nTimeToStartAction;auto target=garage.m_pTarget;
  garage.m_pDoor1=garage.m_pDoor2=nullptr;garage.m_bRecreateDoorOnNextRefresh=false;
  garage.RefreshDoorPointers(false);
  if(garage.m_pDoor1||garage.m_pDoor2)throw std::runtime_error("garage missing-door baseline unexpectedly recovered");
  if(!refreshMissingDoors(garage))throw std::runtime_error("garage door recovery failed");
  if(state!=garage.m_eGarageState||disabled!=garage.m_bDeactivated||position!=garage.m_fDoorPos||action!=garage.m_nTimeToStartAction||target!=garage.m_pTarget)throw std::runtime_error("garage recovery changed gameplay state");
  if(refreshMissingDoors(garage))throw std::runtime_error("garage unnecessarily rescanned valid references");
  ++tested;
 }
 if(!tested)throw std::runtime_error("garage recovery test needs existing garage doors");
 std::printf("PASS GARAGE recovery: %u real garages, missing reference reproduced, state/locks/timer/target preserved\n",tested);
}

#include "Automobile.h"
#include "ModelInfo.h"
#include <vector>
#include <thread>
#include <chrono>
extern int32 desiredNumVehiclesLoaded;
namespace {struct PendingCar {bool active=false;uint32 time=0;};PendingCar pendingCars[MODELINFOSIZE];}
void cpuGarageRequestModel(int id){if(id>=0&&id<MODELINFOSIZE){if(!cpuGarageModelNeeded(id))std::printf("GARAGE MODEL request id=%d loaded=%d state=%u\n",id,CStreaming::HasModelLoaded(id),unsigned(CStreaming::ms_aInfoForModel[id].m_loadState));pendingCars[id].active=true;pendingCars[id].time=CTimer::GetTimeInMilliseconds();}}
void cpuGarageModelRestored(int id){if(id>=0&&id<MODELINFOSIZE){pendingCars[id].active=false;std::printf("GARAGE MODEL restored id=%d\n",id);}}
bool cpuGarageModelNeeded(int id){if(id<0||id>=MODELINFOSIZE||!pendingCars[id].active)return false;uint32 now=CTimer::GetTimeInMilliseconds();int32 age=int32(now-pendingCars[id].time);return age>=0&&age<=1500;}
void cpuGarageResetPending(){for(auto& pending:pendingCars)pending.active=false;}
static void garagePump(){
 for(unsigned i=0;i<4000;++i){CStreaming::LoadRequestedModels();if(!CStreaming::ms_numModelsRequested&&CStreaming::ms_channel[0].state==CHANNELSTATE_IDLE&&CStreaming::ms_channel[1].state==CHANNELSTATE_IDLE)return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 throw std::runtime_error("garage asynchronous streaming test timed out");
}
void cpuGarageStreamingTest(){
 int target=-1;for(int id=0;id<MODELINFOSIZE;++id){auto mi=CModelInfo::GetModelInfo(id);if(mi&&mi->GetModelType()==MITYPE_VEHICLE&&static_cast<CVehicleModelInfo*>(mi)->m_vehicleType==VEHICLE_TYPE_CAR&&!CStreaming::HasModelLoaded(id)){target=id;break;}}
 if(target<0)throw std::runtime_error("garage streaming test has no unloaded vehicle");
 CStreaming::RequestModel(target,STREAMFLAGS_DONT_REMOVE|STREAMFLAGS_PRIORITY);CStreaming::LoadAllRequestedModels(false);
 if(!CStreaming::HasModelLoaded(target))throw std::runtime_error("garage initial test car load failed");
 auto original=new CAutomobile(target,RANDOM_VEHICLE);original->SetPosition(10,20,3);original->m_currentColour1=17;original->m_currentColour2=28;
 CStoredCar stored;stored.Init();stored.StoreCar(original);delete original;
 CStreaming::SetModelIsDeletable(target);CStreaming::RemoveModel(target);
 std::vector<std::pair<int,uint8>> flags;
 for(int id=0;id<MODELINFOSIZE;++id){auto mi=CModelInfo::GetModelInfo(id);if(mi&&mi->GetModelType()==MITYPE_VEHICLE&&CStreaming::HasModelLoaded(id)){flags.push_back({id,CStreaming::ms_aInfoForModel[id].m_flags});CStreaming::ms_aInfoForModel[id].m_flags|=STREAMFLAGS_DONT_REMOVE;}}
 int limit=desiredNumVehiclesLoaded;desiredNumVehiclesLoaded=0;cpuGarageResetPending();
 CStreaming::RequestModel(target,STREAMFLAGS_DEPENDENCY);garagePump();
 if(CStreaming::HasModelLoaded(target))throw std::runtime_error("legacy vehicle-cap starvation not reproduced");
 if(stored.RestoreCar())throw std::runtime_error("unloaded stored car unexpectedly restored");
 garagePump();
 if(!CStreaming::HasModelLoaded(target))throw std::runtime_error("reserved garage model still rejected");
 int challenger=-1;for(int id=0;id<MODELINFOSIZE;++id){auto mi=CModelInfo::GetModelInfo(id);if(id!=target&&mi&&mi->GetModelType()==MITYPE_VEHICLE&&static_cast<CVehicleModelInfo*>(mi)->m_vehicleType==VEHICLE_TYPE_CAR&&!CStreaming::HasModelLoaded(id)){challenger=id;break;}}
 if(challenger<0)throw std::runtime_error("garage competing reservation test needs a second model");
 CStreaming::RequestModel(challenger,STREAMFLAGS_DONT_REMOVE|STREAMFLAGS_PRIORITY);garagePump();
 if(!CStreaming::HasModelLoaded(target))throw std::runtime_error("competing reserved model evicted pending garage model");
 auto restored=stored.RestoreCar();
 if(!restored||restored->m_currentColour1!=17||restored->m_currentColour2!=28||restored->GetPosition().x!=10||restored->GetPosition().y!=20||restored->GetPosition().z!=3||restored->GetModelIndex()!=target||cpuGarageModelNeeded(target))throw std::runtime_error("stored car recovery/state/release failed");
 delete restored;desiredNumVehiclesLoaded=limit;for(auto f:flags)CStreaming::ms_aInfoForModel[f.first].m_flags=f.second;
 CStreaming::RemoveModel(challenger);CStreaming::RemoveModel(target);uint32 clock=CTimer::GetTimeInMilliseconds();cpuGarageRequestModel(target);if(!cpuGarageModelNeeded(target))throw std::runtime_error("garage reservation absent");CTimer::SetTimeInMilliseconds(clock+1501);if(cpuGarageModelNeeded(target))throw std::runtime_error("abandoned garage reservation did not expire");CTimer::SetTimeInMilliseconds(clock-1);if(cpuGarageModelNeeded(target))throw std::runtime_error("garage reservation survived backwards clock");CTimer::SetTimeInMilliseconds(clock);cpuGarageResetPending();if(cpuGarageModelNeeded(target))throw std::runtime_error("pending car registry reset failed");
 std::printf("PASS GARAGE streaming: legacy soft-cap rejection reproduced, model=%d restored with colors/position intact, competing reserved load protected, reservation released/expired/reset\n",target);
}
