#pragma once
#include <cstdint>
namespace soft {
// Native firearm lock-on only; scopes, melee, cars and menu input keep GTA controls.
inline bool miyooAutoAim(uint32_t buttons,bool firearm,bool onFoot,bool gameplay,bool enabled){return enabled&&firearm&&onFoot&&gameplay&&(buttons&32)&&!(buttons&2048);}
}
