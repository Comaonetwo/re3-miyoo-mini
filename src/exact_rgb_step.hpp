#pragma once
#include <cstdint>
#include <cstring>
#include "world_channels.hpp"
namespace soft {
// Inputs are binary32 multiples of 2^-113. Their sums remain on that lattice:
// every nonzero sum is normal, so ARMv7 NEON flush-to-zero cannot change it.
// Bounds also keep <=640 horizontal and <=480 vertical additions finite.
inline bool normalLatticeInput(float value){
 uint32_t bits;std::memcpy(&bits,&value,4);bits&=0x7fffffff;
 return bits==0||(bits>=0x12800000u&&bits<=0x6c800000u); //2^-90..2^90
}
inline bool rgbRegisterSafe(const float* row,const float* dx,const float* dy){
 for(unsigned i=0;i<3;++i)if(!normalLatticeInput(row[i])||!normalLatticeInput(dx[i])||!normalLatticeInput(dy[i]))return false;
 return true;
}
struct RgbRegisterSteps {
#if defined(__aarch64__) || defined(__ARM_NEON) || defined(__ARM_NEON__)
 float32x4_t value,delta;
 RgbRegisterSteps(const float* rgb,const float* step):value{rgb[0],rgb[1],rgb[2],0.f},delta{step[0],step[1],step[2],0.f}{}
 void advance(){value=vaddq_f32(value,delta);}
 void storeRgb(float* rgb)const{rgb[0]=vgetq_lane_f32(value,0);rgb[1]=vgetq_lane_f32(value,1);rgb[2]=vgetq_lane_f32(value,2);}
 void channels(float inverse,unsigned* out)const{worldChannelsVector(value,inverse,out);}
#else
 float value[4],delta[3];
 RgbRegisterSteps(const float* rgb,const float* step):value{rgb[0],rgb[1],rgb[2],0.f},delta{step[0],step[1],step[2]}{}
 void advance(){for(unsigned i=0;i<3;++i)value[i]+=delta[i];}
 void storeRgb(float* rgb)const{for(unsigned i=0;i<3;++i)rgb[i]=value[i];}
 void channels(float inverse,unsigned* out)const{worldChannels(value,inverse,out);}
#endif
};
}
