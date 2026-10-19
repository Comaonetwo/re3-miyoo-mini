#pragma once
#include <algorithm>
#include <cmath>
namespace soft {
// re3 adds 16 once per visibility call. Keep its 30 FPS baseline and cap a
// low-FPS increment to four times that amount; no single-frame opaque pop.
inline unsigned worldFadeStep(float timeStep){
 if(!std::isfinite(timeStep)||timeStep<=0)return 16;
 return unsigned(std::max(16.f,std::min(64.f,std::floor(timeStep*9.6f+.5f))));
}
inline unsigned nextWorldAlpha(unsigned alpha,float timeStep){
 return std::min(255u,alpha+worldFadeStep(timeStep));
}
}
