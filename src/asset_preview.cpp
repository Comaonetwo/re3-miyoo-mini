#include "asset_scene.hpp"
#include <cstdio>
#include <chrono>
#include <cstdlib>
int main(int argc,char** argv) {
    if(argc<3)return 2;
    try {
        auto scene=soft::AssetScene::load(argv[1]);auto camera=scene.spawn;
        if(argc>3)camera.yaw=std::strtof(argv[3],nullptr);
        if(argc>4)camera.position.y=std::strtof(argv[4],nullptr);
        if(argc>5)camera.farZ=std::strtof(argv[5],nullptr);
        if(camera.farZ<=camera.nearZ||camera.farZ>1000)return 2;
        soft::Renderer r;auto start=std::chrono::steady_clock::now();
        for(int i=0;i<60;++i)scene.render(r,camera,argc<=6||std::atoi(argv[6])!=0);
        double t=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::printf("objects=%zu triangles=%zu submitted=%llu host_fps=%.1f\n",scene.groups.size(),scene.triangles.size(),(unsigned long long)r.stats().submitted,60/t);
        return r.writePPM(argv[2])?0:1;
    }catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
