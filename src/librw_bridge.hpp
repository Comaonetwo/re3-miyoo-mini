#pragma once
#include "soft_renderer.hpp"
#include <rw.h>
namespace soft {
// Experimental rigid, non-native Atomic pipeline. Not a full rw::Device.
// Caller owns pipeline; it must outlive bound Atomics. Single render thread.
class LibrwBridge {
public:
    LibrwBridge(Renderer& renderer, const Camera& camera);
    ~LibrwBridge();
    LibrwBridge(const LibrwBridge&)=delete;
    LibrwBridge& operator=(const LibrwBridge&)=delete;
    void bind(rw::Atomic* atomic);
    uint64_t unsupported() const { return unsupported_; }
private:
    Renderer& renderer_;
    const Camera& camera_;
    rw::ObjPipeline* pipeline_;
    uint64_t unsupported_=0;
    static LibrwBridge* active_;
    static void draw(rw::ObjPipeline*,rw::Atomic*);
};
}
