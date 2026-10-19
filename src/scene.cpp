#include "soft_renderer.hpp"
namespace soft {
static void quad(std::vector<Triangle>& s,Vec3 a,Vec3 b,Vec3 c,Vec3 d,uint16_t color) {
    s.push_back({{a,b,c},color}); s.push_back({{a,c,d},color});
}
static void box(std::vector<Triangle>& s,float x,float y,float z,float w,float h,float d,unsigned r,unsigned g,unsigned b) {
    Vec3 a{x,y,z},bb{x+w,y,z},c{x+w,y+h,z},dd{x,y+h,z};
    Vec3 e{x,y,z+d},f{x+w,y,z+d},gg{x+w,y+h,z+d},hh{x,y+h,z+d};
    quad(s,a,bb,c,dd,rgb(r,g,b)); quad(s,bb,f,gg,c,rgb(r*7/10,g*7/10,b*7/10));
    quad(s,f,e,hh,gg,rgb(r,g,b)); quad(s,e,a,dd,hh,rgb(r*8/10,g*8/10,b*8/10));
    quad(s,dd,c,gg,hh,rgb(r*9/10,g*9/10,b*9/10));
}
std::vector<Triangle> makeScene() {
    std::vector<Triangle> s;
    quad(s,{-35,0,-15},{35,0,-15},{35,0,90},{-35,0,90},rgb(68,95,70));
    quad(s,{-3,.01f,-15},{3,.01f,-15},{3,.01f,90},{-3,.01f,90},rgb(55,59,65));
    for(int z=-12;z<85;z+=5) quad(s,{-.06f,.02f,float(z)},{.06f,.02f,float(z)},
        {.06f,.02f,float(z+2)},{-.06f,.02f,float(z+2)},rgb(230,207,142));
    for(int z=0;z<80;z+=8) for(int side:{-1,1}) {
        float x=side<0?-12.f:5.f;
        box(s,x,0,float(z),7,4+(z%17)*.45f,6,100+z%45,105+z%60,125+z%70);
    }
    box(s,-1,0,4,2,.8f,3,190,55,35); box(s,-.8f,.8f,4.6f,1.6f,.65f,1.5f,65,91,112);
    box(s,-1.12f,0,4.4f,.24f,.45f,.6f,25,25,28); box(s,.88f,0,4.4f,.24f,.45f,.6f,25,25,28);
    box(s,-1.12f,0,6.1f,.24f,.45f,.6f,25,25,28); box(s,.88f,0,6.1f,.24f,.45f,.6f,25,25,28);
    return s;
}
void renderScene(Renderer& r,const std::vector<Triangle>& s,const Camera& c) {
    r.clear(rgb(114,157,187)); for(const auto& t:s) r.worldTriangle(t,c);
}
}
