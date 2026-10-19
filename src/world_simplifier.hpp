#pragma once
namespace rw {struct Geometry;}
#include <cstddef>
#include "model_clusters.hpp"
#include <vector>
namespace soft {
struct SimplifiedWorld{rw::Geometry* geometry=nullptr;size_t bytes=0;float error=0;unsigned input=0,output=0,reason=1;std::vector<ModelBounds> changedBounds;};
SimplifiedWorld simplifyWorldGeometry(rw::Geometry* source,size_t budget,const Vec3* protectedCamera=nullptr,float protectedRadius=0,bool intermediate=false);
}
