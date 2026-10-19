#include <initializer_list>
#include "vehicle_lod.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){for(float multiplier:{.5f,1.f,1.2f,2.f})for(bool fast:{false,true})for(bool eligible:{false,true})for(unsigned width:{80u,160u,240u,320u,640u}){
 float old=625*multiplier*multiplier,newDistance=soft::pedLodDistance(old,width,fast,eligible);
 assert(newDistance>0&&newDistance<=old);assert((old==newDistance)==(!fast||!eligible||width>=320));
 if(fast&&eligible&&width<320){float expected=(width>=240?20:16)*multiplier;assert(std::fabs(std::sqrt(newDistance)-expected)<1e-4);}
 // Complementary hi/low callbacks have no gap or overlap, including equality.
 for(float d:{0.f,newDistance*.999f,newDistance,newDistance*1.001f,old,old+1})assert((d<newDistance)!=(d>=newDistance));
}puts("PASS ped LOD: shared squared boundary, camera multiplier, missing LOD/standard/full resolution unchanged");}
