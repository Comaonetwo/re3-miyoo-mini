#include "import_legacy_saves.hpp"
#include <cassert>
#include <fstream>
#include <sys/stat.h>
#include <iterator>
int main(int argc,char** argv){assert(argc==2);std::string root=argv[1];mkdir(root.c_str(),0700);std::string folder=root+"/userfiles";mkdir(folder.c_str(),0700);auto write=[](const std::string& p,const char* s){std::ofstream f(p);f<<s;};auto read=[](const std::string& p){std::ifstream f(p);return std::string(std::istreambuf_iterator<char>(f),{});};write(root+"/1.b","old save");write(root+"/2.b","old second");write(folder+"/GTA3sf2.b","current save");assert(soft::importLegacySaves(root,folder)==1);assert(read(root+"/1.b")=="old save"&&read(folder+"/GTA3sf1.b")=="old save");assert(read(folder+"/GTA3sf2.b")=="current save");assert(soft::importLegacySaves(root,folder)==0);for(auto name:{"/1.b","/2.b","/userfiles/GTA3sf1.b","/userfiles/GTA3sf2.b"})std::remove((root+name).c_str());rmdir(folder.c_str());rmdir(root.c_str());}
