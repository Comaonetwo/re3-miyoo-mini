#include "block_reciprocal.hpp"
#include "vehicle_lod.hpp"
#include <cassert>
#include <cstdio>
#include <random>
int main(){std::mt19937 rng(3902);uint64_t reused=0;for(unsigned n=0;n<100000;++n){float z=std::pow(10.f,float(int(rng()%800)-500)/100);float dz=z*float(int(rng()%200)-100)/10000;soft::BlockReciprocal r;for(unsigned k=0;k<16;++k){float exact=1/z,approx=r.get(z);assert(std::isfinite(approx));assert(std::fabs((approx-exact)/exact)<2e-6f);z+=dz;}reused+=r.reused;}assert(reused>100000);
 for(bool fast:{false,true})for(bool owns:{false,true})for(unsigned width:{80u,160u,240u,320u,640u}){float old=4900,result=soft::vehicleLodDistance(old,width,fast,owns);assert(result>0&&result<=old);assert((result==old)==(!fast||!owns||width>=320));if(width==240&&fast&&owns)assert(std::fabs(std::sqrt(result)-55)<.001f);}
 puts("PASS 1.6 million reciprocal candidates, error<2ppm; common vehicle hi/low boundary, missing LOD/full resolution/standard fallback");}
