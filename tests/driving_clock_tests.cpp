#include "driving_clock.hpp"
#include "driving.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
int main(){try{
    CollisionWorld world({{{{-200,0,-200},{200,0,-200},{200,0,200}},0},{{{-200,0,-200},{200,0,200},{-200,0,200}},0}});
    Driving slow(world,{0,2,0},0),fast=slow;DrivingClock a,b;
    for(int i=0;i<150;++i)a.advance(1./15,true,[&](float dt){slow.update(world,dt,1,.1f,false);});
    for(int i=0;i<600;++i)b.advance(1./60,true,[&](float dt){fast.update(world,dt,1,.1f,false);});
    check(std::fabs(slow.position.x-fast.position.x)<1e-5&&std::fabs(slow.position.z-fast.position.z)<1e-5&&slow.speed==fast.speed,"15 and 60 FPS follow identical physics");
    int ticks=0;DrivingClock clock;auto step=[&](float){++ticks;};clock.advance(.01,true,step);clock.advance(2,false,step);clock.advance(.01,true,step);check(ticks==0,"pause discards fractional time");
    clock.advance(10,true,step);check(ticks==15,"long stall catch-up is bounded");clock.reset();clock.advance(-1,true,step);clock.advance(NAN,true,step);check(ticks==15,"invalid time ignored");
    puts("fixed physics: 15/60 FPS equivalence, pause, stall bounds PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
