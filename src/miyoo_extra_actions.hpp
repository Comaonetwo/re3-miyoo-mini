#pragma once
#include <cstdint>
namespace soft {
// A must be freshly pressed: adding SELECT while accelerating must not
// cycle the camera, including drive-by/turret combinations.
struct MiyooViewLatch {
 uint32_t previous=0;bool active=false;
 bool update(uint32_t b,bool gameplay){
  const bool exclusive=(b&16383u)==(2048u|16u);
  if(!gameplay||!exclusive)active=false;
  if(gameplay&&exclusive&&!(previous&16u))active=true;
  previous=b;return active;
 }
};
struct MiyooExtraActions {bool horn,mission,manualAim,lookBehind,view,turret;};
inline MiyooExtraActions miyooExtraActions(uint32_t b,bool car,bool gameplay){
 return {gameplay&&car&&bool(b&8192u),gameplay&&car&&bool(b&4096u),gameplay&&!car&&bool(b&4096u),gameplay&&!car&&bool(b&8192u),false,gameplay&&car&&bool((b&2048u)&&(b&15u))};
}
}
