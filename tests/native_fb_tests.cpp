#include "fb_blit.hpp"
#include <vector>
#include <cstring>
#include <cstdio>
#include <stdexcept>
int main(){try{for(unsigned width:{320u,640u})for(bool rotate:{false,true})for(unsigned pad:{0u,16u}){soft::DisplayFormat f{640,480,2560+pad,32,{16,8,false},{8,8,false},{0,8,false},{24,8,false}};soft::FrameBlitter blit(f);std::vector<uint16_t> source(width*(width*3/4));for(unsigned i=0;i<source.size();++i)source[i]=uint16_t(i*117u+37u);std::vector<uint8_t> target(blit.requiredBytes(),0x5a);blit.present(source.data(),target.data(),target.size(),rotate,width);
 for(unsigned y=0;y<480;++y){for(unsigned x=0;x<640;++x){unsigned sx=(rotate?639-x:x)*width/640,sy=(rotate?479-y:y)*(width*3/4)/480;unsigned c=source[sy*width+sx],r=c>>11,g=(c>>5)&63,b=c&31;uint32_t expected=0xff000000u|(((r<<3)|(r>>2))<<16)|(((g<<2)|(g>>4))<<8)|((b<<3)|(b>>2)),actual;memcpy(&actual,target.data()+y*f.pitch+x*4,4);if(actual!=expected)throw std::runtime_error("native blit differs from reference");}for(unsigned x=2560;x<f.pitch;++x)if(target[y*f.pitch+x]!=0x5a)throw std::runtime_error("row padding touched");}
 }puts("PASS native framebuffer: every pixel, 320/640 input, rotation, channel layout, pitch padding");return 0;}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
