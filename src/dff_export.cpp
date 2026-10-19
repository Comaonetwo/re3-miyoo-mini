// Offline reader of the owner's PC DFF files, using the pinned re3 librw.
// No raster/device calls or proprietary game code are required.
#include <rw.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>
#include <string>
static rw::Texture* nameOnlyTexture(const char* name,const char* mask){
    auto* t=rw::Texture::create(nullptr);if(!t)return nullptr;
    std::strncpy(t->name,name,31);t->name[31]=0;
    if(mask){std::strncpy(t->mask,mask,31);t->mask[31]=0;}
    return t;
}
static int nameOffset=-1;
static void* nodeConstruct(void* o,rw::int32 offset,rw::int32){static_cast<char*>(o)[offset]=0;return o;}
static rw::Stream* nodeRead(rw::Stream* stream,rw::int32 length,void* o,rw::int32 offset,rw::int32 size){
    if(length<0||length>=size)return nullptr;
    stream->read8(static_cast<char*>(o)+offset,length);static_cast<char*>(o)[offset+length]=0;return stream;
}
static const char* nodeName(rw::Frame* frame){return frame?reinterpret_cast<const char*>(frame)+nameOffset:"";}
static void dumpNodes(rw::Frame* frame,FILE* out){
    if(!frame)return;auto* m=frame->getLTM();
    std::fprintf(out,"%s %.9g %.9g %.9g",nodeName(frame),m->pos.x,m->pos.y,m->pos.z);
    for(auto v:{m->right,m->up,m->at})std::fprintf(out," %.9g %.9g %.9g",v.x,v.y,v.z);
    std::fprintf(out,"\n");for(auto* child=frame->child;child;child=child->next)dumpNodes(child,out);
}
int main(int argc,char** argv) {
    bool textured=false,vehicle=false,localOnly=false,rich=false;const char* atomicName=nullptr;
    if(argc<3)return 2;
    for(int i=3;i<argc;++i){if(std::strcmp(argv[i],"--textures")==0)textured=true;
        else if(std::strcmp(argv[i],"--rich")==0){rich=true;textured=true;}
        else if(std::strcmp(argv[i],"--vehicle")==0)vehicle=true;
        else if(std::strcmp(argv[i],"--local")==0)localOnly=true;
        else if(std::strcmp(argv[i],"--atomic")==0&&i+1<argc)atomicName=argv[++i];else return 2;}
    if(!rw::Engine::init())return 1;
    rw::registerMeshPlugin();rw::registerNativeDataPlugin();
    nameOffset=rw::Frame::registerPlugin(128,0x0253f2fe,nodeConstruct,nullptr,nullptr);
    if(nameOffset<0)return 1;
    rw::Frame::registerPluginStream(0x0253f2fe,nodeRead,nullptr,nullptr);
    if(!rw::Engine::open(nullptr)||!rw::Engine::start())return 1;
    rw::Texture::setLoadTextures(textured);rw::Texture::setCreateDummies(false);
    if(textured)rw::Texture::readCB=nameOnlyTexture;
    rw::StreamFile in;
    if(!in.open(argv[1],"rb")||!rw::findChunk(&in,rw::ID_CLUMP,nullptr,nullptr))return 1;
    auto* clump=rw::Clump::streamRead(&in);in.close();if(!clump)return 1;
    std::vector<rw::Atomic*> selected;rw::Atomic* best=nullptr;
    FORLIST(link,clump->atomics){auto* a=rw::Atomic::fromClump(link);if(!a->geometry)continue;
        const char* name=nodeName(a->getFrame());
        if(atomicName){if(std::strcmp(name,atomicName)==0)selected.push_back(a);}
        else if(vehicle){bool hidden=false;
            for(auto* f=a->getFrame();f;f=f->getParent()){const char* n=nodeName(f);
                if(std::strstr(n,"_dam")||std::strstr(n,"_lo")||std::strstr(n,"_vlo")||std::strncmp(n,"extra",5)==0)hidden=true;}
            if(!hidden)selected.push_back(a);
        }else if(!best||a->geometry->numTriangles>best->geometry->numTriangles)best=a;
    }
    if(best)selected.push_back(best);if(selected.empty())return 1;
    FILE* out=std::fopen(argv[2],"w");if(!out)return 1;
    std::string nodesPath=std::string(argv[2])+".nodes";FILE* nodes=std::fopen(nodesPath.c_str(),"w");if(!nodes)return 1;
    dumpNodes(clump->getFrame(),nodes);if(std::fclose(nodes)!=0)return 1;
    for(auto* selectedAtomic:selected){auto* g=selectedAtomic->geometry;
    if((g->flags&rw::Geometry::NATIVE)||g->numMorphTargets!=1||!g->morphTargets[0].vertices||!g->triangles)return 1;
    for(int i=0;i<g->numTriangles;++i) {
        auto t=g->triangles[i];
        if(t.matId>=g->matList.numMaterials)return 1;
        for(int j=0;j<3;++j)if(t.v[j]>=g->numVertices)return 1;
        auto* m=g->matList.materials[t.matId];
        if(rich){
            rw::Matrix identity;identity.setIdentity();auto matrix=!localOnly&&selectedAtomic->getFrame()?selectedAtomic->getFrame()->getLTM():&identity;
            rw::Matrix inverse;if(!rw::Matrix::invert(&inverse,matrix))return 1;
            std::fprintf(out,"{\"p\":[");
            for(int j=0;j<3;++j){rw::V3d point;rw::V3d::transformPoints(&point,&g->morphTargets[0].vertices[t.v[j]],1,matrix);
                if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))return 1;
                std::fprintf(out,"%s[%.9g,%.9g,%.9g]",j?",":"",point.x,point.y,point.z);}
            std::fprintf(out,"],\"uv\":[");
            for(int j=0;j<3;++j){rw::TexCoords uv{0,0};if(g->numTexCoordSets&&g->texCoords[0])uv=g->texCoords[0][t.v[j]];
                std::fprintf(out,"%s[%.9g,%.9g]",j?",":"",uv.u,uv.v);}
            std::fprintf(out,"],\"rgba\":[");
            for(int j=0;j<3;++j){auto color=(g->flags&rw::Geometry::PRELIT)&&g->colors?g->colors[t.v[j]]:rw::RGBA{255,255,255,255};
                std::fprintf(out,"%s[%u,%u,%u,%u]",j?",":"",color.red,color.green,color.blue,color.alpha);}
            std::fprintf(out,"],\"normals\":[");
            for(int j=0;j<3;++j){rw::V3d normal{0,0,0};if((g->flags&rw::Geometry::NORMALS)&&g->morphTargets[0].normals){auto n=g->morphTargets[0].normals[t.v[j]];
                normal={rw::dot(inverse.right,n),rw::dot(inverse.up,n),rw::dot(inverse.at,n)};float len=rw::length(normal);if(len>1e-8f)normal=rw::scale(normal,1/len);}
                std::fprintf(out,"%s[%.9g,%.9g,%.9g]",j?",":"",normal.x,normal.y,normal.z);}
            // Names from the owner's TXD are ASCII identifiers. Reject rather
            // than emit malformed JSON for an unexpected quote/control byte.
            const char* name=m->texture&&m->texture->name[0]?m->texture->name:"-";
            for(const char* c=name;*c;++c)if(*c<32||*c=='"'||*c=='\\')return 1;
            unsigned flags=0;if(g->flags&rw::Geometry::PRELIT)flags|=1;if(g->flags&rw::Geometry::MODULATE)flags|=2;if(g->flags&rw::Geometry::NORMALS)flags|=4;if(g->flags&rw::Geometry::LIGHT)flags|=8;
            std::fprintf(out,"],\"material\":[%u,%u,%u,%u],\"ambient\":%.9g,\"diffuse\":%.9g,\"flags\":%u,\"texture\":\"%s\",\"address\":%u}\n",
                m->color.red,m->color.green,m->color.blue,m->color.alpha,m->surfaceProps.ambient,m->surfaceProps.diffuse,flags,name,m->texture?m->texture->filterAddressing:0);
            continue;
        }
        // Legacy output intentionally retains its previous cutout behaviour.
        if(m->color.alpha<128)continue;
        unsigned r=m->color.red,b=m->color.blue,green=m->color.green;
        // Replace GTA vehicle primary/secondary paint markers with a fixed blue paint.
        if(vehicle&&((r==60&&green==255&&b==0)||(r==255&&green==0&&b==175))){r=45;green=105;b=190;}
        if((g->flags&rw::Geometry::PRELIT)&&g->colors) {
            unsigned sr=0,sg=0,sb=0;
            for(int j=0;j<3;++j){auto c=g->colors[t.v[j]];sr+=c.red;sg+=c.green;sb+=c.blue;}
            if(g->flags&rw::Geometry::MODULATE){r=r*sr/765;green=green*sg/765;b=b*sb/765;}
            else {r=sr/3;green=sg/3;b=sb/3;}
        }
        rw::V3d local[3],world[3];
        for(int j=0;j<3;++j)local[j]=g->morphTargets[0].vertices[t.v[j]];
        if(!localOnly&&selectedAtomic->getFrame())rw::V3d::transformPoints(world,local,3,selectedAtomic->getFrame()->getLTM());
        else for(int j=0;j<3;++j)world[j]=local[j];
        for(auto p:world){if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))return 1;
            std::fprintf(out,"%.9g %.9g %.9g ",p.x,p.y,p.z);}
        std::fprintf(out,"%u %u %u",r,green,b);
        if(textured){
            for(int j=0;j<3;++j){rw::TexCoords uv{0,0};if(g->numTexCoordSets&&g->texCoords[0])uv=g->texCoords[0][t.v[j]];
                if(!std::isfinite(uv.u)||!std::isfinite(uv.v))return 1;
                std::fprintf(out," %.9g %.9g",uv.u,uv.v);}
            std::fprintf(out," %s %u",m->texture&&m->texture->name[0]?m->texture->name:"-",m->texture?m->texture->filterAddressing:0);
        }
        std::fprintf(out,"\n");
    }
    }
    bool ok=std::fclose(out)==0;clump->destroy();
    rw::Engine::stop();rw::Engine::close();rw::Engine::term();return ok?0:1;
}
