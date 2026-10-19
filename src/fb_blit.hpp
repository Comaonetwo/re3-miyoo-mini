#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
namespace soft {
struct Channel { unsigned offset, length, msbRight; };
struct DisplayFormat {
    unsigned width,height,pitch,bpp;
    Channel red,green,blue,alpha;
};
// Converts/scales RGB565 into an existing Linux framebuffer layout.
// Does not change screen mode. The caller supplies a bounded visible page.
class FrameBlitter {
public:
    explicit FrameBlitter(DisplayFormat format);
    void present(const uint16_t* image, uint8_t* target, size_t bytes, bool rotate180,unsigned imageWidth=320) const;
    size_t requiredBytes() const { return size_t(format_.pitch)*format_.height; }
private:
    DisplayFormat format_;
    std::vector<uint32_t> lut_;
    std::vector<unsigned> xmap_,ymap_,xmap640_,ymap480_;
};
}
