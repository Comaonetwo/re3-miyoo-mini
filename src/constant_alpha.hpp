#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
namespace soft {
// Bound the *existing* float row/column recurrences, not just vertex alpha.
// The alpha plane must round to the same byte throughout the entire rectangle.
inline bool certifyConstantAlpha(const float* row,const float* dx,const float* dy,unsigned width,unsigned height,unsigned alpha){
 if(!width||!height||alpha>255)return false;double x=width-1,y=height-1;
 double e=std::numeric_limits<float>::epsilon();double gx=x*e/(1-x*e),gy=y*e/(1-y*e);if(!std::isfinite(gx+gy)||gx>.001||gy>.001)return false;
 double za=row[0],aa=row[6],zx=dx[0],ax=dx[6],zy=dy[0],ay=dy[6];if(!std::isfinite(za+aa+zx+ax+zy+ay))return false;
 double zs=std::fabs(za)+y*std::fabs(zy),as=std::fabs(aa)+y*std::fabs(ay);
 double ze=gy*zs+gx*(zs+gy*zs+x*std::fabs(zx));double ae=gy*as+gx*(as+gy*as+x*std::fabs(ax));
 double minz=za+std::min(0.,zx*x)+std::min(0.,zy*y)-ze;if(!(minz>1e-8))return false;
 double c=aa-alpha*za,cx=ax-alpha*zx,cy=ay-alpha*zy;
 double difference=std::max({std::fabs(c),std::fabs(c+cx*x),std::fabs(c+cy*y),std::fabs(c+cx*x+cy*y)})+ae+alpha*ze;
 return difference/minz+4*e*256 < .45; // division/multiply and byte rounding margin
}
}
