#pragma once
#include <cmath>
#include <cstring>
#include <initializer_list>
namespace soft {
// Explicit decoration families only. A small bound alone must never hide roads.
inline bool decorName(const char* name){if(!name)return false;
 for(const char* stem:{"bin","wastebin","trash","bollard","parkingmeter","postbox","phonesign"}){unsigned n=std::strlen(stem);if(!std::strncmp(name,stem,n)&&(name[n]==0||(name[n]>='0'&&name[n]<='9')))return true;}return false;}
inline bool distantDecor(float radius,float distance,float nativeDistance){return std::isfinite(radius)&&std::isfinite(distance)&&std::isfinite(nativeDistance)&&radius>0&&radius<=6&&distance>std::fmax(24.f,std::fmin(55.f,nativeDistance*.7f))+radius;}
// Quantize in model space; prelight, ambient, material and alpha remain live.
inline unsigned normalAxis(float x,float y,float z){if(!std::isfinite(x+y+z)||std::fmax(std::fabs(x),std::fmax(std::fabs(y),std::fabs(z)))<1e-8f)return 6;unsigned a=std::fabs(y)>std::fabs(x)?1:0;if(std::fabs(z)>(a==0?std::fabs(x):std::fabs(y)))a=2;float v=a==0?x:a==1?y:z;return a*2+(v<0);}
}
