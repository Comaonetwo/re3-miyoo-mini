#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
// Signed-bit min preserves numeric ordering for nonnegative IEEE values.
// Positive NaNs sort above +inf and are ignored, as by the reference min
// starting at +inf. ANY negative bit pattern (including -0/negative NaN)
// makes the reduction negative: replay the original scalar row order then.
// Partial edge tiles also retain the reference. No approximate depth bound.
inline float depthTileMinimum(const float* depth,unsigned stride,unsigned width,unsigned height){
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 if(width==8&&height==8){int32x4_t lower=vdupq_n_s32(0x7f800000);
  for(unsigned y=0;y<8;++y){int32x4_t a=vreinterpretq_s32_f32(vld1q_f32(depth+y*stride)),b=vreinterpretq_s32_f32(vld1q_f32(depth+y*stride+4));lower=vminq_s32(lower,vminq_s32(a,b));}
  int32x2_t lo=vpmin_s32(vget_low_s32(lower),vget_high_s32(lower));lo=vpmin_s32(lo,lo);int32_t bits=vget_lane_s32(lo,0);
  if(bits>=0){float result;std::memcpy(&result,&bits,4);return result;}
 }
#endif
 float minimum=std::numeric_limits<float>::infinity();for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)minimum=std::min(minimum,depth[y*stride+x]);return minimum;
}
}
