#include "fb_blit.hpp"
#include "soft_renderer.hpp"
#include <vector>
#include <iostream>
#include <cstring>
#include <stdexcept>
static void check(bool p,const char* s) {if(!p){std::cerr<<s<<"\n";std::exit(1);}}
static uint32_t pixel(const std::vector<uint8_t>& data,size_t pos) {uint32_t p;std::memcpy(&p,data.data()+pos,4);return p;}
int main() {
    std::vector<uint16_t> image(320*240);
    for(int y=0;y<240;++y)for(int x=0;x<320;++x)
        image[y*320+x]=y<120?(x<160?soft::rgb(255,0,0):soft::rgb(0,255,0)):
            (x<160?soft::rgb(0,0,255):soft::rgb(255,255,255));
    for(auto size: {std::pair<unsigned,unsigned>{640,480},{752,560}}) {
        unsigned w=size.first,h=size.second,pitch=w*4+16;
        soft::DisplayFormat f{w,h,pitch,32,{16,8,0},{8,8,0},{0,8,0},{24,8,0}};
        soft::FrameBlitter blit(f);std::vector<uint8_t> memory(blit.requiredBytes()+32,0xa5);
        blit.present(image.data(),memory.data()+16,blit.requiredBytes(),false);
        check(pixel(memory,16)==0xffff0000,"top-left color/alpha");
        check(pixel(memory,16+(w-1)*4)==0xff00ff00,"top-right color");
        check(pixel(memory,16+(h-1)*pitch)==0xff0000ff,"bottom-left color");
        blit.present(image.data(),memory.data()+16,blit.requiredBytes(),true);
        check(pixel(memory,16)==0xffffffff,"rotated top-left");
        check(pixel(memory,16+(h-1)*pitch+(w-1)*4)==0xffff0000,"rotated bottom-right");
        for(unsigned y=0;y<h;++y)for(unsigned x=w*4;x<pitch;++x)check(memory[16+y*pitch+x]==0xa5,"padding overwritten");
        for(unsigned i=0;i<16;++i)check(memory[i]==0xa5&&memory[memory.size()-1-i]==0xa5,"guard overwritten");
        bool threw=false;try{blit.present(image.data(),memory.data(),blit.requiredBytes()-1,false);}catch(const std::runtime_error&){threw=true;}
        check(threw,"short page accepted");
    }
    soft::DisplayFormat f{320,240,640,16,{11,5,0},{5,6,0},{0,5,0},{0,0,0}};
    soft::FrameBlitter identity(f);for(size_t i=0;i<image.size();++i)image[i]=uint16_t(i);
    std::vector<uint8_t> out(identity.requiredBytes());identity.present(image.data(),out.data(),out.size(),false);
    check(std::memcmp(image.data(),out.data(),out.size())==0,"RGB565 identity differs");
    soft::DisplayFormat native{640,480,1280,16,{11,5,0},{5,6,0},{0,5,0},{0,0,0}};soft::FrameBlitter hi(native);std::vector<uint16_t> fine(640*480);for(size_t i=0;i<fine.size();++i)fine[i]=uint16_t(i*7);
 std::vector<uint8_t> bytes(hi.requiredBytes());hi.present(fine.data(),bytes.data(),bytes.size(),false,640);check(!std::memcmp(fine.data(),bytes.data(),bytes.size()),"640 native source identity");
 hi.present(fine.data(),bytes.data(),bytes.size(),true,640);for(size_t i=0;i<fine.size();++i){uint16_t v;std::memcpy(&v,bytes.data()+i*2,2);check(v==fine[fine.size()-1-i],"640 native source rotation");}
 f.green={11,6,0};bool threw=false;try{soft::FrameBlitter bad(f);}catch(const std::runtime_error&){threw=true;}
    check(threw,"invalid/overlapping channels accepted");
    std::cout<<"PASS: 640x480/752x560 scaling, rotation, ARGB8888, RGB565 identity, pitch padding, bounds, masks\n";
}
