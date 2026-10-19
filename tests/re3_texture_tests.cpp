#include "software_device.hpp"
#include <cassert>
#include <cstring>
#include <vector>
bool cpuNativeTextureRead(rw::Stream*,rw::Texture**);
static void put(std::vector<uint8_t>& b,unsigned offset,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)b[offset+i]=uint8_t(value>>(8*i));}
static std::vector<uint8_t> body(unsigned format,unsigned depth,unsigned compression,unsigned count){std::vector<uint8_t> b(88+4+count);put(b,0,8);put(b,4,0x1101);std::memcpy(b.data()+8,"synthetic",9);put(b,72,format);put(b,76,1);put(b,80,4,2);put(b,82,4,2);b[84]=depth;b[85]=1;b[86]=4;b[87]=compression;put(b,88,count);return b;}
static rw::Texture* read(std::vector<uint8_t> b){std::vector<uint8_t> chunk(12+b.size());put(chunk,0,1);put(chunk,4,b.size());put(chunk,8,0x1803ffff);std::memcpy(chunk.data()+12,b.data(),b.size());rw::StreamMemory stream;stream.open(chunk.data(),chunk.size());rw::Texture* tex=nullptr;if(!cpuNativeTextureRead(&stream,&tex))return nullptr;return tex;}
int main(){soft::Renderer renderer;soft::SoftwareDevice device(renderer);
 auto b=body(0x500,32,0,64);for(unsigned i=0;i<16;++i){b[92+i*4]=30;b[93+i*4]=20;b[94+i*4]=10;b[95+i*4]=77;}auto tex=read(b);assert(tex&&tex->raster->width==4&&tex->raster->pixels[0]==10&&tex->raster->pixels[3]==77);tex->destroy();
 b=body(0x500,16,1,8);b[93]=248;tex=read(b);assert(tex&&tex->raster->pixels[0]==255);tex->destroy();
 b=body(0x2500,8,0,16);b.insert(b.begin()+88,1024,0);b[88]=10;b[89]=20;b[90]=30;b[91]=44;tex=read(b);assert(tex&&tex->raster->pixels[0]==10&&tex->raster->pixels[3]==44);tex->destroy();
 bool bad=false;b.pop_back();try{tex=read(b);}catch(...){bad=true;}assert(bad);
 b=body(0x500,32,0,64);b[87]=2;bad=false;try{tex=read(b);}catch(...){bad=true;}assert(bad);
 b=body(0x500,32,0,512*512*4);put(b,80,512,2);put(b,82,512,2);std::memcpy(b.data()+8,"FONT1",6);tex=read(b);assert(tex&&tex->raster->width==512&&tex->raster->height==512);tex->destroy();
 std::memset(b.data()+8,0,32);std::memcpy(b.data()+8,"fe_panel",9);tex=read(b);assert(tex&&tex->raster->width==256);tex->destroy();
 std::memset(b.data()+8,0,32);std::memcpy(b.data()+8,"mainmenu24",11);tex=read(b);assert(tex&&tex->raster->width==512);tex->destroy();
 std::memset(b.data()+8,0,32);std::memcpy(b.data()+8,"fence",6);tex=read(b);assert(tex&&tex->raster->width==64);tex->destroy();
 std::memset(b.data()+8,0,32);std::memcpy(b.data()+8,"ordinary",9);tex=read(b);assert(tex&&tex->raster->width==64);tex->destroy();
 auto target=rw::Raster::create(0,0,0,rw::Raster::CAMERA);assert(target&&target->width==320&&target->depth==16);target->destroy();
 auto procedural=rw::Raster::create(16,16,0,rw::Raster::TEXTURE|rw::Raster::C8888);assert(procedural&&procedural->depth==32);procedural->destroy();assert(device.unsupported()==0);
}
