#include "demo_settings.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
int main(int argc,char** argv){if(argc<4)return 2;try{
    auto city=soft::AssetScene::load(argv[1]),car=soft::AssetScene::load(argv[2]);auto collision=soft::CollisionWorld::load(argv[3]);auto spawn=city.spawn.position;spawn.z+=10;
    soft::Driving driver(collision,spawn,city.spawn.yaw);auto run=soft::CheckpointRun::street(collision,driver.position,driver.yaw);soft::MiniMap map(collision,driver.position,driver.yaw);
    auto camera=driver.chaseCamera(80,&collision);soft::Renderer renderer;soft::SoftwareDevice device(renderer);soft::RwAssetScene world(city,device.world()),vehicle(car,device.world());city.discardConvertedData();car.discardConvertedData();
    soft::DemoSettings settings;if(argc>5){settings.quality=std::strcmp(argv[5],"fast")!=0;settings.night=!std::strcmp(argv[5],"night");}settings.apply(device,&vehicle);
    int count=argc>4?std::atoi(argv[4]):100;if(count<1||count>10000)return 2;std::vector<double> times;std::vector<soft::TransparentAtomic> objects;
    for(int i=0;i<count+3;++i){auto begin=std::chrono::steady_clock::now();device.begin(camera);renderer.clear(settings.sky());objects.clear();world.gather(camera,objects);vehicle.gather(camera,objects,&driver);device.checkpoint(run,camera);soft::renderTransparent(objects);device.checkpointGlow(run);
        auto frame=renderer.pixels();soft::drawDrivingHud(frame,map,run,driver,0,80,true,false,true);settings.hud(frame);device.hud(frame);device.end();double elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();if(i>=3)times.push_back(elapsed);}
    if(argc>6){std::ofstream file(argv[6],std::ios::binary);file<<"P6\n320 240\n255\n";for(auto pixel:renderer.pixels()){unsigned char rgb[]={uint8_t(((pixel>>11)&31)*255/31),uint8_t(((pixel>>5)&63)*255/63),uint8_t((pixel&31)*255/31)};file.write(reinterpret_cast<char*>(rgb),3);}}
    double total=0;for(double t:times)total+=t;std::sort(times.begin(),times.end());
    std::printf("native benchmark quality=%d night=%d frames=%d mean_ms=%.3f median_ms=%.3f worst_ms=%.3f unsupported=%llu shaded=%llu\n",settings.quality,settings.night,count,total/count,times[count/2],times.back(),(unsigned long long)device.unsupported(),(unsigned long long)device.shadedTriangles());return device.unsupported()?1:0;
}catch(const std::exception& e){std::puts(e.what());return 1;}}
