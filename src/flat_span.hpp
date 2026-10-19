#pragma once
#include "flat_blend.hpp"
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
// Exact source-over on RGB565: expand destination bits before blending,
// divide by255 before quantizing. n<=65152, so n+1+(n>>8) fits uint16.
inline void flatSourceOver(uint16_t* pixels,unsigned count,unsigned red,unsigned green,unsigned blue,unsigned alpha,const FlatBlend& table){
 if(!alpha)return;
 if(alpha==255){std::fill(pixels,pixels+count,uint16_t((red>>3)<<11|(green>>2)<<5|(blue>>3)));return;}
 unsigned x=0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 const uint16x8_t mask=vdupq_n_u16(63),one=vdupq_n_u16(1),rr=vdupq_n_u16(red*alpha+127),gg=vdupq_n_u16(green*alpha+127),bb=vdupq_n_u16(blue*alpha+127);unsigned inverse=255-alpha;
 for(;x+7<count;x+=8){uint16x8_t p=vld1q_u16(pixels+x),r=vshrq_n_u16(p,11),g=vandq_u16(vshrq_n_u16(p,5),mask),b=vandq_u16(p,vdupq_n_u16(31));
  r=vorrq_u16(vshlq_n_u16(r,3),vshrq_n_u16(r,2));g=vorrq_u16(vshlq_n_u16(g,2),vshrq_n_u16(g,4));b=vorrq_u16(vshlq_n_u16(b,3),vshrq_n_u16(b,2));
  r=vaddq_u16(vmulq_n_u16(r,inverse),rr);g=vaddq_u16(vmulq_n_u16(g,inverse),gg);b=vaddq_u16(vmulq_n_u16(b,inverse),bb);
  r=vshrq_n_u16(vaddq_u16(vaddq_u16(r,one),vshrq_n_u16(r,8)),8);g=vshrq_n_u16(vaddq_u16(vaddq_u16(g,one),vshrq_n_u16(g,8)),8);b=vshrq_n_u16(vaddq_u16(vaddq_u16(b,one),vshrq_n_u16(b,8)),8);
  vst1q_u16(pixels+x,vorrq_u16(vorrq_u16(vshlq_n_u16(vshrq_n_u16(r,3),11),vshlq_n_u16(vshrq_n_u16(g,2),5)),vshrq_n_u16(b,3)));
 }
#endif
 for(;x<count;++x)pixels[x]=table(pixels[x]);
}
}
