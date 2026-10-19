#pragma once
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>
namespace soft {
// Address policy is immutable for a triangle. Fold period/power-of-two
// detection out of the pixel loop; compute one wrap for two neighbours.
struct FilterAddress {
 int size,period,mask; bool clamp,mirror;
 FilterAddress(int n=1,unsigned address=3):size(n),period(address==2?2*n:n),mask((period&(period-1))?-1:period-1),clamp(address==3||address==4),mirror(address==2){}
 inline int sample(float uv) const {
  if(clamp){float f=std::max(-1.f,std::min(2.f,uv))*size;int n=int(f);if(float(n)>f)--n;return std::max(0,std::min(size-1,n));}
  if(mask>=0){float f=uv*size;int n=int(f);if(float(n)>f)--n;n&=mask;return mirror&&n>=size?period-1-n:n;}
  float p=mirror?2.f:1.f;float wrapped=std::fmod(uv,p);if(wrapped<0)wrapped+=p;
  int n=std::min(period-1,int(wrapped*size));return mirror&&n>=size?period-1-n:n;
 }
 inline void pair(int n,int& a,int& b) const {
  if(clamp){if(n<0){a=b=0;}else if(n>=size-1){a=b=size-1;}else{a=n;b=n+1;}return;}
  int t;if(mask>=0)t=n&mask;else{t=n%period;if(t<0)t+=period;}
  int next=t+1;if(next==period)next=0;
  a=mirror&&t>=size?period-1-t:t;
  b=mirror&&next>=size?period-1-next:next;
 }
};
// memcpy permits unaligned access and avoids aliasing violations. Both bytes
// are proven in-bounds by the adjacent-neighbour test at the call site.
inline void alphaPair(const uint8_t* p,unsigned& a,unsigned& b){
 uint16_t word;std::memcpy(&word,p,sizeof word);
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
 a=word>>8;b=word&255;
#else
 a=word&255;b=word>>8;
#endif
}
}
