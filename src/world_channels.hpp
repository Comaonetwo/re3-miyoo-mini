#pragma once
#include <algorithm>
#include <cassert>
#include <cmath>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
constexpr bool worldChannelsAvailable=true;
#else
constexpr bool worldChannelsAvailable=false;
#endif

// Shader contract: inverse is positive, normal, finite and <=1e8. This is
// 1/z for z>1e-8 (otherwise1) in the validated projected-world pipeline.
// ARMv7 NEON flushes denormals: a subnormal color/product in this range
// cannot affect the integer result after +0.5 and clamp (it rounds to0).
// Same separate multiply, +0.5, min(255,value), max(0,value), and truncation
// as four scalar worldChannel calls. Compare/select preserves std::min's
// NaN -> 255 behaviour; the second operand is then finite. No FMA contraction.
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
inline void worldChannelsVector(float32x4_t input,float inverse,unsigned* out){
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
 assert(std::isnormal(inverse)&&inverse>0&&inverse<=1e8f);
 float32x4_t value=vmulq_f32(input,vdupq_n_f32(inverse));
 value=vaddq_f32(value,vdupq_n_f32(.5f));const float32x4_t upper=vdupq_n_f32(255.f);
 value=vbslq_f32(vcltq_f32(value,upper),value,upper);
 value=vmaxq_f32(value,vdupq_n_f32(0.f));vst1q_u32(out,vcvtq_u32_f32(value));
}
#endif
inline void worldChannels(const float* color,float inverse,unsigned* out){
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
 assert(std::isnormal(inverse)&&inverse>0&&inverse<=1e8f);
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 worldChannelsVector(vld1q_f32(color),inverse,out);
#else
 for(unsigned i=0;i<4;++i){float value=color[i]*inverse;value+=.5f;out[i]=unsigned(std::max(0.f,std::min(255.f,value)));}
#endif
}
}
