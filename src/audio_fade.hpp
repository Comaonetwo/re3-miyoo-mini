#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace soft {
// Fractional remaining frames can yield a fade step above int8 range. Clamp
// before conversion; the existing volume/step guard then terminates the voice.
inline int8_t audioFadeStep(float volume,float frames){
 if(!(frames>0)||!std::isfinite(frames)||!std::isfinite(volume))return 127;
 float step=volume/frames;
 return int8_t(std::max(1.f,std::min(127.f,step)));
}
}
