#pragma once
#include <cstdint>
#include <algorithm>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
inline uint16_t skyDeltaPixel(uint16_t p,uint32_t d){
 auto clamp=[](int v,int hi){return unsigned(std::max(0,std::min(hi,v)));};
 return uint16_t((clamp(int(p>>11)+int(d&63)-31,31)<<11)|(clamp(int((p>>5)&63)+int((d>>6)&127)-63,63)<<5)|clamp(int(p&31)+int((d>>13)&63)-31,31));
}
inline void skyDeltaRow(uint16_t* pixels,const uint32_t* delta,unsigned count){unsigned x=0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 const uint16x8_t m31=vdupq_n_u16(31),m63=vdupq_n_u16(63),m127=vdupq_n_u16(127);
 for(;x+7<count;x+=8){uint16x8_t p=vld1q_u16(pixels+x);uint32x4_t a=vld1q_u32(delta+x),b=vld1q_u32(delta+x+4);
  uint16x8_t r=vandq_u16(vcombine_u16(vmovn_u32(a),vmovn_u32(b)),m63),g=vandq_u16(vcombine_u16(vshrn_n_u32(a,6),vshrn_n_u32(b,6)),m127),blue=vandq_u16(vcombine_u16(vshrn_n_u32(a,13),vshrn_n_u32(b,13)),m63);
  r=vminq_u16(vqsubq_u16(vaddq_u16(vshrq_n_u16(p,11),r),m31),m31);
  g=vminq_u16(vqsubq_u16(vaddq_u16(vandq_u16(vshrq_n_u16(p,5),m63),g),m63),m63);
  blue=vminq_u16(vqsubq_u16(vaddq_u16(vandq_u16(p,m31),blue),m31),m31);
  vst1q_u16(pixels+x,vorrq_u16(vorrq_u16(vshlq_n_u16(r,11),vshlq_n_u16(g,5)),blue));
 }
#endif
 for(;x<count;++x)pixels[x]=skyDeltaPixel(pixels[x],delta[x]);
}
}
