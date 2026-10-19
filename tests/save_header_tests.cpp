#include "save_header.hpp"
#include <cassert>
int main(){uint8_t bytes[188]={};bytes[4]='U';bytes[6]='I';uint32_t level=99;for(uint32_t value=0;value<4;++value){bytes[0x44]=value;assert(soft::saveLevel(bytes,sizeof(bytes),level)&&level==value);}assert(!soft::saveLevel(bytes,0x47,level));assert(!soft::saveLevel(nullptr,sizeof(bytes),level));bytes[0x44]=4;assert(!soft::saveLevel(bytes,sizeof(bytes),level));bytes[0x44]=1;bytes[0x47]=255;assert(!soft::saveLevel(bytes,sizeof(bytes),level));}
