#include "core_menu.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool x,const char* message){if(!x)throw std::runtime_error(message);}
int main(){try{
 CoreMenu m;m.toggle();const uint32_t bits=m.settings.bits;
 check(m.settings.preset()==1&&m.settings.dynamicWorld()&&m.settings.autoAim()&&m.settings.hudWidth()==320,"Fresh install starts in MEDIUM with AUTO and auto aim");
 m.row=17;m.key(CoreKey::Accept);check(m.about&&m.open&&!m.help,"About opens without closing settings");
 m.key(CoreKey::Right);m.key(CoreKey::Down);check(m.row==17&&m.settings.bits==bits,"About does not edit settings");
 std::vector<uint16_t> image(320*240);m.draw(image);check(image[65*320+12]!=0,"About draws");
 m.key(CoreKey::Back);check(!m.about&&m.open,"Back returns to settings");
 m.key(CoreKey::Accept);m.key(CoreKey::Start);check(!m.about&&m.open,"Start returns from About");
 m.row=16;m.key(CoreKey::Accept);check(m.help&&!m.about,"Hotkeys opens");m.key(CoreKey::Back);
 m.row=18;m.key(CoreKey::Accept);check(!m.open&&!m.exitRequested,"Resume returns to game");
 m.toggle();m.row=19;m.key(CoreKey::Accept);check(m.exitRequested,"Exit still exits to Onion");
 m.exitRequested=false;m.row=0;m.key(CoreKey::Up);check(m.row==19,"Up wraps to final row");m.key(CoreKey::Down);check(m.row==0,"Down wraps to first row");
 m.row=17;m.key(CoreKey::Accept);m.toggle();m.toggle();check(!m.about&&!m.help,"Reopening resets subpages");
 check(m.settings.bits==bits,"Navigation preserves settings");puts("PASS About, hotkeys, resume, exit, wrapping and unchanged settings");return 0;
}catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
