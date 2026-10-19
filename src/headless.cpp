#include "soft_renderer.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
int main(int argc,char** argv) {
    int frames=argc>1?std::atoi(argv[1]):120;
    if(frames<1||frames>100000) { std::cerr<<"frames must be 1..100000\n";return 2; }
    soft::Renderer r; soft::Camera c; auto scene=soft::makeScene();
    auto start=std::chrono::steady_clock::now();
    for(int i=0;i<frames;++i) soft::renderScene(r,scene,c);
    auto secs=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    if(!r.writePPM(argc>2?argv[2]:"frame.ppm")) return 1;
    std::cout<<"frames="<<frames<<" seconds="<<secs<<" host_fps="<<frames/secs
        <<" triangles="<<r.stats().submitted<<" rasterized="<<r.stats().rasterized
        <<" pixel_writes="<<r.stats().pixels<<" buffers_bytes="<<r.bufferBytes()<<"\n";
}
