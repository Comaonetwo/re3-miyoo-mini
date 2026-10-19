#include "world_fade_policy.hpp"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
 for(unsigned fps:{5u,10u,15u,20u,30u,60u}){
  float dt=50.f/fps;unsigned a=0,frames=0;
  while(a<255){unsigned next=soft::nextWorldAlpha(a,dt);assert(next>a&&next<=255&&next-a<=64);a=next;++frames;}
  assert(frames<=16);if(fps>=30)assert(frames==16);
  if(fps==15)assert(frames==8);if(fps==5)assert(frames==4);
 }
 for(float dt:{0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})assert(soft::worldFadeStep(dt)==16);
 for(unsigned a=0;a<=255;++a)for(unsigned fps=1;fps<=120;++fps){unsigned n=soft::nextWorldAlpha(a,50.f/fps);assert(n>=a&&n<=255);}
 puts("PASS bounded, monotone time-based building fade at 5/10/15/20/30/60 FPS, baseline and invalid timers");
}
