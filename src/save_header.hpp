#pragma once
#include <cstdint>
#include <cstddef>
namespace soft {
// GTA III simple vars: UTF16 name[24], SYSTEMTIME[16], save size[4], level[4].
// Never read an unchecked integer directly into the engine's enum.
inline bool saveLevel(const uint8_t* bytes,size_t size,uint32_t& level){if(!bytes||size<0x48)return false;uint32_t value=uint32_t(bytes[0x44])|(uint32_t(bytes[0x45])<<8)|(uint32_t(bytes[0x46])<<16)|(uint32_t(bytes[0x47])<<24);if(value>3)return false;level=value;return true;}
}
