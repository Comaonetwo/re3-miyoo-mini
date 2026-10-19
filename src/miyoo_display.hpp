#pragma once
#include "fb_blit.hpp"
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <algorithm>
namespace softHost {
soft::Channel field(fb_bitfield f) {return {f.offset,f.length,f.msb_right};}
class MiyooDisplay {
    int fd_=-1;uint8_t* mapped_=nullptr;size_t length_=0;
    fb_var_screeninfo original_{};fb_fix_screeninfo fixed_{};
    size_t visible_=0;std::vector<uint8_t> saved_;
    std::unique_ptr<soft::FrameBlitter> blitter_;
public:
    MiyooDisplay() {
        fd_=open("/dev/fb0",O_RDWR|O_CLOEXEC);
        if(fd_<0) throw std::runtime_error(std::string("open /dev/fb0: ")+strerror(errno));
        try {
            if(ioctl(fd_,FBIOGET_FSCREENINFO,&fixed_)<0||ioctl(fd_,FBIOGET_VSCREENINFO,&original_)<0)
                throw std::runtime_error(std::string("framebuffer info: ")+strerror(errno));
            if(fixed_.type!=FB_TYPE_PACKED_PIXELS||fixed_.visual!=FB_VISUAL_TRUECOLOR)
                throw std::runtime_error("framebuffer is not packed truecolor");
            if(original_.nonstd||original_.grayscale) throw std::runtime_error("unsupported framebuffer mode");
            soft::DisplayFormat f{original_.xres,original_.yres,fixed_.line_length,original_.bits_per_pixel,
                field(original_.red),field(original_.green),field(original_.blue),field(original_.transp)};
            blitter_.reset(new soft::FrameBlitter(f));
            length_=fixed_.smem_len;
            visible_=size_t(original_.yoffset)*fixed_.line_length+size_t(original_.xoffset)*(original_.bits_per_pixel/8);
            size_t required=blitter_->requiredBytes();
            if(length_==0||length_>32*1024*1024||visible_>length_||required>length_-visible_||
               uint64_t(original_.xoffset+original_.xres)*(original_.bits_per_pixel/8)>fixed_.line_length)
                throw std::runtime_error("framebuffer page is outside mapped memory");
            void* p=mmap(nullptr,length_,PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
            if(p==MAP_FAILED) throw std::runtime_error(std::string("mmap framebuffer: ")+strerror(errno));
            mapped_=static_cast<uint8_t*>(p);
            saved_.assign(mapped_+visible_,mapped_+visible_+required);
            std::printf("framebuffer=%ux%u bpp=%u pitch=%u yoffset=%u smem=%zu rotate=180\n",
                original_.xres,original_.yres,original_.bits_per_pixel,fixed_.line_length,original_.yoffset,length_);
        } catch(...) { if(mapped_)munmap(mapped_,length_);close(fd_);fd_=-1;throw; }
    }
    ~MiyooDisplay() {
        if(mapped_) {
            std::copy(saved_.begin(),saved_.end(),mapped_+visible_);
            munmap(mapped_,length_);
        }
        if(fd_>=0)close(fd_);
    }
    void present(const uint16_t* pixels,bool rotated,unsigned imageWidth=320) {blitter_->present(pixels,mapped_+visible_,length_-visible_,rotated,imageWidth);}
};
}
