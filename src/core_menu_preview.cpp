#include "core_menu.hpp"
#include <fstream>
#include <string>
int main(int argc,char**argv){if(argc<2||argc>3)return 2;soft::CoreMenu menu;menu.toggle();if(argc==3){if(std::string(argv[2])=="dynamic")menu.row=2;else if(std::string(argv[2])=="about"){menu.row=17;menu.key(soft::CoreKey::Accept);}else if(std::string(argv[2])=="bottom")menu.row=19;else menu.help=true;}std::vector<uint16_t> frame(320*240);menu.draw(frame);
 std::ofstream f(argv[1],std::ios::binary);f<<"P6\n320 240\n255\n";for(auto v:frame){char rgb[]={char(((v>>11)&31)*255/31),char(((v>>5)&63)*255/63),char((v&31)*255/31)};f.write(rgb,3);}return f?0:1;}
