#include "covered_run.hpp"
#include <cassert>
#include <random>
#include <cmath>
int main(){std::mt19937 rng(2601);unsigned accepted=0;
 for(unsigned n=0;n<30000;++n){float edge[3],step[3];int count=64+rng()%577;
  for(unsigned k=0;k<3;++k){int exp=int(rng()%70)-35;edge[k]=std::ldexp(float(int(rng()%40001)-20000),exp);step[k]=std::ldexp(float(int(rng()%20001)-10000),exp-8);if(n%4==0){edge[k]=std::fabs(edge[k])+1;step[k]=std::fabs(step[k]);}}
  uint8_t mask=0;if(soft::coveredEdgeRun(edge,step,0,count-1,&mask)){++accepted;uint8_t expected=7;
   for(unsigned k=0;k<3;++k){float value=edge[k];for(int i=0;i<count;++i){if(i>=3&&i<count-3)assert(value>0&&std::isfinite(value));if(i>=count-3&&value<=0)expected&=uint8_t(~(1u<<(i-count+3)));value+=step[k];}}
   assert(mask==expected);
  }
 }
 assert(accepted>7000);float e[3]={1,1,1},d[3]={0,0,0};assert(!soft::coveredEdgeRun(e,d,0,62));assert(!soft::coveredEdgeRun(e,d,0,640));
}
