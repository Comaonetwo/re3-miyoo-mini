#include <cstdio>
#include "common.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <vector>
#include "dxt_decoder.hpp"
// Bounded original PC D3D8 TXD decoder. No GPU/native raster allocation.
static unsigned le16(const uint8* p){return unsigned(p[0])|unsigned(p[1])<<8;}
static unsigned le32(const uint8* p){return le16(p)|le16(p+2)<<16;}
bool cpuNativeTextureRead(rw::Stream* stream,rw::Texture** result){
 *result=nullptr;uint32 length=0;if(!rw::findChunk(stream,rw::ID_STRUCT,&length,nullptr)||length<88||length>16*1024*1024){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 std::vector<uint8> body(length);if(stream->read8(body.data(),length)!=length){throw std::runtime_error("Invalid or unsupported PC TXD texture");}auto b=body.data();
 const unsigned format=le32(b+72),alpha=le32(b+76),w=le16(b+80),h=le16(b+82),depth=b[84],levels=b[85],compression=b[87],base=format&0xf00;
 if(le32(b)!=8||!w||!h||w>2048||h>2048||(w&(w-1))||(h&(h-1))||!levels||levels>12||(compression&&compression!=1&&compression!=3&&compression!=5)){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 unsigned pos=88,paletteCount=0;if(compression&&(format&0x6000)){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 if(format&0x6000){paletteCount=(format&0x4000)?32:256;if(depth!=4&&depth!=8){throw std::runtime_error("Invalid or unsupported PC TXD texture");}if(length-pos<paletteCount*4){throw std::runtime_error("Invalid or unsupported PC TXD texture");}pos+=paletteCount*4;}
 const unsigned bytes=paletteCount?1:depth/8;if(bytes<1||bytes>4||(!paletteCount&&bytes==1)){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 unsigned first=0,firstBytes=0;for(unsigned i=0;i<levels;++i){if(length-pos<4){throw std::runtime_error("Invalid or unsupported PC TXD texture");}unsigned n=le32(b+pos);pos+=4;if(n>length-pos){throw std::runtime_error("Invalid or unsupported PC TXD texture");}if(!i){first=pos;firstBytes=n;}pos+=n;}
 unsigned expected=compression?((w+3)/4)*((h+3)/4)*(compression==1?8:16):w*h*bytes;
 if(pos!=length||firstBytes!=expected){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 if(!compression&&!paletteCount&&!((bytes==3||bytes==4)&&(base==0x500||base==0x600))&&!(bytes==2&&(base==0x100||base==0x200||base==0x300||base==0xa00))){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 if(paletteCount)for(unsigned i=0;i<w*h;++i)if(b[first+i]>=paletteCount){throw std::runtime_error("Invalid or unsupported PC TXD texture");}
 char name[33]={};std::memcpy(name,b+8,32);for(char* p=name;*p;++p)if(*p>='A'&&*p<='Z')*p+=32;bool menuTexture=false;const char* menuNames[]={"gta3logo256","gamespy256","rockstarlogo128","gtalogo128","dmalogo128","singleplayer24","playersetup24","mainmenu24","hostgame24","findgame24","connection24","multiplayer24"};for(auto n:menuNames)if(!std::strcmp(name,n))menuTexture=true;
 unsigned cap=std::strstr(name,"font")?1024:menuTexture?512:(std::strstr(name,"hud")||std::strstr(name,"radar")||std::strstr(name,"pager")||!std::strncmp(name,"fe_",3)||!std::strncmp(name,"fe2_",4))?256:64,nw=w,nh=h;
 while(std::max(nw,nh)>cap){nw=std::max(1u,nw/2);nh=std::max(1u,nh/2);}
 if(!std::strcmp(name,"mainmenu24"))std::printf("UI_TEXTURE mainmenu24 source=%ux%u stored=%ux%u\n",w,h,nw,nh);
 auto image=rw::Image::create(nw,nh,32);if(!image){throw std::runtime_error("Invalid or unsupported PC TXD texture");}image->allocate();
 auto five=[](unsigned v){return uint8((v<<3)|(v>>2));};auto six=[](unsigned v){return uint8((v<<2)|(v>>4));};
 for(unsigned y=0;y<nh;++y)for(unsigned x=0;x<nw;++x){unsigned i=(y*h/nh)*w+x*w/nw;auto p=b+first+(compression?0:i*bytes);auto out=image->pixels+y*image->stride+x*4;
  if(compression)soft::dxtPixel(b+first,firstBytes,w,h,compression,x*w/nw,y*h/nh,out);
  else if(paletteCount)std::memcpy(out,b+88+unsigned(*p)*4,4);
  else if(bytes>=3){out[0]=p[2];out[1]=p[1];out[2]=p[0];out[3]=bytes==4?p[3]:255;}
  else{unsigned v=le16(p);if(base==0x200){out[0]=five(v>>11);out[1]=six((v>>5)&63);out[2]=five(v&31);out[3]=255;}
   else if(base==0x300){out[0]=((v>>8)&15)*17;out[1]=((v>>4)&15)*17;out[2]=(v&15)*17;out[3]=(v>>12)*17;}
   else{out[0]=five((v>>10)&31);out[1]=five((v>>5)&31);out[2]=five(v&31);out[3]=(base==0xa00||(v&0x8000))?255:0;}}
  if(!alpha)out[3]=255;
 }
 auto raster=rw::Raster::createFromImage(image);image->destroy();if(!raster){throw std::runtime_error("Invalid or unsupported PC TXD texture");}auto tex=rw::Texture::create(raster);if(!tex){raster->destroy();{throw std::runtime_error("Invalid or unsupported PC TXD texture");}}
 std::memcpy(tex->name,b+8,32);tex->name[31]=0;std::memcpy(tex->mask,b+40,32);tex->mask[31]=0;tex->filterAddressing=le32(b+4);tex->setFilter(rw::Texture::NEAREST);*result=tex;return true;
}
