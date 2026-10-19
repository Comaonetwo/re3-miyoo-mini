#pragma once
#include <algorithm>
#include <cmath>
namespace soft {
// Call only with finite, bounded coordinates representable as int. Conversion
// truncates toward zero; one correction gives floor/ceil exactly, including
// negative values and integral boundaries. Cortex-A7 otherwise calls libm for
// every edge on every scanline. Keep float/double precision of the caller.
template<class Float> inline int rasterFloor(Float value){int n=int(value);return n-int(Float(n)>value);}
template<class Float> inline int rasterCeil(Float value){int n=int(value);return n+int(Float(n)<value);}

// Conservative scanline interval. Keep two border pixels on either side;
// the original top-left edge test still decides actual coverage. Tiny slopes
// do not constrain the interval, avoiding unstable division near horizontal edges.
inline void rasterSpan(float e0,float e1,float e2,float d0,float d1,float d2,int width,int& first,int& last){
 first=0;last=width-1;if(width<16)return;
 const float e[3]={e0,e1,e2},d[3]={d0,d1,d2};
 for(unsigned i=0;i<3;++i){
  if(d[i]==0){if(e[i]<0){first=width;last=-1;return;}continue;}
  if(std::fabs(d[i])<.0001f)continue;
  float crossing=std::max(-2.f,std::min(float(width+2),-e[i]/d[i]));
  if(d[i]>0)first=std::max(first,rasterFloor(crossing)-2);
  else last=std::min(last,rasterCeil(crossing)+2);
 }
}
}
