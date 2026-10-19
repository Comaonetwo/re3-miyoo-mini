#include "checkpoint_run.hpp"
#ifdef SOFT_RW_DEVICE
#include "demo_settings.hpp"
#include <cstring>
#include <memory>
#endif
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <chrono>
int main(int argc,char** argv){if(argc<5)return 2;
    try{auto city=soft::AssetScene::load(argv[1]),car=soft::AssetScene::load(argv[2]);auto world=soft::CollisionWorld::load(argv[3]);
        auto spawn=city.spawn.position;spawn.z+=10;soft::Driving driver(world,spawn,city.spawn.yaw);
        auto run=soft::CheckpointRun::street(world,driver.position,driver.yaw);soft::MiniMap map(world,driver.position,driver.yaw);
        int frames=argc>5?std::atoi(argv[5]):0;auto start=std::chrono::steady_clock::now();
        for(int i=0;i<frames;++i){driver.update(world,1.f/30,1,argc>6?std::strtof(argv[6],nullptr):0,false);run.update(driver.position,driver.speed,1./30,true);}
        auto camera=driver.chaseCamera(80,&world);soft::Renderer renderer;
#ifdef SOFT_RW_DEVICE
        std::unique_ptr<soft::SoftwareDevice> device;
        if(argc<8||std::strcmp(argv[7],"--direct"))device.reset(new soft::SoftwareDevice(renderer));
        std::unique_ptr<soft::RwAssetScene> rwCity,rwCar;soft::DemoSettings settings;
        if(argc>7){settings.night=!std::strcmp(argv[7],"--night");settings.quality=std::strcmp(argv[7],"--fast")!=0;settings.paintIndex=!std::strcmp(argv[7],"--red");}
        std::vector<soft::TransparentAtomic> transparent;
        if(device){rwCity.reset(new soft::RwAssetScene(city,device->world()));rwCar.reset(new soft::RwAssetScene(car,device->world()));settings.apply(*device,rwCar.get());device->begin(camera);}
#endif
#ifdef SOFT_RW_DEVICE
        if(device){renderer.clear(settings.sky());rwCity->gather(camera,transparent);rwCar->gather(camera,transparent,&driver);}
        else {city.render(renderer,camera);driver.draw(renderer,car,camera,true);}
#else
        city.render(renderer,camera);driver.draw(renderer,car,camera,true);
#endif
#ifdef SOFT_RW_DEVICE
        if(device){device->checkpoint(run,camera);soft::renderTransparent(transparent);if(argc<8||std::strcmp(argv[7],"--plain"))device->checkpointGlow(run);}else run.draw(renderer,camera);
#else
        run.draw(renderer,camera);
#endif
        auto frame=renderer.pixels();soft::drawDrivingHud(frame,map,run,driver,0,80,true,false,true);
#ifdef SOFT_RW_DEVICE
        settings.hud(frame);
        if(device){device->hud(frame);device->end();frame=renderer.pixels();
            std::printf("rw_shaded=%llu rw_atomics=%llu rw_im2d=%llu rw_im3d=%llu rw_unsupported=%llu\n",(unsigned long long)device->shadedTriangles(),(unsigned long long)device->atomicCalls(),(unsigned long long)device->im2dTriangles(),(unsigned long long)device->im3dTriangles(),(unsigned long long)device->unsupported());
            if(device->unsupported())return 1;}
#endif
        auto t=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::printf("car_triangles=%zu collision_faces=%zu position=%.2f %.2f %.2f speed=%.2f hit=%d seconds=%.3f\n",car.triangles.size(),world.faceCount(),driver.position.x,driver.position.y,driver.position.z,driver.speed,driver.hit,t);
        std::printf("gates=%zu/%zu timer=%.3f finished=%d wheel=%.3f steer=%.3f\n",run.completed(),run.route().size(),run.seconds(),run.finished(),driver.wheelAngle,driver.steeringAngle);
        std::ofstream out(argv[4],std::ios::binary);out<<"P6\n320 240\n255\n";
        for(auto color:frame){char rgb[3]={char(((color>>11)&31)*255/31),char(((color>>5)&63)*255/63),char((color&31)*255/31)};out.write(rgb,3);}return out?0:1;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
