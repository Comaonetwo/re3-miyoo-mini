#pragma once
#include "soft_renderer.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
namespace soft {
// Render-only density selection. Stable pool-slot ranks, no RNG and no allocation.
// Retain at least two particles per type/spatial cell; all particles within 8m.
class ParticleDensity {
    struct Group {int type=0,x=0,y=0,z=0;uint32_t first=~0u,second=~0u;bool used=false;};
    struct Candidate {unsigned group=0;uint32_t rank=0;float distance=0;bool valid=false;};
    std::array<Group,2048> groups_{};std::array<Candidate,1000> candidates_{};std::array<bool,1000> visible_{};
public:
    unsigned offered=0,skipped=0;
    void reset(){for(auto& g:groups_)g.used=false;for(auto& c:candidates_)c.valid=false;visible_.fill(true);offered=skipped=0;}
    static uint32_t rank(unsigned slot){uint32_t x=slot+1;x^=x>>16;x*=0x7feb352d;x^=x>>15;x*=0x846ca68b;return x^(x>>16);}
    void offer(unsigned slot,int type,Vec3 position,Vec3 camera){
        if(slot>=1000||!std::isfinite(position.x+position.y+position.z+camera.x+camera.y+camera.z)||std::fabs(position.x)>1e6||std::fabs(position.y)>1e6||std::fabs(position.z)>1e6)return;
        float dx=position.x-camera.x,dy=position.y-camera.y,dz=position.z-camera.z,d=dx*dx+dy*dy+dz*dz;if(!std::isfinite(d)||d<=64)return;
        int x=int(std::floor(position.x/16)),y=int(std::floor(position.y/16)),z=int(std::floor(position.z/16));
        unsigned hash=(uint32_t(x)*73856093u^uint32_t(y)*19349663u^uint32_t(z)*83492791u^uint32_t(type)*2654435761u)&2047;
        for(unsigned n=0;n<2048;++n){auto& g=groups_[(hash+n)&2047];if(!g.used){g=Group();g.used=true;g.type=type;g.x=x;g.y=y;g.z=z;}else if(g.type!=type||g.x!=x||g.y!=y||g.z!=z)continue;
            unsigned group=(hash+n)&2047;uint32_t r=rank(slot);if(r<g.first){g.second=g.first;g.first=r;}else if(r<g.second)g.second=r;
            auto& c=candidates_[slot];c.group=group;c.rank=r;c.distance=d;c.valid=true;++offered;return;
        }
    }
    void finish(){for(unsigned slot=0;slot<1000;++slot){auto c=candidates_[slot];if(!c.valid)continue;auto& g=groups_[c.group];if(c.rank==g.first||c.rank==g.second)continue;
        float keep=1-.5f*std::min(1.f,(c.distance-64.f)/192.f);float value=float(c.rank&65535u)/65536.f;
        visible_[slot]=value<keep;skipped+=!visible_[slot];}}
    bool visible(unsigned slot)const{return slot>=1000||visible_[slot];}
};
}
