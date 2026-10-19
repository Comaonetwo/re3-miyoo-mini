#include "menu_actions.hpp"
#include <stdexcept>
#include <cstdio>
using namespace soft;
static void check(bool b,const char* what){if(!b)throw std::runtime_error(what);}
int main(){try{MenuActions menu;
    check(menu.event(1,1,true,false)==MenuAction::None,"Menu down waits");check(menu.event(1,0,false,false)==MenuAction::Lighting,"solo Menu release");
    menu.event(1,1,true,false);check(menu.event(56,1,true,false)==MenuAction::Paint,"Menu+Y paint");check(menu.event(56,2,true,false)==MenuAction::None,"repeat ignored");check(menu.event(1,0,false,false)==MenuAction::None,"paint chord does not toggle night");
    menu.event(1,1,true,false);check(menu.event(29,1,true,false)==MenuAction::Quality,"Menu+B quality");check(menu.event(1,0,false,false)==MenuAction::None,"quality chord does not toggle night");
    check(menu.event(97,1,false,true)==MenuAction::None&&menu.event(97,0,false,false)==MenuAction::None,"solo Select ignored");
    menu.event(1,1,true,false);menu.event(97,1,true,true);menu.event(97,0,true,false);check(menu.event(1,0,false,false)==MenuAction::None,"exit chord suppresses lighting");
    menu.event(1,1,true,true);check(menu.event(1,0,false,true)==MenuAction::None,"reverse exit chord suppresses lighting");menu.reset();check(menu.event(1,0,false,false)==MenuAction::None,"dropped input release ignored");
    puts("Menu lighting/paint/quality and exit-chord isolation PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
