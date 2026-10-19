#include "librw_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
// Builds real librw Geometry/Material/Atomic objects, passes them through
// Atomic::defaultRenderCB -> ObjPipeline, and compares with the core renderer.
int main(int argc,char** argv) {
    if(!rw::Engine::init()) return 1;
    rw::registerMeshPlugin();rw::registerNativeDataPlugin();
    if(!rw::Engine::open(nullptr)||!rw::Engine::start()) return 1;
    int result=0;
    {
        soft::Renderer direct,via;soft::Camera camera; auto scene=soft::makeScene();
        soft::renderScene(direct,scene,camera);via.clear(soft::rgb(114,157,187));
        soft::LibrwBridge bridge(via,camera);
        auto* geometry=rw::Geometry::create(int(scene.size()*3),int(scene.size()),rw::Geometry::POSITIONS);
        for(size_t i=0;i<scene.size();++i) {
            auto* m=rw::Material::create();uint16_t c=scene[i].color;
            unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;
            m->color={rw::uint8((r<<3)|(r>>2)),rw::uint8((g<<2)|(g>>4)),rw::uint8((b<<3)|(b>>2)),255};
            geometry->triangles[i].matId=rw::uint16(geometry->matList.appendMaterial(m));m->destroy();
            for(int j=0;j<3;++j) {
                auto v=scene[i].v[j];geometry->morphTargets[0].vertices[i*3+j]={v.x,v.y,v.z};
                geometry->triangles[i].v[j]=rw::uint16(i*3+j);
            }
        }
        geometry->calculateBoundingSphere();
        auto* frame=rw::Frame::create();auto* atomic=rw::Atomic::create();
        atomic->setFrame(frame);atomic->setGeometry(geometry,0);geometry->destroy();bridge.bind(atomic);
        atomic->render();
        if(direct.pixels()!=via.pixels()||bridge.unsupported()!=0) result=1;
        // Verify world transform, not just identity geometry.
        rw::V3d shift{1,0,0};frame->translate(&shift,rw::COMBINEREPLACE);
        via.clear(0);direct.clear(0);atomic->render();
        for(auto t:scene) { for(auto& v:t.v) v.x+=1;direct.worldTriangle(t,camera); }
        if(direct.pixels()!=via.pixels()) result=1;
        if(!via.writePPM(argc>1?argv[1]:"librw-frame.ppm")) result=1;
        std::cout<<"librw_pipeline_match="<<(result==0)<<" triangles="<<via.stats().submitted
            <<" unsupported="<<bridge.unsupported()<<"\n";
        atomic->setFrame(nullptr);atomic->destroy();frame->destroy();
    }
    if(rw::Geometry::numAllocated||rw::Atomic::numAllocated||rw::Material::numAllocated||rw::Frame::numAllocated) {
        std::cerr<<"librw object leak\n";result=1;
    }
    rw::Engine::stop();rw::Engine::close();rw::Engine::term();return result;
}
