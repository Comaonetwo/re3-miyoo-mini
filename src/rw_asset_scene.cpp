#include "rw_asset_scene.hpp"
#include <map>
#include <tuple>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace soft {
namespace {
rw::RGBA rgba(const uint8_t* c){return {c[0],c[1],c[2],c[3]};}
uint32_t packed(const uint8_t* c){return uint32_t(c[0])|(uint32_t(c[1])<<8)|(uint32_t(c[2])<<16)|(uint32_t(c[3])<<24);}
rw::RGBA paintColor(uint32_t c,uint8_t alpha){return {uint8_t(c>>16),uint8_t(c>>8),uint8_t(c),alpha};}
}
struct RwAssetScene::Impl {
    const AssetScene* source;rw::World* world;std::vector<SceneGroup> groups;
    struct Part {rw::Atomic* atomic;rw::Frame* frame;size_t group;bool transparent;Vec3 center;};
    std::vector<Part> parts;std::vector<rw::Raster*> rasters;
    std::map<unsigned,rw::Texture*> textures;std::vector<bool> fractional;
    std::vector<std::pair<rw::Material*,uint8_t>> paintMaterials;
    Impl(const AssetScene& s,rw::World* w):source(&s),world(w),groups(s.groups){}
    ~Impl(){for(auto p:parts){if(world)world->removeAtomic(p.atomic);p.atomic->destroy();p.frame->destroy();}
        for(auto t:textures){t.second->raster=nullptr;t.second->destroy();}for(auto r:rasters)if(r)r->destroy();}
    rw::Texture* texture(const TriangleTexture& m){if(m.texture==65535)return nullptr;
        unsigned u=m.addressU?m.addressU:1,v=m.addressV?m.addressV:1,key=unsigned(m.texture)|(u<<16)|(v<<20);
        auto found=textures.find(key);if(found!=textures.end())return found->second;
        if(!rasters[m.texture]){const auto& t=source->textures[m.texture];auto image=rw::Image::create(t.width,t.height,32);image->allocate();
            for(size_t i=0;i<t.pixels.size();++i){auto c=t.pixels[i];auto p=image->pixels+i*4;p[0]=uint8_t(((c>>11)&31)*255/31);p[1]=uint8_t(((c>>5)&63)*255/63);p[2]=uint8_t((c&31)*255/31);p[3]=t.alpha[i];}
            rasters[m.texture]=rw::Raster::createFromImage(image);image->destroy();if(!rasters[m.texture])throw std::runtime_error("RW asset raster creation failed");}
        auto t=rw::Texture::create(rasters[m.texture]);if(!t)throw std::runtime_error("RW texture allocation failed");
        t->setFilter(rw::Texture::NEAREST);t->setAddressU(rw::Texture::Addressing(u));t->setAddressV(rw::Texture::Addressing(v));textures[key]=t;return t;
    }
    bool transparent(uint32_t i){if(source->surfaces.empty())return false;auto& s=source->surfaces[i];
        if(s.material[3]<255)return true;if(s.flags&1)for(auto& c:s.rgba)if(c[3]<255)return true;
        auto texture=source->materials[i].texture;return texture!=65535&&fractional[texture];}
    void build(){rasters.resize(source->textures.size(),nullptr);fractional.resize(source->textures.size());
        for(size_t i=0;i<source->textures.size();++i)for(auto a:source->textures[i].alpha)if(a>0&&a<255){fractional[i]=true;break;}
        for(size_t group=0;group<groups.size();++group){auto range=groups[group];
            std::map<unsigned,std::vector<uint32_t>> batches;
            for(uint32_t i=range.begin;i<range.begin+range.count;++i){unsigned flags=source->surfaces.empty()?0:source->surfaces[i].flags&15;batches[flags|(unsigned(transparent(i))<<8)].push_back(i);}
            for(auto& batch:batches)for(size_t begin=0;begin<batch.second.size();){
                size_t n=std::min(size_t(21845),batch.second.size()-begin);unsigned bits=batch.first&15;
                unsigned flags=rw::Geometry::POSITIONS|rw::Geometry::TEXTURED;
                if(bits&1)flags|=rw::Geometry::PRELIT;if(bits&2)flags|=rw::Geometry::MODULATE;if(bits&4)flags|=rw::Geometry::NORMALS;if(bits&8)flags|=rw::Geometry::LIGHT;
                auto g=rw::Geometry::create(int(n*3),int(n),flags);if(!g)throw std::runtime_error("RW geometry allocation failed");
                auto a=rw::Atomic::create();auto frame=rw::Frame::create();a->setGeometry(g,0);g->destroy();a->setFrame(frame);if(world)world->addAtomic(a);
                parts.push_back({a,frame,group,bool(batch.first&256),{0,0,0}});
                using Key=std::tuple<uint32_t,uint16_t,uint8_t,uint8_t,float,float,uint8_t>;
                std::map<Key,uint16_t> materials;
                for(size_t i=0;i<n;++i){auto index= batch.second[begin+i];const auto& tri=source->triangles[index];TriangleTexture fallback={{{0,0},{0,0},{0,0}},65535,1,1};
                    const auto& m=source->materials.empty()?fallback:source->materials[index];TriangleSurface s{};
                    if(!source->surfaces.empty())s=source->surfaces[index];else{auto c=tri.color;s.material[0]=uint8_t(((c>>11)&31)*255/31);s.material[1]=uint8_t(((c>>5)&63)*255/63);s.material[2]=uint8_t((c&31)*255/31);s.material[3]=255;s.ambient=s.diffuse=1;}
                    Key key(packed(s.material),m.texture,m.addressU,m.addressV,s.ambient,s.diffuse,s.flags&48);auto found=materials.find(key);uint16_t id;
                    if(found!=materials.end())id=found->second;else{auto mat=rw::Material::create();mat->color=rgba(s.material);mat->surfaceProps={s.ambient,0,s.diffuse};mat->setTexture(texture(m));
                        id=uint16_t(g->matList.appendMaterial(mat));if(s.flags&48)paintMaterials.push_back({mat,uint8_t(s.flags&48)});mat->destroy();materials[key]=id;}
                    auto& rwtri=g->triangles[i];rwtri.matId=id;
                    for(int j=0;j<3;++j){uint16_t v=uint16_t(i*3+j);rwtri.v[j]=v;auto p=tri.v[j];g->morphTargets[0].vertices[v]={p.x,p.y,p.z};g->texCoords[0][v]={m.uv[j].u,m.uv[j].v};
                        if(bits&1)g->colors[v]=rgba(s.rgba[j]);if(bits&4){auto normal=s.normals[j];g->morphTargets[0].normals[v]={normal.x,normal.y,normal.z};}}
                }
                g->calculateBoundingSphere();g->unlock();auto center=g->morphTargets[0].boundingSphere.center;parts.back().center={center.x,center.y,center.z};begin+=n;
            }
        }
        source=nullptr; // All geometry/material/texture data is now owned by librw.
    }
    void gather(const Camera& camera,std::vector<TransparentAtomic>& transparent,const Driving* vehicle){float horizontal=160/camera.focal,vertical=120/camera.focal;
        std::unique_ptr<DrivingPose> pose;if(vehicle)pose.reset(new DrivingPose(*vehicle));
        for(auto p:parts){auto group=groups[p.group];Vec3 center=p.center;
            if(!vehicle){auto point=view(group.center,camera);float r=group.radius;
                if(point.z+r<camera.nearZ||point.z-r>camera.farZ||std::fabs(point.x)>horizontal*point.z+r*std::sqrt(1+horizontal*horizontal)||std::fabs(point.y)>vertical*point.z+r*std::sqrt(1+vertical*vertical))continue;
                rw::Matrix identity;identity.setIdentity();p.frame->transform(&identity,rw::COMBINEREPLACE);
            }else{unsigned wheel=groups.size()==5&&p.group>0?unsigned(p.group-1):4;
                auto origin=pose->transform({0,0,0},wheel),x=pose->transform({1,0,0},wheel),y=pose->transform({0,1,0},wheel),z=pose->transform({0,0,1},wheel);
                rw::Matrix matrix;matrix.setIdentity();matrix.flags=0;matrix.right={x.x-origin.x,x.y-origin.y,x.z-origin.z};matrix.up={y.x-origin.x,y.y-origin.y,y.z-origin.z};matrix.at={z.x-origin.x,z.y-origin.y,z.z-origin.z};matrix.pos={origin.x,origin.y,origin.z};p.frame->transform(&matrix,rw::COMBINEREPLACE);center=pose->transform(center,wheel);}
            if(p.transparent)transparent.push_back({p.atomic,view(center,camera).z});else p.atomic->render();
        }
    }
};
RwAssetScene::RwAssetScene(const AssetScene& s,rw::World* world):impl_(new Impl(s,world)){impl_->build();}
RwAssetScene::~RwAssetScene()=default;
void RwAssetScene::gather(const Camera& c,std::vector<TransparentAtomic>& objects,const Driving* d){impl_->gather(c,objects,d);}
void RwAssetScene::draw(const Camera& c,const Driving* d){std::vector<TransparentAtomic> objects;gather(c,objects,d);renderTransparent(objects);}
void RwAssetScene::paint(uint32_t primary,uint32_t secondary){for(auto p:impl_->paintMaterials)p.first->color=paintColor(p.second&16?primary:secondary,p.first->color.alpha);}
size_t RwAssetScene::atomics() const{return impl_->parts.size();}
void renderTransparent(std::vector<TransparentAtomic>& objects){if(objects.empty())return;
    std::stable_sort(objects.begin(),objects.end(),[](TransparentAtomic a,TransparentAtomic b){return a.distance>b.distance;});
    const int settings[]={rw::ZTESTENABLE,rw::ZWRITEENABLE,rw::VERTEXALPHA,rw::SRCBLEND,rw::DESTBLEND,rw::ALPHATESTFUNC};void* saved[6];for(int i=0;i<6;++i)saved[i]=rw::GetRenderStatePtr(settings[i]);
    rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,0);rw::SetRenderState(rw::VERTEXALPHA,1);rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
    for(auto p:objects)p.atomic->render();for(int i=0;i<6;++i)rw::SetRenderStatePtr(settings[i],saved[i]);
}
}
