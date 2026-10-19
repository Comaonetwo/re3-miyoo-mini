#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
namespace soft {
// World units from changed geometry, independent of the model origin. A coarse
// bounding region can reject extra candidates but cannot weaken the near guard.
inline float worldIntermediateDistance(bool active,float screenDistance){
 if(!std::isfinite(screenDistance)||screenDistance<0)return std::numeric_limits<float>::infinity();
 return std::max(25.f,screenDistance)*(active?1.f:1.2f);
}
}
