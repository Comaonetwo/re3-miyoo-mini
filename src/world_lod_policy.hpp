#pragma once
#include "model_clusters.hpp"
#include <vector>
namespace soft {
inline float boundsDistanceSquared(const ModelBounds& b,Vec3 c){
 if(!b.valid||!std::isfinite(c.x+c.y+c.z)||!std::isfinite(b.minimum.x+b.minimum.y+b.minimum.z+b.maximum.x+b.maximum.y+b.maximum.z)||b.minimum.x>b.maximum.x||b.minimum.y>b.maximum.y||b.minimum.z>b.maximum.z)return -1;
 float x=std::max({b.minimum.x-c.x,0.f,c.x-b.maximum.x}),y=std::max({b.minimum.y-c.y,0.f,c.y-b.maximum.y}),z=std::max({b.minimum.z-c.z,0.f,c.z-b.maximum.z});return x*x+y*y+z*z;
}
inline bool safeChangedGeometry(const std::vector<ModelBounds>& bounds,Vec3 c,float distance){if(bounds.empty()||!std::isfinite(distance)||distance<0)return false;for(const auto& b:bounds){float d=boundsDistanceSquared(b,c);if(d<0||d<distance*distance)return false;}return true;}
}
