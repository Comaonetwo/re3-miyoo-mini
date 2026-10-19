#include "dynamic_world.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
using namespace soft;
static void event(DynamicWorld& d,double ms,const char* reason){
 for(unsigned i=0;i<2000;++i){float before=d.distance();unsigned target=d.targetDistance();bool change=d.sample(ms,true);assert(d.distance()>=80&&d.distance()<=120);double step=(target>before?20:40)*std::max(ms,1000./30)/1000.;assert(std::fabs(d.distance()-before)<=step+.001);if(change){assert(!std::strcmp(d.reason(),reason));assert(d.decisionFps()>0);return;}}
 assert(false&&"missing controller event");
}
int main(){unsigned scenarios=0;
 for(unsigned base:{240u,320u}){DynamicWorld d;d.configure(base,true,100,true);
 event(d,70,"rain_down");assert(d.thinRain()&&d.level()==0&&d.targetDistance()==100);
 event(d,70,"distance_down");assert(d.targetDistance()==80);
 event(d,70,"resolution_down");assert(d.level()==1);event(d,70,"resolution_down");assert(d.level()==2);for(unsigned n=0;n<200;++n)d.sample(70,true);assert(d.distance()==80);
 event(d,30,"resolution_up");assert(d.level()==1);event(d,30,"resolution_up");assert(d.level()==0);
 event(d,30,"distance_up");assert(d.targetDistance()==100);event(d,30,"distance_up");assert(d.targetDistance()==120);event(d,30,"rain_up");assert(!d.thinRain());
 d.configure(base,false,180,true);assert(d.width()==base&&d.distance()==180&&!d.thinRain());for(unsigned n=0;n<300;++n)assert(!d.sample(70,true));++scenarios;}
 DynamicWorld d;d.configure(240,true,100,false);for(unsigned n=0;n<300;++n)assert(!d.sample(1000./19.5,true));assert(d.width()==240&&d.targetDistance()==100);
 for(unsigned n=0;n<300;++n){assert(!d.sample(100,false));assert(!d.sample(300,true));assert(!d.sample(std::numeric_limits<double>::quiet_NaN(),true));}assert(d.width()==240&&d.distance()==100);
 event(d,70,"distance_down");event(d,70,"resolution_down");auto width=d.width();d.resetSamples();for(unsigned n=0;n<5;++n)d.sample(70,true);assert(d.width()==width);d.configure(320,true,140,false);assert(d.width()==320&&d.distance()==120);
 d.configure(160,true,60,true);assert(d.width()==160&&d.distance()==80);event(d,70,"rain_down");for(unsigned n=0;n<200;++n)d.sample(70,true);assert(d.level()==0&&d.targetDistance()==80);d.configure(160,true,60,false);assert(!d.thinRain());++scenarios;
 DynamicWorld gates;gates.configure(240,true,80,true);for(unsigned n=0;n<8;++n)assert(!gates.sample(60,true));assert(gates.sample(60,true)&&gates.thinRain());gates.resetSamples();for(unsigned n=0;n<40;++n)assert(!gates.sample(30,true));assert(gates.targetDistance()==80);++scenarios;
 // A borderline scene recovers, becomes too expensive, then tries again.
 // The failed exact rung is held for eight eligible seconds, never permanently.
 for(unsigned baseWidth:{240u,320u}){
  DynamicWorld f;f.configure(baseWidth,true,80,false);
  event(f,70,"resolution_down");event(f,70,"resolution_down");
  event(f,40,"resolution_up");assert(f.level()==1);
  event(f,75,"resolution_down");assert(f.level()==2&&f.retryWait()==8);
  // A pause resets measurement/causal attribution but must not expire the hold.
  for(unsigned n=0;n<200;++n)assert(!f.sample(100,false));assert(f.retryWait()==8);
  for(unsigned n=0;n<150;++n){assert(!f.sample(40,true));assert(f.level()==2);}
  assert(f.retryWait()>1.9&&f.retryWait()<2.1);
  event(f,40,"resolution_up");assert(f.level()==1&&f.retryWait()==0);
  // A successful recovery is not blocked by the failed lower rung.
  event(f,40,"resolution_up");assert(f.level()==0);
  // Disabling/re-enabling AUTO starts a fresh scene and clears old hold state.
  f.configure(baseWidth,false,80,false);f.configure(baseWidth,true,80,false);
  event(f,70,"resolution_down");event(f,40,"resolution_up");assert(f.level()==0);
  ++scenarios;
 }
 unsigned base=0,thin=0;for(unsigned n=0;n<1000;++n){bool a=worldRainVisible(n,true,false),b=worldRainVisible(n,true,true);base+=a;thin+=b;assert(!b||a);assert(worldRainVisible(n,false,false));assert(b==worldRainVisible(n,true,true));}assert(base==350&&thin==200);
 std::printf("PASS %u coordinator scenarios: rain/distance/resolution order, reverse recovery,18/21FPS .5/1.5s, ramps/floors/OFF/manual/pause/stalls/weather; failed upgrade hold/pause/reset/recovery; stable rain200/350 (old REDUCED500) subset\n",scenarios);
}
