#include "modulation_tables.hpp"
#include "soft_renderer.hpp"
#include <cassert>
#include <cstdio>
int main(){auto& t=soft::ModulationTables::get();for(unsigned c=0;c<256;++c){for(unsigned x=0;x<32;++x)assert(t.five[c*32+x]==c*((x<<3)|(x>>2))/255);for(unsigned x=0;x<64;++x)assert(t.six[c*64+x]==c*((x<<2)|(x>>4))/255);}
 assert(sizeof(t)==24576);printf("PASS exhaustive 24576 unquantized RGB modulation results; shared immutable table 24 KiB\n");}
