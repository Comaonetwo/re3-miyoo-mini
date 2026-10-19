#pragma once
#include <cmath>
#include <algorithm>
namespace soft {
// Nested, evenly spread subsets of the old REDUCED baseline: MEDIUM70%, LOW40%.
// Caller preserves the skipped streak's native RNG consumption.
inline bool worldRainVisible(unsigned index,bool reduced,bool extra){if(!reduced)return true;if(index&1u)return false;unsigned slot=(index/2)%10;return extra?(slot==0||slot==3||slot==6||slot==9):(slot!=2&&slot!=5&&slot!=8);}
class DynamicWorld {
 unsigned base_=0,level_=0,manual_=100,target_=100;bool enabled_=false,raining_=false,thin_=false;
 double recoveryWait_[2]={0,0},recoveryAge_=4;unsigned recoveryLevel_=3;
 double low_=0,high_=0,average_=0,settle_=0,decisionFps_=0;float distance_=100;const char* reason_="initial";
 void changed(const char* reason){decisionFps_=fps();reason_=reason;resetSamples();settle_=.6;}
public:
 unsigned level()const{return level_;}
 double retryWait()const{return level_?recoveryWait_[level_-1]:0;}
 unsigned width()const{const unsigned medium[]={240,200,160},high[]={320,280,240};return base_==240?medium[level_]:base_==320?high[level_]:base_;}
 float distance()const{return distance_;}unsigned targetDistance()const{return target_;}
 bool thinRain()const{return thin_;}const char* reason()const{return reason_;}
 double decisionFps()const{return decisionFps_;}
 double fps()const{return average_>0?1000/average_:0;}
 unsigned configure(unsigned base,bool enabled,unsigned manual=100,bool raining=false){
  if(base!=base_||enabled!=enabled_||manual!=manual_){base_=base;enabled_=enabled;manual_=manual;level_=0;thin_=false;recoveryWait_[0]=recoveryWait_[1]=0;recoveryAge_=4;recoveryLevel_=3;target_=enabled?std::max(80u,std::min(120u,manual)):manual;distance_=float(target_);resetSamples();reason_="configure";}
  if(raining!=raining_){raining_=raining;recoveryLevel_=3;if(!raining)thin_=false;resetSamples();}
  return width();
 }
 void resetSamples(){low_=high_=average_=settle_=0;}
 bool sample(double ms,bool eligible){
  if(!enabled_||!eligible||!std::isfinite(ms)||ms<=0||ms>250){resetSamples();recoveryLevel_=3;return false;}
  const double dt=std::max(ms,1000./30)/1000.;
  // Count eligible gameplay time only; pauses do not expire a failed-step hold.
  for(auto& wait:recoveryWait_)wait=std::max(0.,wait-dt);
  recoveryAge_=std::min(4.,recoveryAge_+dt);
  // Reach the selected target gradually; further decisions wait for arrival.
  float delta=float(target_)-distance_;if(std::fabs(delta)>.001f){float step=float(dt)*(delta>0?20.f:40.f);distance_+=std::max(-step,std::min(step,delta));low_=high_=average_=0;settle_=.6;return false;}
  if(settle_>0){settle_-=dt;return false;}
  average_=average_?average_+(ms-average_)*(dt/(.5+dt)):ms;
  low_=average_>1000./18?low_+dt:0;high_=average_<1000./21?high_+dt:0;
  if(low_>=.5){
   if(raining_&&!thin_){thin_=true;recoveryLevel_=3;changed("rain_down");return true;}
   if(target_>80){target_=std::max(80u,target_-20);recoveryLevel_=3;changed("distance_down");return true;}
   if((base_==240||base_==320)&&level_<2){// If this exact upgrade failed quickly, avoid probing it every few seconds.
    // Lower quality still responds immediately, and other levels are independent.
    if(recoveryLevel_==level_&&recoveryAge_<=3)recoveryWait_[level_]=8;
    recoveryLevel_=3;++level_;changed("resolution_down");return true;}
  }
  if(high_>=1.5){
   if(level_>0){if(recoveryWait_[level_-1]>0)return false;
    --level_;recoveryLevel_=level_;recoveryAge_=0;changed("resolution_up");return true;}
   if(target_<120){target_=std::min(120u,target_+20);changed("distance_up");return true;}
   if(thin_){thin_=false;changed("rain_up");return true;}
  }
  return false;
 }
};
}
