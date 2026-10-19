#include "world_channels.hpp"
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cmath>
#include <random>
static unsigned scalar(float value){return unsigned(std::max(0.f,std::min(255.f,value+.5f)));}
static uint64_t checked=0;
static void check(const float* color,float inverse){unsigned out[4];soft::worldChannels(color,inverse,out);for(unsigned k=0;k<4;++k){assert(out[k]==scalar(color[k]*inverse));++checked;}}
static float bitsFloat(uint32_t b){float x;std::memcpy(&x,&b,4);return x;}
int main(){
 std::mt19937 random(62052);
 for(unsigned i=0;i<2000000;++i){float c[4];for(float& v:c)v=bitsFloat(random());float inverse=std::min(1e8f,bitsFloat(((1+random()%153)<<23)|(random()&0x7fffffu)));check(c,inverse);}
 for(float inverse:{std::numeric_limits<float>::min(),1e-20f,1e-6f,.001f,.1f,1.f,1.3f,100.f,1e8f})for(int n=-2;n<=257;++n){float c=float(n)-.5f;float a[4]={c,std::nextafter(c,-INFINITY),std::nextafter(c,INFINITY),float(n)};check(a,inverse);}
 const float special[]={-INFINITY,-1e30f,-255.f,-.5f,-0.f,0.f,std::numeric_limits<float>::denorm_min(),.5f,255.f,1e30f,INFINITY,NAN};
 for(float a:special)for(float b:special)for(float c:special){float value[4]={a,b,c,-a};for(float inverse:{std::numeric_limits<float>::min(),.1f,1.f,1e8f})check(value,inverse);}
 std::printf("PASS world channels: %llu exact scalar comparisons, full color binary32, legal shader inverse range, half-integer neighbours, saturation, signed zero, NaN/Inf; SIMD=%d\n",(unsigned long long)checked,soft::worldChannelsAvailable);
}
