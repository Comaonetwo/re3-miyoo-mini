#pragma once
#include "soft_renderer.hpp"
#include <cmath>
namespace soft {
// RGB565 certificate belongs to decoded raster and is rebuilt on unlock.
// No alpha inference; caller must separately require decodedOpaque and the
// additive blend modes. Source-over black is never skipped by this test.
inline void certifyInkBounds(Texture& t){
 t.inkX0=t.width;t.inkY0=t.height;t.inkX1=t.inkY1=-1;
 for(unsigned y=0;y<t.height;++y)for(unsigned x=0;x<t.width;++x)if(t.pixels[size_t(y)*t.width+x]){
  t.inkX0=std::min(t.inkX0,int(x));t.inkX1=std::max(t.inkX1,int(x));t.inkY0=std::min(t.inkY0,int(y));t.inkY1=std::max(t.inkY1,int(y));}
 t.decodedInkBounds=true;
}
inline bool outsideInk(float u,float v,const Texture& t,bool linear,bool wrap=false){
 if(!t.decodedInkBounds)return false;if(t.inkX1<0)return true;
 float x=u*t.width,y=v*t.height;if(linear){x-=.5f;y-=.5f;}
 if(!std::isfinite(x)||!std::isfinite(y))return false;
 const float margin=.0001f;
 // Clamp addresses may extend an edge texel indefinitely: only reject that
 // direction when the corresponding edge is itself outside the ink bounds.
 bool xValid=!wrap||(u>=0&&u<=1&&t.inkX0>0&&t.inkX1<int(t.width)-1);
 bool yValid=!wrap||(v>=0&&v<=1&&t.inkY0>0&&t.inkY1<int(t.height)-1);
 return (xValid&&((t.inkX0>0&&x+(linear?1:0)<t.inkX0-margin)||(t.inkX1<int(t.width)-1&&x>t.inkX1+1+margin)))||
        (yValid&&((t.inkY0>0&&y+(linear?1:0)<t.inkY0-margin)||(t.inkY1<int(t.height)-1&&y>t.inkY1+1+margin)));
}
}
