#include "texel_run.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
template<bool Wrap> void test(){std::mt19937 rng(2701);unsigned accepted=0;
 for(unsigned n=0;n<60000;++n){unsigned w=1u<<(rng()%10),h=1u<<(rng()%10);float z=.001f+float(rng()%200000)/1000,zx=n%3?float(int(rng()%2001)-1000)*z*.0001f:0;
  float u=float(int(rng()%200001)-100000)*z*.001f,v=float(int(rng()%200001)-100000)*z*.001f;
  float ux=float(int(rng()%201)-100)*z*.00001f,vx=float(int(rng()%201)-100)*z*.00001f;
  if(n%11==0)u=ux=0;if(n%13==0)v=vx=0;float inverse=zx==0?1/z:0;size_t index=0;uint32_t divisions=0;
  if(constantTexelRun<Wrap>(z,u,v,zx,ux,vx,w,h,zx==0,inverse,index,divisions)){++accepted;
   for(unsigned i=0;i<8;++i){assert(z>0);float iz=zx==0?inverse:1/z;int x=texelRunFloor(u*iz*w),y=texelRunFloor(v*iz*h);x=Wrap?x&int(w-1):std::max(0,std::min(int(w)-1,x));y=Wrap?y&int(h-1):std::max(0,std::min(int(h)-1,y));assert(index==size_t(y)*w+x);z+=zx;u+=ux;v+=vx;}
  }
 }
 assert(accepted>1000);printf("PASS %s texel certificates accepted=%u / 60000, original eight-step sampler identical\n",Wrap?"repeat":"clamp",accepted);
}
int main(){test<true>();test<false>();}
