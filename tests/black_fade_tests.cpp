#include "black_fade.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
int main(){auto& lut=soft::BlackFade::get();for(unsigned a=0;a<256;++a)for(unsigned p=0;p<65536;++p){unsigned r=p>>11,g=(p>>5)&63,b=p&31;r=((r<<3)|(r>>2))*(255-a)+127;g=((g<<2)|(g>>4))*(255-a)+127;b=((b<<3)|(b>>2))*(255-a)+127;assert(lut(uint16_t(p),a)==soft::rgb(r/255,g/255,b/255));}puts("PASS all 16777216 black source-over RGB565/alpha combinations");}
