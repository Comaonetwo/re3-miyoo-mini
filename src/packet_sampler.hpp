#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
static constexpr bool packetSamplerAvailable=true;
#else
static constexpr bool packetSamplerAvailable=false;
#endif
#if defined(__GNUC__) || defined(__clang__)
#define SOFT_PACKET_INLINE inline __attribute__((always_inline))
#else
#define SOFT_PACKET_INLINE inline
#endif
SOFT_PACKET_INLINE void packetCoordinates(const float* z,const float* u,const float* v,unsigned w,unsigned h,float* x,float* y){
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 float safe[4];for(unsigned i=0;i<4;++i)safe[i]=z[i]>=1e-6f&&z[i]<=1e6f?z[i]:1.f;
 float32x4_t d=vld1q_f32(safe),r=vrecpeq_f32(d);
 for(unsigned i=0;i<3;++i)r=vmulq_f32(r,vrecpsq_f32(d,r));
 vst1q_f32(x,vmulq_n_f32(vmulq_f32(vld1q_f32(u),r),float(w)));
 vst1q_f32(y,vmulq_n_f32(vmulq_f32(vld1q_f32(v),r),float(h)));
#else
 for(unsigned i=0;i<4;++i){float r=1/z[i];x[i]=u[i]*r*w;y[i]=v[i]*r*h;}
#endif
}
template<bool Wrap> SOFT_PACKET_INLINE bool packetTexel(float coordinate,float numerator,unsigned size,int& sample){
 if(!std::isfinite(coordinate)||std::fabs(coordinate)>1e8f)return false;
 int cell=int(coordinate);if(float(cell)>coordinate)--cell;
 sample=Wrap?cell&int(size-1):std::max(0,std::min(int(size)-1,cell));
 if(size==1||numerator==0)return true;
 // Three NR corrections on normal positive inputs, followed by the original
 // multiply order. This guard exceeds the reciprocal/multiply rounding error;
 // points close to a nearest-texel boundary use the original scalar division.
 float error=2e-6f*(std::fabs(coordinate)+1)+1e-6f;
 if(!Wrap&&coordinate+error<1){sample=0;return true;}
 if(!Wrap&&coordinate-error>float(size-1)){sample=int(size)-1;return true;}
 float fraction=coordinate-float(cell);return fraction>error&&1-fraction>error;
}
template<bool Wrap> SOFT_PACKET_INLINE void packetSamples(const float* z,const float* u,const float* v,unsigned w,unsigned h,size_t* indices,bool* certified){
 float x[4],y[4];packetCoordinates(z,u,v,w,h,x,y);
 for(unsigned i=0;i<4;++i){int tx=0,ty=0;certified[i]=z[i]>=1e-6f&&z[i]<=1e6f&&packetTexel<Wrap>(x[i],u[i],w,tx)&&packetTexel<Wrap>(y[i],v[i],h,ty);indices[i]=size_t(ty)*w+tx;}
}
}
#undef SOFT_PACKET_INLINE
