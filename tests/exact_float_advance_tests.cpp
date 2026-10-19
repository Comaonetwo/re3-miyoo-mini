#include "exact_float_advance.hpp"
#include <random>
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace soft;
static void compare(float start,float step,unsigned count){volatile float reference=start;for(unsigned i=0;i<count;++i)reference=reference+step;float actual=exactFloatAdvance(start,step,count);assert(floatBits(actual)==floatBits(reference));}
int main(){std::mt19937 random(210713);
 for(unsigned n=0;n<200000;++n){uint32_t a=random(),b=random();a=(a&0x807fffffu)|((1+random()%250)<<23);b=(b&0x807fffffu)|((1+random()%250)<<23);compare(bitsFloat(a),bitsFloat(b),random()%641);}
 for(unsigned exponent=1;exponent<254;++exponent)for(unsigned sign:{0u,0x80000000u})for(unsigned mantissa:{0u,1u,2u,3u,31u,100u,0x3fffffu,0x7ffffbu,0x7ffffcu,0x7ffffdu,0x7ffffeu,0x7fffffu}){
  float value=bitsFloat(sign|(exponent<<23)|mantissa);
  for(float delta:{-.125f,-.25f,-.5f,-.75f,-1.f,-1.4f,-1.5f,-2.5f,-17.5f,.125f,.25f,.5f,.75f,1.f,1.4f,1.5f,2.5f,17.5f})compare(value,std::ldexp(delta,int(exponent)-150),257);
 }
 for(float v:{0.f,-0.f,std::numeric_limits<float>::denorm_min(),-std::numeric_limits<float>::denorm_min()})for(float step:{0.f,-0.f,std::numeric_limits<float>::denorm_min(),-std::numeric_limits<float>::denorm_min(),1.f,-1.f})compare(v,step,640);
 uint64_t saved=0;float a=exactFloatAdvance(32.f,.003f,300,&saved);volatile float ref=32;for(unsigned i=0;i<300;++i)ref=ref+.003f;assert(floatBits(a)==floatBits(ref));assert(saved>250);
 puts("PASS 200000 arbitrary recurrences, exponent/sign/boundary/tie/cancellation/subnormal/signed-zero cases, identical binary32 bits");
}
