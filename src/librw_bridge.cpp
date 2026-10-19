#include "librw_bridge.hpp"
#include <stdexcept>
namespace soft {
LibrwBridge* LibrwBridge::active_=nullptr;
LibrwBridge::LibrwBridge(Renderer& r,const Camera& c):renderer_(r),camera_(c),pipeline_(nullptr) {
    if(active_) throw std::runtime_error("only one LibrwBridge may be active");
    pipeline_=rw::ObjPipeline::create();
    if(!pipeline_) throw std::runtime_error("ObjPipeline allocation failed");
    pipeline_->impl.render=&draw; active_=this;
}
LibrwBridge::~LibrwBridge() { active_=nullptr; pipeline_->destroy(); }
void LibrwBridge::bind(rw::Atomic* a) { a->pipeline=pipeline_; }
void LibrwBridge::draw(rw::ObjPipeline*,rw::Atomic* a) {
    auto& self=*active_; auto* g=a->geometry;
    if(!g||g->flags&rw::Geometry::NATIVE||g->numMorphTargets!=1||!g->morphTargets[0].vertices||!g->triangles) {
        ++self.unsupported_;return;
    }
    // No textures, skinning, lights or alpha yet. Keep their omission visible.
    if(g->flags&rw::Geometry::LIGHT) ++self.unsupported_;
    for(int i=0;i<g->numTriangles;++i) {
        auto& tri=g->triangles[i]; bool valid=true;
        for(int j=0;j<3;++j) if(tri.v[j]>=g->numVertices) valid=false;
        if(!valid||tri.matId>=g->matList.numMaterials) { ++self.unsupported_;continue; }
        auto* mat=g->matList.materials[tri.matId];
        if(!mat) { ++self.unsupported_;continue; }
        if(mat->texture||mat->color.alpha!=255) ++self.unsupported_;
        unsigned red=mat->color.red,green=mat->color.green,blue=mat->color.blue;
        if((g->flags&rw::Geometry::PRELIT)&&g->colors) {
            unsigned r=0,gg=0,b=0;
            for(int j=0;j<3;++j) { auto& c=g->colors[tri.v[j]];r+=c.red;gg+=c.green;b+=c.blue; }
            if(g->flags&rw::Geometry::MODULATE) {red=red*r/765;green=green*gg/765;blue=blue*b/765;}
            else {red=r/3;green=gg/3;blue=b/3;}
        }
        rw::V3d points[3],world[3];
        for(int j=0;j<3;++j) points[j]=g->morphTargets[0].vertices[tri.v[j]];
        if(a->getFrame()) rw::V3d::transformPoints(world,points,3,a->getFrame()->getLTM());
        else for(int j=0;j<3;++j) world[j]=points[j];
        Vec3 v[3];for(int j=0;j<3;++j) v[j]=view({world[j].x,world[j].y,world[j].z},self.camera_);
        self.renderer_.triangle(v,rgb(red,green,blue),self.camera_);
    }
}
}
