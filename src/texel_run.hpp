#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace soft {
inline int texelRunFloor(float f){int n=int(f);if(float(n)>f)--n;return n;}
// Certify one original nearest texel for eight float-recurring candidates.
// Projective ratios of exact linear attributes are monotone for positive Z.
// The expanded margin bounds recurrence error, endpoint perturbations and
// rounded reciprocal/multiplies. Ambiguous texel boundaries use the old path.
template<bool Wrap> inline bool texelRunAxis(float first,float last,float initial,float delta,float zsum,float invLower,unsigned size,bool fixed,int& sample){
 if(!std::isfinite(first)||!std::isfinite(last)||std::fabs(first)>1e8f||std::fabs(last)>1e8f)return false;
 int cell=texelRunFloor(first);sample=Wrap?cell&int(size-1):std::max(0,std::min(int(size)-1,cell));
 if(fixed)return true;
 float error=16e-6f*size*(std::fabs(initial)+7*std::fabs(delta))*invLower*(1+zsum*invLower)+1e-6f;
 if(!std::isfinite(error))return false;
 float low=std::min(first,last)-error,high=std::max(first,last)+error;
 if(!Wrap&&high<1){sample=0;return true;}
 if(!Wrap&&low>float(size-1)){sample=int(size)-1;return true;}
 return low>float(cell)&&high<float(cell+1);
}
template<bool Wrap> inline bool constantTexelRun(float z,float u,float v,float zx,float ux,float vx,unsigned width,unsigned height,bool affine,float rowInverse,size_t& index,uint32_t& divisions){
 float ze=z,ue=u,ve=v;for(unsigned i=0;i<7;++i){ze+=zx;ue+=ux;ve+=vx;}
 if(!(z>1e-8f&&ze>1e-8f))return false;
 float a=affine?rowInverse:1/z,b=affine?rowInverse:1/ze;if(!affine)divisions+=2;
 float fu=u*a*width,eu=ue*b*width,fv=v*a*height,ev=ve*b*height;
 // Fast rejection before the error-bound arithmetic in the common case.
 if(!std::isfinite(fu)||!std::isfinite(eu)||!std::isfinite(fv)||!std::isfinite(ev)||std::fabs(fu)>1e8f||std::fabs(eu)>1e8f||std::fabs(fv)>1e8f||std::fabs(ev)>1e8f)return false;
 if(Wrap&&(texelRunFloor(fu)!=texelRunFloor(eu)||texelRunFloor(fv)!=texelRunFloor(ev)))return false;
 float zsum=std::fabs(z)+7*std::fabs(zx),lower=std::min(z,ze)-16e-6f*zsum;if(!(lower>1e-8f))return false;
 float invLower=affine?rowInverse:1/lower;if(!affine)++divisions;int x,y;
 if(!texelRunAxis<Wrap>(fu,eu,u,ux,zsum,invLower,width,(u==0&&ux==0)||(affine&&ux==0),x)||!texelRunAxis<Wrap>(fv,ev,v,vx,zsum,invLower,height,(v==0&&vx==0)||(affine&&vx==0),y))return false;
 index=size_t(y)*width+x;return true;
}
}
