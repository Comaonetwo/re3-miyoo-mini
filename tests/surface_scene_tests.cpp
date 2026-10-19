#include "asset_scene.hpp"
#include <cstdio>
#include <fstream>
#include <cstring>
#include <stdexcept>
using namespace soft;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
static void u32(std::string& b,uint32_t n){for(int i=0;i<4;++i)b.push_back(char(n>>(i*8)));}
static void f32(std::string& b,float f){uint32_t n;std::memcpy(&n,&f,4);u32(b,n);}
static void vec(std::string& b,float x,float y,float z){f32(b,x);f32(b,y);f32(b,z);}
static void save(const char* p,const std::string& b){std::ofstream o(p,std::ios::binary);o.write(b.data(),b.size());}
static bool rejects(const char* p,const std::string& b){save(p,b);try{AssetScene::load(p);return false;}catch(const std::runtime_error&){return true;}}
int main(int argc,char** argv){if(argc!=2)return 2;try{
    std::string b="M3SCENE3";u32(b,1);vec(b,0,2,-8);f32(b,0);u32(b,1);u32(b,0);u32(b,0);u32(b,1);vec(b,0,.5f,5);f32(b,2);
    vec(b,-1,0,5);vec(b,1,0,5);vec(b,0,1,5);b.append(2,char(255));for(int i=0;i<6;++i)f32(b,0);b.append(2,char(255));b.push_back(3);b.push_back(3);
    size_t start=b.size();for(int i=0;i<3;++i){b.push_back(char(i==0?255:0));b.push_back(char(i==1?255:0));b.push_back(char(i==2?255:0));b.push_back(char(255));}
    b.push_back(char(255));b.push_back(0);b.push_back(0);b.push_back(char(128));for(int i=0;i<3;++i)vec(b,0,0,1);f32(b,.25f);f32(b,1);b.push_back(15);
    check(b.size()-start==61,"surface record length");save(argv[1],b);auto s=AssetScene::load(argv[1]);check(s.surfaces.size()==1&&s.surfaces[0].material[3]==128&&s.surfaces[0].rgba[1][1]==255&&s.surfaces[0].normals[0].z==1,"original surface properties preserved");
    auto bad=b;bad.pop_back();check(rejects(argv[1],bad),"truncated surface rejected");bad=b;bad.back()=char(255);check(rejects(argv[1],bad),"unknown surface flags rejected");
    bad=b;uint32_t nan=0x7fc00000;for(int i=0;i<4;++i)bad[start+52+i]=char(nan>>(8*i));check(rejects(argv[1],bad),"NaN coefficient rejected");
    bad=b;float high=2;uint32_t bits;std::memcpy(&bits,&high,4);for(int i=0;i<4;++i)bad[start+24+i]=char(bits>>(8*i));check(rejects(argv[1],bad),"unbounded normal rejected");
    std::remove(argv[1]);puts("M3SCENE3 RGBA/alpha/normals/properties and malformed input PASS");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
