#include "soft_renderer.hpp"
#include <stdexcept>
#include <cmath>
#include <cstdio>
using namespace soft;
static unsigned seed=12345;static unsigned randomValue(){seed=seed*1664525u+1013904223u;return seed;}
int main(){try{Renderer r;for(int n=0;n<180;++n){unsigned scale=(n%3==0)?1:(n%3==1)?2:4;r.renderScale(scale);r.clear(0);ScreenVertex v[3];for(auto& p:v){p={float(int(randomValue()%1600)-160)/4,float(int(randomValue()%1280)-160)/4,2,0,0,255,255,255,255,0};}
 ScreenState state;r.screenTriangle(v,nullptr,state);
 auto edge=[](ScreenVertex a,ScreenVertex b,double x,double y){return (double(b.x)-a.x)*(y-a.y)-(double(b.y)-a.y)*(x-a.x);};double area=edge(v[0],v[1],v[2].x,v[2].y);if(std::fabs(area)<1e-5)continue;if(area<0)std::swap(v[1],v[2]);
 bool top[3];for(int i=0;i<3;++i){auto a=v[(i+1)%3],b=v[(i+2)%3];top[i]=b.y<a.y||(b.y==a.y&&b.x>a.x);}
 auto image=r.pixels();for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x){double sx=(x/scale+.5)*scale,sy=(y/scale+.5)*scale;bool inside=true;for(int i=0;i<3;++i){double e=edge(v[(i+1)%3],v[(i+2)%3],sx,sy);inside&=e>0||(e==0&&top[i]);}if((image[y*320+x]!=0)!=inside)throw std::runtime_error("scanline differs from independent double edge reference");}
 }puts("PASS randomized clipped coverage, both windings, shared-edge rule, 320/160/80");return 0;}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
