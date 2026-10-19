#include "vector_filter.hpp"
#include "filter_address.hpp"
#include <random>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cmath>
int main(){
 for(int size:{1,2,3,7,16,63,256,511,1024})for(unsigned address=1;address<=4;++address){
  soft::FilterAddress axis(size,address);
  auto reference=[&](int n){if(address==3||address==4)return std::max(0,std::min(size-1,n));int period=address==2?2*size:size;n%=period;if(n<0)n+=period;return address==2&&n>=size?period-1-n:n;};
  for(int n=-32769;n<=32769;++n){int a,b;axis.pair(n,a,b);assert(a==reference(n)&&b==reference(n+1));}
 }

 auto original=[](float uv,int size,unsigned address){
  if(address==3||address==4){float f=std::max(-1.f,std::min(2.f,uv))*size;int n=int(f);if(float(n)>f)--n;return std::max(0,std::min(size-1,n));}
  if(!(size&(size-1))){float f=uv*size;int n=int(f);if(float(n)>f)--n;if(address==2){n&=2*size-1;return n<size?n:2*size-1-n;}return n&(size-1);}
  float period=address==2?2.f:1.f;float wrapped=std::fmod(uv,period);if(wrapped<0)wrapped+=period;int n=std::min(int(period*size)-1,int(wrapped*size));return address==2&&n>=size?2*size-1-n:n;
 };
 std::mt19937 addresses(3301);std::uniform_real_distribution<float> uv(-100000.f,100000.f);
 for(int size:{1,2,3,7,16,63,256,511,1024})for(unsigned address=1;address<=4;++address){soft::FilterAddress axis(size,address);
  for(unsigned n=0;n<10000;++n){float f=n%2?uv(addresses):float(int(addresses()%10001)-5000)/size;assert(axis.sample(f)==original(f,size,address));}
 }
 printf("PASS 2359404 neighbour pairs, 360000 exact nearest coordinates, 258 unaligned alpha pairs\n");
 uint8_t bytes[259];for(unsigned i=0;i<259;++i)bytes[i]=uint8_t(i*73);
 for(unsigned i=0;i<258;++i){unsigned a,b;soft::alphaPair(bytes+i,a,b);assert(a==bytes[i]&&b==bytes[i+1]);}
std::mt19937 rng(3201);unsigned count=0;
 for(unsigned n=0;n<300000;++n){unsigned p[4];for(auto& v:p)v=rng()&65535;unsigned x=n%3==0?0:n%3==1?255:rng()%256,y=rng()%256;
  unsigned r[4],g[4],b[4];for(unsigned i=0;i<4;++i){unsigned rr=p[i]>>11,gg=(p[i]>>5)&63,bb=p[i]&31;r[i]=(rr<<3)|(rr>>2);g[i]=(gg<<2)|(gg>>4);b[i]=(bb<<3)|(bb>>2);}
  unsigned red=soft::bilinearChannel(r[0],r[1],r[2],r[3],x,y),green=soft::bilinearChannel(g[0],g[1],g[2],g[3],x,y),blue=soft::bilinearChannel(b[0],b[1],b[2],b[3],x,y);
  assert(soft::vectorBilinear565(p,x,y)==uint16_t((red>>3)<<11|(green>>2)<<5|(blue>>3)));++count;
 }
 for(unsigned n=0;n<=65152;++n)assert((n+1+(n>>8))/256==n/255);
 for(unsigned n=0;n<300000;++n){unsigned r=rng()%256,g=rng()%256,b=rng()%256,a=n%3==0?0:n%3==1?255:rng()%256,dr=rng()%256,dg=rng()%256,db=rng()%256;
 unsigned rr=(r*a+dr*(255-a)+127)/255,gg=(g*a+dg*(255-a)+127)/255,bb=(b*a+db*(255-a)+127)/255;
 assert(soft::vectorSourceOver565(r,g,b,a,dr,dg,db)==uint16_t((rr>>3)<<11|(gg>>2)<<5|(bb>>3)));
 }
 for(unsigned n=0;n<300000;++n){float x=float(int32_t(rng()%200000001)-100000000)/13.f;assert(soft::exactFilterFloor(x)==int(std::floor(x)));}
 for(float x:{-1.01f,-1.f,-.999f,-.01f,-0.f,0.f,.999f,1.f,1.01f,100000000.f,-100000000.f})assert(soft::exactFilterFloor(x)==int(std::floor(x)));
 printf("PASS %u exact SIMD/scalar bilinear RGB565 comparisons, endpoint/random weights; 300000 blend comparisons; every divide-255 input\n",count);
}
