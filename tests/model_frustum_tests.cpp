#include "model_clusters.hpp"
#include <cassert>
#include <random>
#include <cstdio>
using namespace soft;
int main(){std::mt19937 random(2033);unsigned rejected=0;
 for(unsigned n=0;n<20000;++n){float m[12];for(auto& v:m)v=float(int(random()%2001)-1000)/137.f;m[3]*=20;m[7]*=20;m[11]*=20;
 Camera c;c.nearZ=.1f;c.farZ=20+random()%150;c.focal=80+random()%250;float vertical=.5f+random()%100/100.f;
 ModelBounds b;Vec3 p={float(int(random()%400)-200),float(int(random()%400)-200),float(int(random()%400)-200)};b.include(p);b.include({p.x+random()%40,p.y+random()%40,p.z+random()%40});ModelFrustum frustum(m,vertical,c);
 unsigned mask=frustum.activePlanes(b,frustum.boundMargin(b));unsigned all=63,any=0;
 for(unsigned k=0;k<8;++k){float x=k&1?b.maximum.x:b.minimum.x,y=k&2?b.maximum.y:b.minimum.y,z=k&4?b.maximum.z:b.minimum.z;Vec3 p={-(x*m[0]+y*m[1]+z*m[2]+m[3]),(x*m[4]+y*m[5]+z*m[6]+m[7])*vertical,x*m[8]+y*m[9]+z*m[10]+m[11]};unsigned code=frustumCode(p,c);all&=code;any|=code;}
 if(mask==0)assert(any==0);if(mask&128)assert(all!=0);
 ModelBounds leaf;Vec3 center={(b.minimum.x+b.maximum.x)*.5f,(b.minimum.y+b.maximum.y)*.5f,(b.minimum.z+b.maximum.z)*.5f};leaf.include(center);if(!(mask&128)&&frustum.outsideMasked(leaf,mask,frustum.boundMargin(b))){Vec3 q={-(center.x*m[0]+center.y*m[1]+center.z*m[2]+m[3]),(center.x*m[4]+center.y*m[5]+center.z*m[6]+m[7])*vertical,center.x*m[8]+center.y*m[9]+center.z*m[10]+m[11]};assert(frustumCode(q,c));}
 if(!frustum.outside(b))continue;++rejected;unsigned common=63;
 for(unsigned k=0;k<8;++k){float x=k&1?b.maximum.x:b.minimum.x,y=k&2?b.maximum.y:b.minimum.y,z=k&4?b.maximum.z:b.minimum.z;
  Vec3 camera={-(x*m[0]+y*m[1]+z*m[2]+m[3]),(x*m[4]+y*m[5]+z*m[6]+m[7])*vertical,x*m[8]+y*m[9]+z*m[10]+m[11]};common&=frustumCode(camera,c);}
 assert(common!=0);
 }
 assert(rejected>10000);ModelBounds b;b.include({0,0,.1f});float identity[12]={-1,0,0,0,0,1,0,0,0,0,1,0};Camera c;ModelFrustum f(identity,1,c);assert(!f.outside(b));b.valid=false;assert(!f.outside(b));puts("PASS 20000 random affine bounds against independent float corner classification; near-plane contact retained");
}
