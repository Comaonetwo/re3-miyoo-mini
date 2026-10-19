#include "audio_fade.hpp"
#include <cassert>
#include <limits>
int main(){assert(soft::audioFadeStep(127,.4421f)==127);assert(soft::audioFadeStep(60,3)==20);assert(soft::audioFadeStep(0,10)==1);assert(soft::audioFadeStep(100,0)==127);assert(soft::audioFadeStep(100,std::numeric_limits<float>::quiet_NaN())==127);for(unsigned volume=0;volume<=127;++volume)for(unsigned i=1;i<800;++i){float frames=i*.013f,step=volume/frames;int expected=step>127?127:step<1?1:int(step);assert(soft::audioFadeStep(float(volume),frames)==expected);}}
