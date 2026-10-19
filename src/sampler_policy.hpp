#pragma once
#include <algorithm>
#include <cmath>
namespace soft {
inline bool selectPacketSampler(const double* scalar,const double* packet){
 double a[3],b[3];for(unsigned i=0;i<3;++i){if(!(scalar[i]>0&&packet[i]>0&&std::isfinite(scalar[i])&&std::isfinite(packet[i]))||packet[i]>=scalar[i]*.99)return false;a[i]=scalar[i];b[i]=packet[i];}
 std::sort(a,a+3);std::sort(b,b+3);return b[1]<a[1]*.97;
}
}
