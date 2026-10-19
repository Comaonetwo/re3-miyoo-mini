#include "asset_scene.hpp"
#include <fstream>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <cmath>
static void check(bool b){if(!b)throw std::runtime_error("asset scene test failed");}
static void u32(std::string& s,uint32_t n){for(int i=0;i<4;++i)s.push_back(char(n>>(8*i)));}
static void f32(std::string& s,float f){uint32_t n;std::memcpy(&n,&f,4);u32(s,n);}
static void save(const char* path,const std::string& s){std::ofstream f(path,std::ios::binary);f.write(s.data(),s.size());check(bool(f));}
static bool rejected(const char* path,const std::string& s){save(path,s);try{soft::AssetScene::load(path);}catch(const std::exception&){return true;}return false;}
int main(int argc,char** argv){if(argc!=2)return 2;const char* path=argv[1];
 try {
    std::string bytes="M3SCENE1";u32(bytes,1);
    for(float f:{0.f,0.f,0.f,0.f})f32(bytes,f);
    u32(bytes,1);u32(bytes,0);u32(bytes,1);
    for(float f:{0.f,0.f,10.f,2.f})f32(bytes,f);
    for(float f:{-1.f,-1.f,10.f,1.f,-1.f,10.f,0.f,1.f,10.f})f32(bytes,f);
    bytes.push_back(char(0xff));bytes.push_back(char(0xff));save(path,bytes);
    auto scene=soft::AssetScene::load(path);soft::Renderer direct,culled;
    soft::renderScene(direct,scene.triangles,scene.spawn);scene.render(culled,scene.spawn);check(direct.pixels()==culled.pixels());
    scene.spawn.yaw=3.14159265f;scene.render(culled,scene.spawn);check(culled.stats().submitted==0);
    for(size_t n=0;n<bytes.size();++n)check(rejected(path,bytes.substr(0,n)));
    check(rejected(path,bytes+"x"));auto bad=bytes;bad[0]='X';check(rejected(path,bad));
    bad=bytes;bad[8]=0;check(rejected(path,bad));
    bad=bytes;bad[32]=1;check(rejected(path,bad)); // group starts beyond zero
    bad=bytes;bad[52]=bad[53]=bad[54]=bad[55]=0;check(rejected(path,bad)); // sphere excludes vertices
    bad=bytes;bad[56]=0;bad[57]=0;bad[58]=char(0xc0);bad[59]=char(0x7f);check(rejected(path,bad)); // NaN
    auto textured=bytes.substr(0,32);textured[7]='2';u32(textured,1);u32(textured,2);u32(textured,2);
    for(int i=0;i<4;++i){textured.push_back(char(0xff));textured.push_back(char(0xff));textured.push_back(char(0xff));}
    textured+=bytes.substr(32);for(int i=0;i<6;++i)f32(textured,.5f);
    textured.push_back(0);textured.push_back(0);textured.push_back(1);textured.push_back(1);
    save(path,textured);auto texturedScene=soft::AssetScene::load(path);texturedScene.render(culled,texturedScene.spawn);
    scene.spawn.yaw=0;soft::renderScene(direct,scene.triangles,scene.spawn);check(culled.pixels()==direct.pixels());
    for(size_t n=0;n<textured.size();++n)check(rejected(path,textured.substr(0,n)));
    bad=textured;bad[36]=char(129);check(rejected(path,bad)); // invalid texture width
    bad=textured;bad[bad.size()-4]=1;check(rejected(path,bad)); // out-of-range texture reference
    bad=textured;bad[bad.size()-1]=5;check(rejected(path,bad)); // invalid address mode
    std::remove(path);std::puts("asset scene validation and culling PASS");return 0;
 }catch(const std::exception& e){std::remove(path);std::fprintf(stderr,"%s\n",e.what());return 1;}
}
