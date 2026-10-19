#include "fb_blit.hpp"
#include "soft_renderer.hpp"
#include <stdexcept>
#include <cstring>
namespace soft {
static uint32_t mask(Channel c,unsigned bpp) {
    if(c.msbRight||c.length>8||c.offset+c.length>bpp) throw std::runtime_error("unsupported framebuffer channel");
    return c.length?((1u<<c.length)-1u)<<c.offset:0;
}
static uint32_t channel(unsigned value,Channel c) {
    return c.length?((value*((1u<<c.length)-1u)+127)/255)<<c.offset:0;
}
FrameBlitter::FrameBlitter(DisplayFormat f):format_(f),lut_(65536) {
    if((f.bpp!=16&&f.bpp!=32)||f.width<1||f.height<1||f.width>2048||f.height>2048||
       f.pitch<f.width*(f.bpp/8)||f.pitch>16384) throw std::runtime_error("unsupported framebuffer dimensions/pitch/bpp");
    uint32_t rm=mask(f.red,f.bpp),gm=mask(f.green,f.bpp),bm=mask(f.blue,f.bpp),am=mask(f.alpha,f.bpp);
    if(!rm||!gm||!bm||(rm&gm)||(rm&bm)||(gm&bm)||(am&(rm|gm|bm))) throw std::runtime_error("invalid framebuffer channel masks");
    // Some RGB888/ARGB8888 display paths also expect unused upper bits to be set.
    uint32_t opaque=f.bpp==32?~(rm|gm|bm):am;
    for(unsigned i=0;i<65536;++i) {
        unsigned r=(i>>11)&31,g=(i>>5)&63,b=i&31;
        lut_[i]=channel((r<<3)|(r>>2),f.red)|channel((g<<2)|(g>>4),f.green)|channel((b<<3)|(b>>2),f.blue)|opaque;
    }
    xmap_.resize(f.width);ymap_.resize(f.height);xmap640_.resize(f.width);ymap480_.resize(f.height);
    for(unsigned x=0;x<f.width;++x)xmap640_[x]=unsigned(size_t(x)*640/f.width);
    for(unsigned y=0;y<f.height;++y)ymap480_[y]=unsigned(size_t(y)*480/f.height);
    for(unsigned x=0;x<f.width;++x)xmap_[x]=unsigned(size_t(x)*Renderer::width/f.width);
    for(unsigned y=0;y<f.height;++y)ymap_[y]=unsigned(size_t(y)*Renderer::height/f.height);
}
void FrameBlitter::present(const uint16_t* image,uint8_t* target,size_t bytes,bool rotate180,unsigned imageWidth) const {
    if(imageWidth!=320&&imageWidth!=640)throw std::runtime_error("invalid input frame width");
    const auto& xm=imageWidth==640?xmap640_:xmap_;const auto& ym=imageWidth==640?ymap480_:ymap_;
    if(!image||!target||bytes<requiredBytes()) throw std::runtime_error("framebuffer page too small");
    // Actual Miyoo mode: 640x480 RGB32. Avoid generic coordinate maps and
    // per-pixel format branching; build doubled rows in RAM, never read FB back.
    if(format_.width==640&&format_.height==480&&format_.bpp==32){
        if(imageWidth==320){uint32_t line[640];for(unsigned y=0;y<240;++y){unsigned sy=rotate180?239-y:y;const uint16_t* source=image+sy*320;
            for(unsigned x=0;x<320;++x){uint32_t p=lut_[source[rotate180?319-x:x]];line[x*2]=line[x*2+1]=p;}
            std::memcpy(target+size_t(y*2)*format_.pitch,line,sizeof(line));std::memcpy(target+size_t(y*2+1)*format_.pitch,line,sizeof(line));
        }}else for(unsigned y=0;y<480;++y){const uint16_t* source=image+(rotate180?479-y:y)*640;uint8_t* row=target+size_t(y)*format_.pitch;
            for(unsigned x=0;x<640;x+=4){uint32_t line[4];for(unsigned k=0;k<4;++k)line[k]=lut_[source[rotate180?639-x-k:x+k]];std::memcpy(row+x*4,line,sizeof(line));}
        }
        return;
    }
    for(unsigned y=0;y<format_.height;++y) {
        unsigned sy=ym[rotate180?format_.height-1-y:y];
        uint8_t* row=target+size_t(y)*format_.pitch;
        for(unsigned x=0;x<format_.width;++x) {
            unsigned sx=xm[rotate180?format_.width-1-x:x];
            uint32_t pixel=lut_[image[size_t(sy)*imageWidth+sx]];
            if(format_.bpp==32) std::memcpy(row+x*4,&pixel,4);
            else {uint16_t p=uint16_t(pixel);std::memcpy(row+x*2,&p,2);}
        }
    }
}
}
