#pragma once
#include <cstdint>
#include <algorithm>
namespace soft {
// Independent RGB565 component tables reproduce the general blend exactly.
// Amortize their construction over large constant-colour, untextured triangles.
struct FlatBlend {
    uint16_t red[32],green[64],blue[32];
    FlatBlend() = default;
    FlatBlend(unsigned r,unsigned g,unsigned b,unsigned alpha,unsigned mode){
        auto blend=[&](unsigned source,unsigned dest){
            if(mode==1)return (source*alpha+dest*(255-alpha)+127)/255;
            if(mode==4)return (dest*(255-source)+127)/255;
            if(mode==5)return dest;
            if(mode==2||mode==3)return std::min(255u,dest+(source*(mode==2?alpha:255)+127)/255);
            return source;
        };
        for(unsigned i=0;i<32;++i){unsigned dest=(i<<3)|(i>>2);red[i]=uint16_t((blend(r,dest)>>3)<<11);blue[i]=uint16_t(blend(b,dest)>>3);}
        for(unsigned i=0;i<64;++i)green[i]=uint16_t((blend(g,(i<<2)|(i>>4))>>2)<<5);
    }
    uint16_t operator()(uint16_t old)const{return uint16_t(red[old>>11]|green[(old>>5)&63]|blue[old&31]);}
};
}
