#include "asset_scene.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <cmath>
namespace soft {
namespace {
struct Reader {
    std::ifstream file;
    explicit Reader(const std::string& path):file(path,std::ios::binary) {if(!file)throw std::runtime_error("Cannot open scene: "+path);}
    void read(void* p,size_t n){if(!file.read(static_cast<char*>(p),n))throw std::runtime_error("Truncated scene");}
    uint16_t u16(){uint8_t b[2];read(b,2);return uint16_t(b[0])|(uint16_t(b[1])<<8);}
    uint32_t u32(){uint8_t b[4];read(b,4);return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
    float number(){uint32_t bits=u32();float f;std::memcpy(&f,&bits,4);if(!std::isfinite(f)||std::fabs(f)>100000)throw std::runtime_error("Invalid scene coordinate");return f;}
    Vec3 vec(){return {number(),number(),number()};}
};
}
AssetScene AssetScene::load(const std::string& path) {
    Reader in(path);char magic[8];in.read(magic,8);
    bool rich=std::memcmp(magic,"M3SCENE3",8)==0;
    bool textured=rich||std::memcmp(magic,"M3SCENE2",8)==0;
    if(!textured&&std::memcmp(magic,"M3SCENE1",8))throw std::runtime_error("Unsupported scene format");
    uint32_t count=in.u32();if(!count||count>100000)throw std::runtime_error("Scene triangle budget exceeded");
    AssetScene scene;scene.spawn.position=in.vec();scene.spawn.yaw=in.number();scene.spawn.farZ=160;
    uint32_t groups=in.u32();if(!groups||groups>4096)throw std::runtime_error("Scene group budget exceeded");
    if(textured){
        uint32_t textureCount=in.u32();if(textureCount>512)throw std::runtime_error("Scene texture budget exceeded");
        size_t pixels=0;
        for(uint32_t i=0;i<textureCount;++i){uint32_t w=in.u32(),h=in.u32();
            if(!w||!h||w>128||h>128||(w&(w-1))||(h&(h-1)))throw std::runtime_error("Invalid texture dimensions");
            pixels+=size_t(w)*h;if(pixels>8*1024*1024/3)throw std::runtime_error("Scene texture memory budget exceeded");
            Texture tex;tex.width=uint16_t(w);tex.height=uint16_t(h);tex.pixels.resize(size_t(w)*h);tex.alpha.resize(tex.pixels.size());
            for(size_t j=0;j<tex.pixels.size();++j){tex.pixels[j]=in.u16();in.read(&tex.alpha[j],1);}
            scene.textures.push_back(std::move(tex));
        }
        scene.materials.resize(count);
    }
    uint32_t expected=0;
    for(uint32_t i=0;i<groups;++i) {
        SceneGroup group;group.begin=in.u32();group.count=in.u32();group.center=in.vec();group.radius=in.number();
        if(group.begin!=expected||!group.count||group.count>count-expected||group.radius<0)throw std::runtime_error("Invalid scene group range");
        expected+=group.count;scene.groups.push_back(group);
    }
    if(expected!=count)throw std::runtime_error("Incomplete scene groups");
    scene.triangles.resize(count);if(rich)scene.surfaces.resize(count);
    for(uint32_t i=0;i<count;++i){auto& tri=scene.triangles[i];for(auto& v:tri.v)v=in.vec();tri.color=in.u16();
        if(textured){auto& m=scene.materials[i];for(auto& uv:m.uv)uv={in.number(),in.number()};
            m.texture=in.u16();in.read(&m.addressU,1);in.read(&m.addressV,1);
            if((m.texture!=65535&&m.texture>=scene.textures.size())||m.addressU>4||m.addressV>4)
                throw std::runtime_error("Invalid scene texture reference/addressing");
        }
        if(rich){auto& s=scene.surfaces[i];in.read(s.rgba,12);in.read(s.material,4);
            for(auto& normal:s.normals){normal=in.vec();float length=normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;if(length>1.021f)throw std::runtime_error("Invalid surface normal");}
            s.ambient=in.number();s.diffuse=in.number();in.read(&s.flags,1);
            if((s.flags&~63)||s.ambient<0||s.ambient>4||s.diffuse<0||s.diffuse>4)throw std::runtime_error("Invalid surface properties");
        }
    }
    if(in.file.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing scene data");
    // The culling sphere must enclose every vertex; don't trust the asset header.
    for(auto g:scene.groups)for(uint32_t i=g.begin;i<g.begin+g.count;++i)for(auto v:scene.triangles[i].v) {
        double x=v.x-g.center.x,y=v.y-g.center.y,z=v.z-g.center.z;
        if(x*x+y*y+z*z>double(g.radius+.02f)*(g.radius+.02f))throw std::runtime_error("Invalid scene bounding sphere");
    }
    return scene;
}
void AssetScene::discardConvertedData(){std::vector<Triangle>().swap(triangles);std::vector<TriangleTexture>().swap(materials);std::vector<TriangleSurface>().swap(surfaces);std::vector<Texture>().swap(textures);}
void AssetScene::render(Renderer& r,const Camera& c,bool textured) const {
    r.clear(rgb(114,157,187));draw(r,c,textured);
}
void AssetScene::draw(Renderer& r,const Camera& c,bool textured) const {
    if(triangles.empty())return;
    const float horizontal=160/c.focal,vertical=120/c.focal;
    for(auto g:groups) {
        auto p=view(g.center,c);float radius=g.radius;
        if(p.z+radius<c.nearZ||p.z-radius>c.farZ||
           std::fabs(p.x)>horizontal*p.z+radius*std::sqrt(1+horizontal*horizontal)||
           std::fabs(p.y)>vertical*p.z+radius*std::sqrt(1+vertical*vertical))continue;
        for(uint32_t i=g.begin;i<g.begin+g.count;++i){
            if(textured&&!materials.empty()&&materials[i].texture!=65535){
                auto& t=triangles[i];auto& m=materials[i];Vec3 v[3];for(int j=0;j<3;++j)v[j]=view(t.v[j],c);
                r.texturedTriangle(v,m.uv,t.color,textures[m.texture],m.addressU,m.addressV,c);
            }else r.worldTriangle(triangles[i],c);
        }
    }
}
}
