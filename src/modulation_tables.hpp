#pragma once
#include <cstdint>
namespace soft {
// Store unquantized 8-bit results, preserving the original rounding before
// RGB565 conversion or source-alpha blending. 24 KiB shared once per process.
struct ModulationTables {
 uint8_t five[256*32],six[256*64];
 ModulationTables(){for(unsigned c=0;c<256;++c){for(unsigned t=0;t<32;++t)five[c*32+t]=c*((t<<3)|(t>>2))/255;for(unsigned t=0;t<64;++t)six[c*64+t]=c*((t<<2)|(t>>4))/255;}}
 static const ModulationTables& get(){static const ModulationTables tables;return tables;}
};
}
