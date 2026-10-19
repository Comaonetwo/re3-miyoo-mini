#pragma once
#include "soft_renderer.hpp"
#include <cstdint>
namespace soft {
struct CoreSettings {
    static constexpr uint32_t defaultBits=(1u<<24)|(1u<<22)|3u|12u|32u|2048u|(2u<<15)|(1u<<17)|128u|256u;
    uint32_t bits;uint32_t customGraphics=0;bool customSelected=false;
    static constexpr uint32_t graphicsMask=3u|12u|16u|32u|64u|1536u|6144u|16384u|98304u|131072u|262144u|(3u<<19)|(1u<<21)|(1u<<22)|(15u<<24);
    unsigned preset()const;void applyPreset(unsigned id);void graphicsChanged(){customSelected=true;customGraphics=bits&graphicsMask;}
    static const char* presetName(unsigned id);
    explicit CoreSettings(uint32_t b=defaultBits):bits((valid(b)?b:defaultBits)&~((1u<<21)|(3u<<25))),customGraphics(bits&graphicsMask){}
    static bool valid(uint32_t b){return !(b&~268435455u)&&(!(b&(1u<<27))||(b&(1u<<17)))&&(!((b>>19)&3)||((b>>2)&3)==3)&&!((b&(1u<<18))&&(b&(1u<<4)))&&((b>>15)&3)!=3&&((b>>9)&3)!=3&&((b>>11)&3)!=3;}
    unsigned scale() const {return (bits&3)==1?1:(bits&3)==2?4:(bits&3)==3?3:2;}
    unsigned distance() const {unsigned extra=(bits>>19)&3;return extra==3?120:extra==1?140:extra==2?180:40+20*((bits>>2)&3);}
    unsigned distanceIndex()const{unsigned e=(bits>>19)&3;return e==3?4:e?4+e:((bits>>2)&3);}
    void distanceIndex(unsigned i){i%=7;unsigned extra=i==4?3:i>=5?i-4:0;bits=(bits&~(12u|(3u<<19)))|((i<4?i:3u)<<2)|(extra<<19);}
    bool dynamicWorld()const{return bits&(1u<<24);}
    bool reducedDecor()const{return bits&(1u<<25);}
    bool farLighting()const{return bits&(1u<<26);}
    bool parallelRenderer()const{return bits&(1u<<23);}
    bool reducedParticles()const{return bits&(1u<<22);}
    bool experimentalWorld()const{return bits&(1u<<21);}
    bool bakedLighting()const{return bits&(1u<<18);}
    bool smooth() const {return bits&(1u<<4);}
    bool textures() const {return bits&(1u<<5);}
    bool shadows() const {return bits&(1u<<6);}
    bool stats() const {return bits&(1u<<7);}
    bool nativeUi() const {return bits&(1u<<9);}
    unsigned hudWidth() const {return bits&(1u<<10)?240:nativeUi()?640:320;}
    unsigned menuWidth() const {return ((bits>>11)&3)==1?240:((bits>>11)&3)==2?320:640;}
    unsigned skyWidth() const {return ((bits>>15)&3)==1?160:((bits>>15)&3)==2?120:0;}
    bool lowRain() const {return bits&(1u<<27);}
    bool reducedRain() const {return bits&(1u<<17);}
    bool fastEffects() const {return !(bits&(1u<<14));}
    bool autoAim() const {return !(bits&(1u<<13));}
    bool camera() const {return bits&(1u<<8);}
    static CoreSettings load(const char* path);
    bool save(const char* path) const;
};
enum class CoreKey {Up,Down,Left,Right,Accept,Back,Start};
class CoreMenu {
public:
    CoreSettings settings;bool open=false,help=false,about=false,exitRequested=false;unsigned row=0;
    static constexpr unsigned rowCount=20;
    void toggle(){open=!open;help=false;about=false;}
    void key(CoreKey key);
    void draw(std::vector<uint16_t>& frame) const;
};
}
