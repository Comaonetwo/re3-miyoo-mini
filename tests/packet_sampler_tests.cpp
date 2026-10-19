#include "packet_sampler.hpp"
#include <cassert>
#include <random>
#include <cstring>
#include <cstdio>
using namespace soft;
static int scalar(float u,float z,unsigned size,bool wrap){float f=u*(1/z)*size;int n=int(f);if(float(n)>f)--n;return wrap?n&int(size-1):std::max(0,std::min(int(size)-1,n));}
int main(){unsigned accepted=0,fallback=0;std::mt19937 rng(2702);
 for(unsigned n=0;n<150000;++n){float z[4],u[4],v[4];unsigned w=1u<<(rng()%11),h=1u<<(rng()%11);for(unsigned k=0;k<4;++k){int exp=int(rng()%39)-19;z[k]=std::ldexp(1+float(rng()%65536)/65536,exp);u[k]=float(int(rng()%200001)-100000)*z[k]*.001f;v[k]=float(int(rng()%200001)-100000)*z[k]*.001f;if(n%7==0)u[k]=float(int(rng()%100)-50)*z[k]/w;if(n%11==0)v[k]=float(int(rng()%100)-50)*z[k]/h;if(n%13==0)u[k]=0;}
  for(bool wrap:{false,true}){size_t indices[4];bool fast[4];if(wrap)packetSamples<true>(z,u,v,w,h,indices,fast);else packetSamples<false>(z,u,v,w,h,indices,fast);for(unsigned k=0;k<4;++k){if(fast[k]){++accepted;assert(indices[k]==size_t(scalar(v[k],z[k],h,wrap))*w+scalar(u[k],z[k],w,wrap));}else ++fallback;}}
 }
 assert(accepted>100000&&fallback>10000);
 // Every binary32 mantissa in one normal binade. Power-of-two scaling spans
 // the supported Z exponents without changing normalized reciprocal error.
 float worst=0;for(unsigned i=0;i<(1u<<23);i+=4){float z[4],one[4]={1,1,1,1},x[4],y[4];for(unsigned k=0;k<4;++k){uint32_t bits=0x3f800000u|uint32_t(i+k);std::memcpy(z+k,&bits,4);}packetCoordinates(z,one,one,1,1,x,y);for(unsigned k=0;k<4;++k){float exact=1/z[k],error=std::fabs(x[k]-exact)/exact;worst=std::max(worst,error);assert(error<4e-7f);}}
 printf("PASS packet sampler NEON=%d certified=%u fallback=%u; 8388608 mantissas worst reciprocal relative error=%.9g\n",packetSamplerAvailable,accepted,fallback,worst);
}
