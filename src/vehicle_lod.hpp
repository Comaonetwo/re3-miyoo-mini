#pragma once
namespace soft {
// Keep original low-detail geometry and common hi/low boundary. Unsupported
// clumps, boats/planes/trains and >=320px worlds retain original thresholds.
inline float vehicleLodDistance(float originalSquared,unsigned width,bool fast,bool hasLod){
 if(!fast||!hasLod||width>=320)return originalSquared;
 float ratio=(width>=240?55.f:45.f)/70.f;return originalSquared*(ratio*ratio);
}
// Only complete original rigid ped LODs qualify; skinned models, player and
// cutscene models retain their original callbacks/detail.
inline float pedLodDistance(float originalSquared,unsigned width,bool fast,bool hasLod){
 if(!fast||!hasLod||width>=320)return originalSquared;
 float ratio=(width>=240?20.f:16.f)/25.f;return originalSquared*(ratio*ratio);
}
}
