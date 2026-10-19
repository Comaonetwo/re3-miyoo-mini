#pragma once
#include "software_device.hpp"
#include "driving.hpp"
namespace soft {
struct TransparentAtomic {rw::Atomic* atomic;float distance;};
// Owns actual librw objects and bounds. Source may be discarded after building;
// Device/World must outlive this.
class RwAssetScene {
    struct Impl;std::unique_ptr<Impl> impl_;
public:
    explicit RwAssetScene(const AssetScene& scene,rw::World* world=nullptr);
    ~RwAssetScene();
    // Compatibility convenience for one scene; multi-scene frontends gather
    // transparent objects from all scenes before a common sorted pass.
    void draw(const Camera& camera,const Driving* vehicle=nullptr);
    void gather(const Camera& camera,std::vector<TransparentAtomic>& transparent,const Driving* vehicle=nullptr);
    void paint(uint32_t primary,uint32_t secondary);
    size_t atomics() const;
};
void renderTransparent(std::vector<TransparentAtomic>& objects);
}
