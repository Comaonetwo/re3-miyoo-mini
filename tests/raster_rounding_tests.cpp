#include "raster_span.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <random>
namespace {
void reference(float e0,float e1,float e2,float d0,float d1,float d2,int width,int& first,int& last){
 first=0;last=width-1;if(width<16)return;
 const float e[3]={e0,e1,e2},d[3]={d0,d1,d2};
 for(unsigned i=0;i<3;++i){
  if(d[i]==0){if(e[i]<0){first=width;last=-1;return;}continue;}
  if(std::fabs(d[i])<.0001f)continue;
  float crossing=std::max(-2.f,std::min(float(width+2),-e[i]/d[i]));
  if(d[i]>0)first=std::max(first,int(std::floor(crossing))-2);
  else last=std::min(last,int(std::ceil(crossing))+2);
 }
}
void check(float x){assert(soft::rasterFloor(x)==int(std::floor(x)));assert(soft::rasterCeil(x)==int(std::ceil(x)));}
}
int main(){
 // Around every integer in the validated screen-coordinate domain, including
 // offscreen negatives, ties, both zero signs and the nearest representables.
 for(int n=-1000001;n<=1000001;++n){float v=float(n);check(v);check(std::nextafter(v,-INFINITY));check(std::nextafter(v,INFINITY));}
 check(-0.f);check(std::numeric_limits<float>::denorm_min());check(-std::numeric_limits<float>::denorm_min());
 std::mt19937 random(59051);std::uniform_real_distribution<float> coord(-1000001.f,1000001.f),edge(-1e6f,1e6f),slope(-4096,4096);
 for(unsigned n=0;n<1000000;++n){check(coord(random));float e[3],d[3];for(unsigned k=0;k<3;++k){e[k]=edge(random);d[k]=slope(random);}if(n%3==0)d[n%3]=0;if(n%5==0)d[n%3]=.00001f;int w=1+int(random()%640),a,b,c,f;
  reference(e[0],e[1],e[2],d[0],d[1],d[2],w,a,b);soft::rasterSpan(e[0],e[1],e[2],d[0],d[1],d[2],w,c,f);assert(a==c&&b==f);
 }
 // Exact and adjacent crossing boundaries, empty spans, degenerate and
 // malformed edges retain the original clamping/fallback behaviour.
 const float special[]={-INFINITY,-640,-2,-1,-.0001f,-.00001f,-0.f,0.f,.00001f,.0001f,1,2,640,INFINITY,NAN};
 for(float e:special)for(float d:special)for(int w:{1,15,16,80,160,240,320,640}){int a,b,c,f;reference(e,e,-e,d,-d,0,w,a,b);soft::rasterSpan(e,e,-e,d,-d,0,w,c,f);assert(a==c&&b==f);}
 puts("PASS raster rounding: >7 million coordinate cases, million reference scanlines, boundary/nonfinite edge cases; exact integer bounds");
}
