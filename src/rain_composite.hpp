#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
// Exact maximum of the common 2x2 world footprint. Positive IEEE bits
// retain subnormals, unlike an ARMv7 NEON floating maximum; canonicalize -0.
inline float rainDepth2x2(const float* top,const float* bottom){
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 const uint32x2_t positive=vdup_n_u32(0x7fffffffu);uint32x2_t a=vand_u32(vreinterpret_u32_f32(vld1_f32(top)),positive),b=vand_u32(vreinterpret_u32_f32(vld1_f32(bottom)),positive);uint32x2_t value=vmax_u32(a,b);value=vpmax_u32(value,value);uint32_t bits=vget_lane_u32(value,0);float result;std::memcpy(&result,&bits,4);return result;
#else
 float result=0;for(unsigned i=0;i<2;++i){result=std::max(result,top[i]);result=std::max(result,bottom[i]);}return result;
#endif
}
// Four 2x2 footprints at once: contiguous world rows, no scalar round trips.
inline void rainDepthRow2x2(const float* top,const float* bottom,float* output,unsigned count){unsigned x=0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 const uint32x4_t positive=vdupq_n_u32(0x7fffffffu);
 for(;x+3<count;x+=4){const unsigned src=x*2;uint32x4_t a=vmaxq_u32(vandq_u32(vreinterpretq_u32_f32(vld1q_f32(top+src)),positive),vandq_u32(vreinterpretq_u32_f32(vld1q_f32(bottom+src)),positive)),b=vmaxq_u32(vandq_u32(vreinterpretq_u32_f32(vld1q_f32(top+src+4)),positive),vandq_u32(vreinterpretq_u32_f32(vld1q_f32(bottom+src+4)),positive));
  uint32x2_t lo=vpmax_u32(vget_low_u32(a),vget_high_u32(a)),hi=vpmax_u32(vget_low_u32(b),vget_high_u32(b));vst1q_f32(output+x,vreinterpretq_f32_u32(vcombine_u32(lo,hi)));
 }
#endif
 for(;x<count;++x)output[x]=rainDepth2x2(top+x*2,bottom+x*2);
}
// World reciprocal depth and rain limits are nonnegative, non-NaN (+inf is
// the empty limit). Their unsigned IEEE bits retain exact strict ordering,
// including positive subnormals, without ARMv7 floating-point flush-to-zero.
inline unsigned rainCompositeRow(uint16_t* image,const float* depth,const uint16_t* red,const uint16_t* green,const uint16_t* blue,const float* minimum,unsigned count){unsigned x=0,written=0;
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 const uint32x4_t positive=vdupq_n_u32(0x7fffffffu);
 const uint16x8_t m31=vdupq_n_u16(31),m63=vdupq_n_u16(63),m255=vdupq_n_u16(255),zero=vdupq_n_u16(0);
 for(;x+7<count;x+=8){uint16x8_t rr=vld1q_u16(red+x),gg=vld1q_u16(green+x),bb=vld1q_u16(blue+x);
  uint32x4_t a=vcltq_u32(vandq_u32(vreinterpretq_u32_f32(vld1q_f32(depth+x)),positive),vandq_u32(vreinterpretq_u32_f32(vld1q_f32(minimum+x)),positive)),b=vcltq_u32(vandq_u32(vreinterpretq_u32_f32(vld1q_f32(depth+x+4)),positive),vandq_u32(vreinterpretq_u32_f32(vld1q_f32(minimum+x+4)),positive));
  uint16x8_t mask=vandq_u16(vcombine_u16(vmovn_u32(a),vmovn_u32(b)),vmvnq_u16(vceqq_u16(vorrq_u16(vorrq_u16(rr,gg),bb),zero)));
  uint16x8_t p=vld1q_u16(image+x),r=vshrq_n_u16(p,11),g=vandq_u16(vshrq_n_u16(p,5),m63),bl=vandq_u16(p,m31);
  r=vminq_u16(vaddq_u16(vorrq_u16(vshlq_n_u16(r,3),vshrq_n_u16(r,2)),rr),m255);g=vminq_u16(vaddq_u16(vorrq_u16(vshlq_n_u16(g,2),vshrq_n_u16(g,4)),gg),m255);bl=vminq_u16(vaddq_u16(vorrq_u16(vshlq_n_u16(bl,3),vshrq_n_u16(bl,2)),bb),m255);
  uint16x8_t result=vorrq_u16(vorrq_u16(vshlq_n_u16(vshrq_n_u16(r,3),11),vshlq_n_u16(vshrq_n_u16(g,2),5)),vshrq_n_u16(bl,3));vst1q_u16(image+x,vbslq_u16(mask,result,p));
  uint16x8_t ones=vshrq_n_u16(mask,15);uint16x4_t sum=vadd_u16(vget_low_u16(ones),vget_high_u16(ones));sum=vpadd_u16(sum,sum);sum=vpadd_u16(sum,sum);written+=vget_lane_u16(sum,0);
 }
#endif
 for(;x<count;++x)if((red[x]|green[x]|blue[x])&&depth[x]<minimum[x]){unsigned p=image[x],r=p>>11,g=(p>>5)&63,b=p&31;r=std::min(255u,((r<<3)|(r>>2))+red[x]);g=std::min(255u,((g<<2)|(g>>4))+green[x]);b=std::min(255u,((b<<3)|(b>>2))+blue[x]);image[x]=uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));++written;}
 return written;
}
}
