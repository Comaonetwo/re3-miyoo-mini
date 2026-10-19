#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#include <arm_neon.h>
#endif
namespace soft {
// Bound the exact row's repeated float additions, including up to 640 steps.
// Threshold rounds upward before float-only depth probes/scanning. Only skip
// a row when every candidate fails the original strict reciprocal-Z test.
inline bool hiddenDepthRow(const float* depth,int first,int last,float row,float step){
 if(last-first+1<32)return false;
 int middle=(first+last)/2;
 // A cheap failed probe only declines the optimization. It never culls.
 if(depth[middle]<=row+step*float(middle))return false;
 double a=double(row)+double(step)*first,b=double(row)+double(step)*last;
 double upper=std::max(a,b)+.0001*(std::fabs(double(row))+std::fabs(double(step))*last)+1e-7;
 float threshold=float(upper);if(!std::isfinite(threshold)||threshold<=0)return false;
 uint32_t bits;std::memcpy(&bits,&threshold,4);++bits;std::memcpy(&threshold,&bits,4);
 if(!(depth[first]>=threshold&&depth[last]>=threshold&&depth[(first+last)/2]>=threshold))return false;
 int x=first;
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
 float32x4_t bound=vdupq_n_f32(threshold);
 for(;x+3<=last;x+=4){uint32x4_t pass=vcgeq_f32(vld1q_f32(depth+x),bound);uint32x2_t both=vand_u32(vget_low_u32(pass),vget_high_u32(pass));if((vget_lane_u32(both,0)&vget_lane_u32(both,1))!=UINT32_MAX)return false;}
#endif
 for(;x<=last;++x)if(!(depth[x]>=threshold))return false;
 return true;
}
}
