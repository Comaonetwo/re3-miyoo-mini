#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <limits>
namespace soft {
inline uint32_t floatBits(float value){uint32_t bits;std::memcpy(&bits,&value,sizeof(bits));return bits;}
inline float bitsFloat(uint32_t bits){float value;std::memcpy(&value,&bits,sizeof(value));return value;}
// Advance the original float recurrence, not start + count*step. Inside one
// normal exponent binade, the rounded mantissa displacement is constant once
// tie-to-even parity settles. Two sampled additions verify that displacement.
// Stay two ULPs from either boundary; crossing/zero/subnormal/nonfinite values
// use actual additions. This preserves texel/depth boundaries bit for bit.
inline float exactFloatAdvance(float value,float step,unsigned count,uint64_t* saved=nullptr){
 static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE binary32 required");
 while(count){
  if(count<16){do{value+=step;}while(--count);break;}
  uint32_t original=floatBits(value),bucket=original&0xff800000u;
  float next=value+step;uint32_t first=floatBits(next);
  if(first==original){if(saved)*saved+=count-1;return next;}
  if((bucket&0x7f800000u)!=0&&(bucket&0x7f800000u)!=0x7f800000u&&(first&0xff800000u)==bucket){
   float secondValue=next+step;uint32_t second=floatBits(secondValue);
   int mantissa=int(original&0x7fffffu),delta=int(first&0x7fffffu)-mantissa;
   if((second&0xff800000u)==bucket&&delta==int(second&0x7fffffu)-int(first&0x7fffffu)){
    unsigned allowed=0;
    if(delta>0&&mantissa<=0x7ffffd)allowed=unsigned(0x7ffffd-mantissa)/unsigned(delta);
    if(delta<0&&mantissa>=2)allowed=unsigned(mantissa-2)/unsigned(-delta);
    unsigned jump=std::min(count,allowed);
    if(jump>=16){value=bitsFloat(bucket|uint32_t(mantissa+delta*int(jump)));count-=jump;if(saved)*saved+=jump-2;continue;}
   }
  }
  value=next;--count;
 }
 return value;
}
}
