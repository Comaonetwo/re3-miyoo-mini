#pragma once
#include <algorithm>
#include <cmath>
namespace soft {
// 60 Hz physics independent of display FPS. Ignore >250 ms stalls and clear
// the fractional remainder while paused/free-camera so resume cannot jump.
class DrivingClock {
    double remainder_=0;
public:
    void reset(){remainder_=0;}
    template<class Step> void advance(double elapsed,bool active,Step step){
        if(!active||!std::isfinite(elapsed)||elapsed<0){reset();return;}
        constexpr double tick=1./60;
        remainder_+=std::min(elapsed,.25);
        while(remainder_+1e-9>=tick){step(float(tick));remainder_=std::max(0.,remainder_-tick);}
    }
};
}
