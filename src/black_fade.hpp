#pragma once
#include <cstdint>
namespace soft {
// Exact rounded source-over with an RGB-zero source. 24 KiB, process lifetime.
struct BlackFade {
 uint8_t five[256*32],six[256*64];
 BlackFade(){for(unsigned a=0;a<256;++a){for(unsigned d=0;d<32;++d)five[(a<<5)|d]=uint8_t((((((d<<3)|(d>>2))*(255-a)+127)/255)>>3));for(unsigned d=0;d<64;++d)six[(a<<6)|d]=uint8_t((((((d<<2)|(d>>4))*(255-a)+127)/255)>>2));}}
 uint16_t operator()(uint16_t p,unsigned a)const{return uint16_t((five[(a<<5)|(p>>11)]<<11)|(six[(a<<6)|((p>>5)&63)]<<5)|five[(a<<5)|(p&31)]);}
 static const BlackFade& get(){static const BlackFade table;return table;}
};
}
