#include "rw_asset_scene.hpp"
#include <cstdio>
#include <stdexcept>
#include <cmath>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static AssetScene fixture(){AssetScene s;Texture t;t.width=t.height=2;t.pixels={rgb(255,255,255),rgb(255,0,0),rgb(0,255,0),0};t.alpha={255,255,255,0};s.textures.push_back(t);
    for(int i=0;i<5;++i){float x=float(i-2)*.7f;s.triangles.push_back({{{x-.6f,0,0},{x+.6f,0,0},{x,1,0}},rgb(200,180,230)});
        s.materials.push_back({{{-1,0},{2,0},{.5f,2}},0,uint8_t(i%3+1),uint8_t(i%3+1)});s.groups.push_back({uint32_t(i),1,{x,.5f,0},1});}return s;}
int main(){try{
    auto scene=fixture();Renderer direct,cpu;Camera camera;camera.position={1,2,-8};camera.yaw=.12f;
    CollisionWorld ground({{{{-100,0,-100},{100,0,-100},{100,0,100}},0},{{{-100,0,-100},{100,0,100},{-100,0,100}},0}});
    Driving car(ground,{0,2,0},0);car.yaw=.3f;car.pitch=.11f;car.roll=-.13f;car.wheelAngle=.8f;car.steeringAngle=.2f;
    {SoftwareDevice device(cpu);{RwAssetScene rwScene(scene);check(rwScene.atomics()==5,"group object count");
        for(bool textured:{false,true})for(bool vehicle:{false,true}){
            direct.clear(0);cpu.clear(0);device.begin(camera);device.worldTextures(textured);
            if(vehicle)car.draw(direct,scene,camera,textured);else scene.draw(direct,camera,textured);
            rwScene.draw(camera,vehicle?&car:nullptr);device.end();size_t differences=0,visible=0;
            for(size_t i=0;i<direct.pixels().size();++i){differences+=direct.pixels()[i]!=cpu.pixels()[i];visible+=direct.pixels()[i]!=0;}
            std::printf("texture=%d vehicle=%d differences=%zu visible=%zu\n",textured,vehicle,differences,visible);
            check(visible>100,"fixture visible");check(differences<30,"Atomic pose/texture agrees with direct renderer within edge rounding");
        }
        camera.yaw=3.14f;cpu.clear(0);device.begin(camera);auto calls=device.atomicCalls();rwScene.draw(camera);device.end();check(calls==device.atomicCalls(),"world culling removes rear groups");
        check(device.unsupported()==0,"all asset states supported");
    }check(rw::Atomic::numAllocated==0&&rw::Geometry::numAllocated==0&&rw::Texture::numAllocated==0,"asset objects freed before engine shutdown");}
    check(rw::Engine::state==rw::Engine::Dead,"engine shutdown");puts("RW world/car: sampler modes, alpha, pose, culling, lifecycle PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
