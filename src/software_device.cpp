#include "detail_policy.hpp"
#include "software_device.hpp"
#include "model_clusters.hpp"
#include "world_simplifier.hpp"
#include "world_lod_policy.hpp"
#include <chrono>
#include "additive_bounds.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <array>
#include <cstdio>
namespace soft {
struct SoftwareDevice::Impl {
    struct RasterData {std::vector<uint8_t> bytes;Texture texture;bool dirty=true,locked=false,alpha=false,traced=false;};
    struct MaterialDraw {bool ready=false,valid=false;Texture* texture=nullptr;uint8_t addressU=3,addressV=3;};
    std::vector<MaterialDraw> materialDraws;bool prepareAtomics=true,preparedScreen=true,groupedColors=false;uint64_t materialPrepared=0,materialReused=0,flatProjectionReused=0;
    struct GeometryCache {std::vector<rw::RGBA> vertexColors;std::vector<uint8_t> vertexColorReady;size_t vertexColorCharge=0;bool vertexColorValid=false,vertexColorPrimed=false;unsigned vertexColorChanges=0;const rw::RGBA* vertexColorSource=nullptr;float vertexColorKey[9]={};rw::Geometry* simplified=nullptr;size_t simplifiedCharge=0;bool simplifyAttempted=false;bool simplifiedIntermediate=false;std::vector<ModelBounds> changedBounds;Vec3 simplifyCamera{0,0,0};bool hasSimplifyCamera=false;float simplifiedError=0;const rw::RGBA* opacityColors=nullptr;bool opacityReady=false,opaqueVertices=false,opacityPrelit=false;std::vector<Vec3> unitNormals;std::vector<uint8_t> normalReady;float normalMatrix[9]={};const rw::V3d* normalSource=nullptr;size_t normalCharge=0;ModelBounds whole;std::vector<ModelBounds> bounds;int vertices=0,triangles=0;rw::V3d* positions=nullptr;rw::Triangle* indices=nullptr;size_t charge=0;unsigned mutationStreak=0;bool dynamic=false;};
    static Impl* active;static int offset,geometryOffset;
    static constexpr unsigned clusterSize=32;
    size_t simplifiedBytes=0;uint64_t simplifiedBuilds=0;unsigned simplifyBudget=1;double simplifyPeakMs=0;
    static constexpr size_t simplifiedLimit=1024*1024;
    static constexpr size_t cacheLimit=1024*1024;
    bool directAtomicEnabled=false;uint64_t directVertexCount=0;
    bool earlyWideEnabled=false,earlyVisibilityEnabled=false;uint64_t earlyQueries=0,earlyHidden=0;
    bool flatFadingEnabled=false;uint64_t flattenedFading=0;
    bool farSurface=false,farNormal=false;uint64_t farNormals=0;
    bool distanceShadingEnabled=false,distanceShadingAtomic=false;uint64_t distanceShaded=0;
    bool ambientShortcutEnabled=true;uint64_t ambientSkipped=0;
    bool whitePrelightEnabled=false;uint64_t whitePrelightCount=0;bool vertexColorCacheEnabled=false;size_t vertexColorBytes=0;uint64_t cachedColors=0,preparedColors=0;static constexpr size_t vertexColorLimit=256*1024;
    bool filteredEffects=false,preservePrelight=false,textureAlphaEnabled=true,transferEnabled=false,traceTextures=false;uint64_t transferredVertices=0,textureAlphaTriangles=0;bool normalCacheEnabled=false;size_t normalCacheBytes=0;uint64_t normalsReused=0,normalsPrepared=0;static constexpr size_t normalCacheLimit=512*1024;
    bool hierarchyEnabled=false,modelInside=false;uint64_t modelTests=0,modelCulls=0,insideModels=0,planeTestsSaved=0;
    bool fastClusterBoundsEnabled=false;rw::Geometry* (*worldLodSelector)(rw::Atomic*)=nullptr;uint64_t worldLodCalls=0,worldLodSaved=0;bool clusterEnabled=false;size_t cacheBytes=0;uint64_t clusterTests=0,clusterCulls=0,clusterTriangles=0,verticesSkipped=0,cacheRebuilds=0,dynamicFallbacks=0;
    std::vector<uint8_t> selectedVertices;
    Renderer& renderer;Camera projection;rw::Matrix inverse;
    rw::Camera* cam=nullptr;rw::Frame* frame=nullptr;rw::Raster* image=nullptr;rw::Raster* depth=nullptr;
    rw::Raster* overlays[3]={};rw::ObjPipeline* pipeline=nullptr;
    void* states[rw::GSALPHATESTREF+1]={};std::vector<rw::cpu::Im3DVertex> transformed,atomicScratch;
    bool cameraWarning=false;bool updating=false,worldTextured=true,quality=true,gameCompat=false;uint64_t failures=0,triangles2=0,triangles3=0,atomics=0,shaded=0,matFx=0,skins=0,skinVertices=0;
    std::vector<rw::cpu::Im3DVertex> atomicVertices;std::vector<uint8_t> clipCodes,colorReady;std::vector<Vec2> projected;std::vector<Vec3> screenVertices;std::vector<uint8_t> projectionReady;uint64_t earlyCulls=0,backfaceCulls=0,colorReused=0,groupedColorReused=0;
    std::vector<rw::V3d> skinPositions,skinNormals;std::array<rw::Matrix,256> boneMatrices;
    std::vector<Vec3> directLight;std::vector<int> drawOrder;std::array<bool,1024> warned{};bool warnedBlend=false;
    rw::World* world=nullptr;rw::Light* ambient=nullptr;rw::Light* sun=nullptr;rw::Frame* sunFrame=nullptr;
    explicit Impl(Renderer& target):renderer(target){
        if(active||rw::Engine::state!=rw::Engine::Dead)throw std::runtime_error("SoftwareDevice requires its own librw engine");
        active=this;
    }
    static GeometryCache* geometryCache(rw::Geometry* g){GeometryCache* cache=nullptr;if(g&&geometryOffset>=0)std::memcpy(&cache,reinterpret_cast<char*>(g)+geometryOffset,sizeof(cache));return cache;}
    static void setGeometryCache(void* object,GeometryCache* cache){std::memcpy(reinterpret_cast<char*>(object)+geometryOffset,&cache,sizeof(cache));}
    static void* geometryCtor(void* object,int,int){setGeometryCache(object,nullptr);return object;}
    static void* geometryDtor(void* object,int,int){auto cache=geometryCache(static_cast<rw::Geometry*>(object));if(cache){if(active){active->cacheBytes-=cache->charge;active->normalCacheBytes-=cache->normalCharge;active->vertexColorBytes-=cache->vertexColorCharge;}if(cache->simplified){if(active)active->simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();}delete cache;}setGeometryCache(object,nullptr);return object;}
    static void* geometryCopy(void* destination,void*,int,int){setGeometryCache(destination,nullptr);return destination;}
    static GeometryCache* boundsFor(rw::Geometry* g){auto& self=*active;auto cache=geometryCache(g);
        const unsigned dirty=rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKPOLYGONS;
        if(cache&&cache->dynamic){++self.dynamicFallbacks;return nullptr;}
        if(cache&&cache->vertices==g->numVertices&&cache->triangles==g->numTriangles&&cache->positions==g->morphTargets[0].vertices&&cache->indices==g->triangles&&!(g->lockedSinceInst&dirty)){cache->mutationStreak=0;return cache;}
        unsigned streak=cache?cache->mutationStreak+1:0;
        // Wavy water and other continuously modified meshes should not pay to
        // rebuild static bounds on each draw. Keep a tiny ownership sentinel
        // and use the original geometry path after repeated consecutive edits.
        if(cache&&streak>=3){self.cacheBytes-=cache->charge-sizeof(GeometryCache);cache->charge=sizeof(GeometryCache);
            std::vector<ModelBounds>().swap(cache->bounds);self.normalCacheBytes-=cache->normalCharge;self.vertexColorBytes-=cache->vertexColorCharge;cache->normalCharge=0;cache->vertexColorCharge=0;std::vector<rw::RGBA>().swap(cache->vertexColors);std::vector<uint8_t>().swap(cache->vertexColorReady);cache->vertexColorValid=false;cache->vertexColorPrimed=false;std::vector<Vec3>().swap(cache->unitNormals);std::vector<uint8_t>().swap(cache->normalReady);if(cache->simplified){self.simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();cache->simplified=nullptr;cache->simplifiedCharge=0;std::vector<ModelBounds>().swap(cache->changedBounds);}cache->dynamic=true;++self.dynamicFallbacks;return nullptr;}
        if(cache){self.cacheBytes-=cache->charge;self.normalCacheBytes-=cache->normalCharge;self.vertexColorBytes-=cache->vertexColorCharge;if(cache->simplified){self.simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();}delete cache;cache=nullptr;setGeometryCache(g,nullptr);}
        size_t count=(g->numTriangles+clusterSize-1)/clusterSize,charge=sizeof(GeometryCache)+count*sizeof(ModelBounds);
        if(charge>cacheLimit-self.cacheBytes)return nullptr;
        try{cache=new GeometryCache;cache->bounds.resize(count);}catch(const std::bad_alloc&){delete cache;return nullptr;}
        cache->mutationStreak=streak;cache->vertices=g->numVertices;cache->triangles=g->numTriangles;cache->positions=g->morphTargets[0].vertices;cache->indices=g->triangles;
        for(unsigned k=0;k<count;++k){auto& box=cache->bounds[k];bool invalid=false;
            for(unsigned i=k*clusterSize;i<std::min(unsigned(g->numTriangles),(k+1)*clusterSize);++i)for(unsigned j=0;j<3;++j){unsigned index=g->triangles[i].v[j];
                if(index>=unsigned(g->numVertices)){invalid=true;continue;}auto p=g->morphTargets[0].vertices[index];
                if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)){invalid=true;continue;}box.include({p.x,p.y,p.z});}
            if(invalid)box.valid=false;
        }
        bool allValid=true;for(const auto& box:cache->bounds){if(!box.valid){allValid=false;break;}cache->whole.include(box.minimum);cache->whole.include(box.maximum);}if(!allValid)cache->whole.valid=false;
        cache->charge=sizeof(GeometryCache)+cache->bounds.capacity()*sizeof(ModelBounds);
        if(cache->charge>cacheLimit-self.cacheBytes){delete cache;return nullptr;}
        self.cacheBytes+=cache->charge;++self.cacheRebuilds;setGeometryCache(g,cache);
        // Same ownership as a GPU instancer: mutations via Geometry::lock set
        // the relevant bits again. Other dirty attributes are left untouched.
        g->lockedSinceInst&=~dirty;return cache;
    }
    static GeometryCache* normalsFor(rw::Geometry* g,const rw::V3d* source,const rw::Matrix& inverse){
        auto& self=*active;auto cache=geometryCache(g);if(!self.normalCacheEnabled||!cache||cache->dynamic||cache->vertices!=g->numVertices||cache->positions!=g->morphTargets[0].vertices||(g->lockedSinceInst&(rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKPOLYGONS)))return nullptr;
        float matrix[9]={inverse.right.x,inverse.right.y,inverse.right.z,inverse.up.x,inverse.up.y,inverse.up.z,inverse.at.x,inverse.at.y,inverse.at.z};
        if(cache->normalCharge){
            if(cache->normalSource!=source||g->lockedSinceInst&rw::Geometry::LOCKNORMALS||std::memcmp(matrix,cache->normalMatrix,sizeof matrix))std::fill(cache->normalReady.begin(),cache->normalReady.end(),0);
        }else{
            if(size_t(g->numVertices)*(sizeof(Vec3)+1)>normalCacheLimit-self.normalCacheBytes)return nullptr;
            try{cache->unitNormals.resize(g->numVertices);cache->normalReady.resize(g->numVertices);}catch(const std::bad_alloc&){std::vector<Vec3>().swap(cache->unitNormals);std::vector<uint8_t>().swap(cache->normalReady);return nullptr;}
            size_t charge=cache->unitNormals.capacity()*sizeof(Vec3)+cache->normalReady.capacity();
            if(charge>normalCacheLimit-self.normalCacheBytes){std::vector<Vec3>().swap(cache->unitNormals);std::vector<uint8_t>().swap(cache->normalReady);return nullptr;}
            cache->normalCharge=charge;self.normalCacheBytes+=charge;
        }
        std::memcpy(cache->normalMatrix,matrix,sizeof matrix);cache->normalSource=source;g->lockedSinceInst&=~rw::Geometry::LOCKNORMALS;return cache;
    }
    static GeometryCache* colorsFor(rw::Geometry* g,const rw::Material* material,bool lit,const rw::RGBAf& ambient){
        auto& self=*active;if(!self.vertexColorCacheEnabled||g->numVertices>8192||!(g->flags&rw::Geometry::PRELIT)||!g->colors)return nullptr;
        if(lit&&(!std::isfinite(material->surfaceProps.ambient)||!std::isfinite(material->surfaceProps.diffuse)||!std::isfinite(ambient.red+ambient.green+ambient.blue)))return nullptr;
        auto cache=boundsFor(g);if(!cache||cache->dynamic)return nullptr;
        rw::RGBA tint=(g->flags&rw::Geometry::MODULATE)?material->color:rw::RGBA{255,255,255,255};
        float key[9]={float(lit),float(tint.red),float(tint.green),float(tint.blue),lit?ambient.red:0,lit?ambient.green:0,lit?ambient.blue:0,lit?material->surfaceProps.ambient:0,lit?material->surfaceProps.diffuse:0};
        bool sourceChanged=g->lockedSinceInst&rw::Geometry::LOCKPRELIGHT;
        if(sourceChanged)cache->opacityReady=false;
        g->lockedSinceInst&=~rw::Geometry::LOCKPRELIGHT;
        if(!cache->vertexColorPrimed||sourceChanged||cache->vertexColorSource!=g->colors||std::memcmp(key,cache->vertexColorKey,sizeof key)){
            cache->vertexColorValid=false;cache->vertexColorPrimed=true;cache->vertexColorSource=g->colors;std::memcpy(cache->vertexColorKey,key,sizeof key);
            // First observe an unchanged recipe twice. Rapidly changing lighting
            // uses the original path and relinquishes scarce cache space.
            if(++cache->vertexColorChanges>=3&&cache->vertexColorCharge){self.vertexColorBytes-=cache->vertexColorCharge;cache->vertexColorCharge=0;std::vector<rw::RGBA>().swap(cache->vertexColors);std::vector<uint8_t>().swap(cache->vertexColorReady);}
            return nullptr;
        }
        cache->vertexColorChanges=0;
        if(!cache->vertexColorCharge){
            if(size_t(g->numVertices)*5>vertexColorLimit-self.vertexColorBytes)return nullptr;
            try{cache->vertexColors.resize(g->numVertices);cache->vertexColorReady.resize(g->numVertices);}catch(const std::bad_alloc&){std::vector<rw::RGBA>().swap(cache->vertexColors);std::vector<uint8_t>().swap(cache->vertexColorReady);return nullptr;}
            size_t charge=cache->vertexColors.capacity()*sizeof(rw::RGBA)+cache->vertexColorReady.capacity();
            if(charge>vertexColorLimit-self.vertexColorBytes){std::vector<rw::RGBA>().swap(cache->vertexColors);std::vector<uint8_t>().swap(cache->vertexColorReady);return nullptr;}
            cache->vertexColorCharge=charge;self.vertexColorBytes+=charge;
        }
        if(!cache->vertexColorValid){std::fill(cache->vertexColorReady.begin(),cache->vertexColorReady.end(),0);cache->vertexColorValid=true;}
        return cache;
    }
    static bool selectClusters(rw::Atomic* a,rw::Geometry* g){auto& self=*active;self.drawOrder.clear();self.selectedVertices.clear();self.modelInside=false;
        if(!self.clusterEnabled||g->numVertices<64||g->numTriangles<64)return false;
        auto cache=boundsFor(g);if(!cache)return false;
        rw::Matrix modelView=self.inverse;if(a->getFrame()){auto model=*a->getFrame()->getLTM(),view=self.inverse;model.flags&=~rw::Matrix::IDENTITY;view.flags&=~rw::Matrix::IDENTITY;rw::Matrix::mult(&modelView,&model,&view);}
        float matrix[12]={modelView.right.x,modelView.up.x,modelView.at.x,modelView.pos.x,modelView.right.y,modelView.up.y,modelView.at.y,modelView.pos.y,modelView.right.z,modelView.up.z,modelView.at.z,modelView.pos.z};
        ModelFrustum frustum(matrix,120/rw::engine->currentCamera->viewWindow.y/self.projection.focal,self.projection);
        bool fastBounds=self.fastClusterBoundsEnabled&&!self.hierarchyEnabled&&cache->bounds.size()>=4;
        unsigned mask=63;double margin=0;if(fastBounds){++self.modelTests;margin=frustum.boundMargin(cache->whole);mask=frustum.activePlanes(cache->whole,margin);if(mask&128){++self.modelCulls;self.clusterCulls+=cache->bounds.size();self.clusterTriangles+=g->numTriangles;self.earlyCulls+=g->numTriangles;self.triangles3+=g->numTriangles;return true;}if(mask==0){self.modelInside=true;++self.insideModels;}}
        if(self.hierarchyEnabled){++self.modelTests;margin=frustum.boundMargin(cache->whole);mask=frustum.activePlanes(cache->whole,margin);
            if(mask&128){++self.modelCulls;self.clusterCulls+=cache->bounds.size();self.clusterTriangles+=g->numTriangles;self.earlyCulls+=g->numTriangles;self.triangles3+=g->numTriangles;return true;}
            if(mask==0){self.modelInside=true;++self.insideModels;}
        }
        PreparedModelFrustum prepared(frustum,cache->whole,fastBounds,mask);
        unsigned savedPlanes=0;if(self.hierarchyEnabled){unsigned active=0;for(unsigned plane=0;plane<6;++plane)active+=(mask>>plane)&1;savedPlanes=6-active;}
        for(unsigned k=0;k<cache->bounds.size();++k){++self.clusterTests;unsigned first=k*clusterSize,last=std::min(unsigned(g->numTriangles),first+clusterSize);
            if(mask&&(self.hierarchyEnabled?frustum.outsideMasked(cache->bounds[k],mask,margin):(fastBounds?prepared.outsideCached(cache->bounds[k]):frustum.outside(cache->bounds[k])))){++self.clusterCulls;self.clusterTriangles+=last-first;self.earlyCulls+=last-first;self.triangles3+=last-first;continue;}
            self.planeTestsSaved+=savedPlanes;
            for(unsigned i=first;i<last;++i)self.drawOrder.push_back(i);
        }
        if(self.drawOrder.size()*4<unsigned(g->numTriangles)*3){self.selectedVertices.assign(g->numVertices,0);
            for(int i:self.drawOrder)for(auto index:g->triangles[i].v)if(index<g->numVertices)self.selectedVertices[index]=1;
            for(auto selected:self.selectedVertices)if(!selected)++self.verticesSkipped;
        }
        return true;
    }
    static RasterData* data(rw::Raster* raster){RasterData* p=nullptr;if(raster&&offset>=0)std::memcpy(&p,reinterpret_cast<char*>(raster)+offset,sizeof(p));return p;}
    static void setData(void* object,RasterData* p){std::memcpy(reinterpret_cast<char*>(object)+offset,&p,sizeof(p));}
    static void* rasterCtor(void* object,int,int){setData(object,nullptr);return object;}
    static void* rasterDtor(void* object,int,int){if(active)active->renderer.flushDeferred();delete data(static_cast<rw::Raster*>(object));setData(object,nullptr);return object;}
    static void reject(int line){if(active){++active->failures;if(active->gameCompat&&line>=0&&line<int(active->warned.size())&&!active->warned[line]){active->warned[line]=true;std::fprintf(stderr,"CPU Device unsupported at software_device.cpp:%d\n",line);}}}
    static rw::Raster* create(rw::Raster* r){
        bool texture=r->type==rw::Raster::TEXTURE||r->type==rw::Raster::NORMAL;
        bool target=r->type==rw::Raster::CAMERA||r->type==rw::Raster::ZBUFFER;
        if(target){if(r->width==0&&r->height==0){r->width=320;r->height=240;}if(r->depth==0){r->depth=r->type==rw::Raster::CAMERA?16:32;r->format=r->type==rw::Raster::CAMERA?rw::Raster::C565:rw::Raster::D32;}}
        if(texture&&r->depth==0){r->depth=r->format==rw::Raster::C565?16:32;if(r->format==rw::Raster::DEFAULT)r->format=rw::Raster::C8888;}
        bool format=(r->depth==16&&(r->format==rw::Raster::C565||r->format==rw::Raster::DEFAULT))||
            (r->depth==32&&(r->format==rw::Raster::C8888||r->format==rw::Raster::DEFAULT||r->type==rw::Raster::ZBUFFER));
        if((!texture&&!target)||!format||r->flags||r->width<1||r->height<1||r->width>1024||r->height>1024||
            (target&&(r->width!=320||r->height!=240))){reject(__LINE__);r->destroy();return nullptr;}
        auto p=new RasterData;setData(r,p);r->stride=r->width*(r->depth/8);
        p->bytes.resize(size_t(r->stride)*r->height);r->pixels=p->bytes.data();
        return r;
    }
    static uint8_t* lock(rw::Raster* r,int level,int mode){if(active)active->renderer.flushDeferred();auto p=data(r);
        if(!p||level!=0||p->locked||!(mode&(rw::Raster::LOCKREAD|rw::Raster::LOCKWRITE))||r->parent!=r){reject(__LINE__);return nullptr;}
        p->locked=true;if(mode&rw::Raster::LOCKWRITE)p->dirty=true;return r->pixels;
    }
    static void unlock(rw::Raster* r,int level){auto p=data(r);if(!p||!p->locked||level!=0){reject(__LINE__);return;}p->locked=false;}
    static uint8_t* palette(rw::Raster*,int){reject(__LINE__);return nullptr;}
    static void paletteEnd(rw::Raster*){reject(__LINE__);}
    static int levels(rw::Raster* r){return data(r)?1:0;}
    static rw::bool32 findFormat(rw::Image* img,int type,int* w,int* h,int* d,int* f){
        if(img&&img->pixels&&img->depth!=32)img->convertTo32();
        if(!img||img->depth!=32||type!=rw::Raster::TEXTURE||img->width<1||img->height<1||img->width>1024||img->height>1024){reject(__LINE__);return 0;}
        *w=img->width;*h=img->height;*d=32;*f=type|rw::Raster::C8888;return 1;
    }
    static rw::bool32 fromImage(rw::Raster* r,rw::Image* img){if(active)active->renderer.flushDeferred();auto p=data(r);
        if(!p||!img||img->depth!=32||r->depth!=32||img->width!=r->width||img->height!=r->height||!img->pixels||img->stride<img->width*4){reject(__LINE__);return 0;}
        for(int y=0;y<r->height;++y)std::memcpy(r->pixels+y*r->stride,img->pixels+y*img->stride,size_t(r->width)*4);
        p->dirty=true;return 1;
    }
    static rw::Image* toImage(rw::Raster* r){if(!data(r)){reject(__LINE__);return nullptr;}
        auto img=rw::Image::create(r->width,r->height,32);img->allocate();
        for(int y=0;y<r->height;++y)for(int x=0;x<r->width;++x){auto dst=img->pixels+y*img->stride+x*4;
            if(r->depth==32)std::memcpy(dst,r->pixels+y*r->stride+x*4,4);
            else{uint16_t v;std::memcpy(&v,r->pixels+y*r->stride+x*2,2);dst[0]=((v>>11)&31)*255/31;dst[1]=((v>>5)&63)*255/63;dst[2]=(v&31)*255/31;dst[3]=255;}}
        return img;
    }
    static Texture* texture(rw::Raster* r){auto p=data(r);
        if(!p||p->locked||r->parent!=r||r->type==rw::Raster::ZBUFFER){reject(__LINE__);return nullptr;}
        if(p->dirty){if(active)active->renderer.flushDeferred();auto& t=p->texture;t.width=uint16_t(r->width);t.height=uint16_t(r->height);t.pixels.resize(size_t(r->width)*r->height);t.alpha.resize(t.pixels.size());t.decodedUniform=true;
            for(size_t i=0;i<t.pixels.size();++i){if(r->depth==16){std::memcpy(&t.pixels[i],r->pixels+i*2,2);t.alpha[i]=255;}
                else{auto src=r->pixels+i*4;t.pixels[i]=rgb(src[0],src[1],src[2]);t.alpha[i]=src[3];}if(t.decodedUniform&&i&&t.pixels[i]!=t.pixels[0])t.decodedUniform=false;}
            p->alpha=std::any_of(t.alpha.begin(),t.alpha.end(),[](uint8_t a){return a<255;});t.decodedOpaque=!p->alpha;certifyInkBounds(t);p->dirty=false;
        }return &p->texture;
    }
    static void beginUpdate(rw::Camera* c){auto& self=*active;
        self.updating=false;
        if(!c||!c->getFrame()||!data(c->frameBuffer)||c->frameBuffer->type!=rw::Raster::CAMERA||c->projection!=rw::Camera::PERSPECTIVE||c->viewOffset.x||c->viewOffset.y||
            !std::isfinite(c->viewWindow.x)||!std::isfinite(c->viewWindow.y)||!std::isfinite(c->nearPlane)||!std::isfinite(c->farPlane)||c->viewWindow.x<.016f||c->viewWindow.y<.012f||c->nearPlane<.001f||c->farPlane<=c->nearPlane||c->farPlane>10000){if(c&&self.gameCompat&&!self.cameraWarning){self.cameraWarning=true;std::fprintf(stderr,"CPU CAMERA rejected projection=%d near=%g far=%g window=%g,%g offset=%g,%g\n",int(c->projection),c->nearPlane,c->farPlane,c->viewWindow.x,c->viewWindow.y,c->viewOffset.x,c->viewOffset.y);}reject(__LINE__);return;}
        rw::Matrix::invertOrthonormal(&self.inverse,c->getFrame()->getLTM());
        self.projection=Camera{};self.projection.position={0,0,0};self.projection.focal=160/c->viewWindow.x;
        self.projection.nearZ=c->nearPlane;self.projection.farZ=c->farPlane;
        // Our renderer has one focal value; differing Y focal lengths are adapted in transform.
        self.updating=true;
    }
    static void endUpdate(rw::Camera* c){if(!active->updating||!c||!data(c->frameBuffer)){reject(__LINE__);return;}
        auto r=c->frameBuffer;auto& pixels=active->renderer.pixels();
        if(r->depth==16&&pixels.size()==320*240)std::memcpy(r->pixels,pixels.data(),pixels.size()*2);
        else for(size_t i=0;i<320*240;++i){auto v=pixels[pixels.size()==640*480?(i/320*2)*640+(i%320*2):i];if(r->depth==16){std::memcpy(r->pixels+i*2,&v,2);continue;}auto dst=r->pixels+i*4;dst[0]=((v>>11)&31)*255/31;dst[1]=((v>>5)&63)*255/63;dst[2]=(v&31)*255/31;dst[3]=255;}
        data(r)->dirty=true;active->updating=false;active->transformed.clear();
    }
    static void clear(rw::Camera* c,rw::RGBA* color,uint32_t mode){
        if(!c||!data(c->frameBuffer)||!color||(mode&~uint32_t(rw::Camera::CLEARIMAGE|rw::Camera::CLEARZ))){reject(__LINE__);return;}
        active->renderer.clearBuffers(rgb(color->red,color->green,color->blue),mode&rw::Camera::CLEARIMAGE,mode&rw::Camera::CLEARZ);
    }
    static void show(rw::Raster* r,uint32_t){if(!data(r)||r->type!=rw::Raster::CAMERA)reject(__LINE__);}
    static void state(int s,void* value){if(s<0||s>rw::GSALPHATESTREF){reject(__LINE__);return;}
        if(active->gameCompat){if(s==rw::TEXTUREFILTER&&uintptr_t(value)>rw::Texture::LINEAR)value=reinterpret_cast<void*>(uintptr_t(rw::Texture::LINEAR));if(s==rw::FOGENABLE||s==rw::STENCILENABLE||s==rw::GSALPHATEST)value=nullptr;}
        active->states[s]=value;
        if(s==rw::TEXTUREADDRESS)active->states[rw::TEXTUREADDRESSU]=active->states[rw::TEXTUREADDRESSV]=value;
    }
    static void* get(int s){if(s<0||s>rw::GSALPHATESTREF){reject(__LINE__);return nullptr;}return active->states[s];}
    static uintptr_t value(int s){return reinterpret_cast<uintptr_t>(get(s));}
    static bool screenState(ScreenState& out,int alphaOverride=-1,int addressU=-1,int addressV=-1){
        if(value(rw::FOGENABLE)||value(rw::STENCILENABLE)||value(rw::GSALPHATEST)){reject(__LINE__);return false;}
        out.linear=active->filteredEffects&&value(rw::TEXTUREFILTER)==rw::Texture::LINEAR;out.zTest=value(rw::ZTESTENABLE);out.zWrite=value(rw::ZWRITEENABLE);out.cull=unsigned(value(rw::CULLMODE));
        out.addressU=uint8_t(addressU<0?value(rw::TEXTUREADDRESSU):uintptr_t(addressU));out.addressV=uint8_t(addressV<0?value(rw::TEXTUREADDRESSV):uintptr_t(addressV));
        if(out.addressU<1||out.addressU>4||out.addressV<1||out.addressV>4||value(rw::TEXTUREFILTER)>rw::Texture::LINEAR){reject(__LINE__);return false;}
        if(value(rw::ALPHATESTFUNC)==rw::ALPHAGREATEREQUAL)out.alphaRef=unsigned(value(rw::ALPHATESTREF));
        else if(value(rw::ALPHATESTFUNC)!=rw::ALPHAALWAYS){reject(__LINE__);return false;}
        auto source=value(rw::SRCBLEND),dest=value(rw::DESTBLEND);
        if(!(alphaOverride<0?value(rw::VERTEXALPHA):uintptr_t(alphaOverride)))out.blend=0;
        else if(source==rw::BLENDONE&&dest==rw::BLENDZERO)out.blend=0;
        else if(source==rw::BLENDSRCALPHA&&dest==rw::BLENDINVSRCALPHA)out.blend=1;
        else if(source==rw::BLENDSRCALPHA&&dest==rw::BLENDONE)out.blend=2;
        else if(source==rw::BLENDONE&&dest==rw::BLENDONE)out.blend=3;
        else if(source==rw::BLENDZERO&&dest==rw::BLENDINVSRCCOLOR)out.blend=4;
        else if(source==rw::BLENDZERO&&dest==rw::BLENDONE)out.blend=5;
        else{if(active->gameCompat&&!active->warnedBlend){active->warnedBlend=true;std::fprintf(stderr,"CPU blend source=%lu destination=%lu\n",(unsigned long)source,(unsigned long)dest);}reject(__LINE__);return false;}return true;
    }
    static ScreenVertex screen(rw::cpu::Im2DVertex p){return {p.x,p.y,p.reciprocalZ,p.u,p.v,float(p.r),float(p.g),float(p.b),float(p.a),p.z};}
    static void triangle2(void* vertices,int count,int a,int b,int c){
        if(!active->updating||!vertices||count<0||count>65535||a<0||b<0||c<0||a>=count||b>=count||c>=count){reject(__LINE__);return;}
        ScreenState options;if(!screenState(options))return;auto r=static_cast<rw::Raster*>(get(rw::TEXTURERASTER));
        Texture* tex=r?texture(r):nullptr;if(r&&!tex)return;
        if(active->gameCompat){options.screenDepth=true;options.linear=tex&&value(rw::TEXTUREFILTER)==rw::Texture::LINEAR;options.alphaRef=options.blend==5?0:1;
            if(r&&data(r)->alpha&&options.blend==0&&value(rw::SRCBLEND)==rw::BLENDSRCALPHA&&value(rw::DESTBLEND)==rw::BLENDINVSRCALPHA)options.blend=1;}
        auto v=static_cast<rw::cpu::Im2DVertex*>(vertices);

        ScreenVertex points[3]={screen(v[a]),screen(v[b]),screen(v[c])};active->renderer.screenTriangle(points,tex,options);++active->triangles2;
    }
    static void line2(void* vertices,int count,int a,int b){
        if(!active->updating||!vertices||count<0||count>65535||a<0||b<0||a>=count||b>=count){reject(__LINE__);return;}
        auto v=static_cast<rw::cpu::Im2DVertex*>(vertices);ScreenVertex p=screen(v[a]),q=screen(v[b]);
        float dx=q.x-p.x,dy=q.y-p.y,length=std::hypot(dx,dy);
        if(!std::isfinite(length)||length>1e6f){reject(__LINE__);return;}
        ScreenState options;if(!screenState(options))return;
        float nx=length>1e-5f?-dy/(2*length):.5f,ny=length>1e-5f?dx/(2*length):0;
        if(length<=1e-5f){p.y-=.5f;q.y+=.5f;}
        ScreenVertex corners[4]={p,p,q,q};corners[0].x+=nx;corners[0].y+=ny;corners[1].x-=nx;corners[1].y-=ny;
        corners[2].x+=nx;corners[2].y+=ny;corners[3].x-=nx;corners[3].y-=ny;
        ScreenVertex first[3]={corners[0],corners[1],corners[2]},second[3]={corners[2],corners[1],corners[3]};
        active->renderer.screenTriangle(first,nullptr,options);active->renderer.screenTriangle(second,nullptr,options);
    }
    static bool lines2(rw::PrimitiveType type,void* v,int n,uint16_t* indices,int count){
        if(type!=rw::PRIMTYPELINELIST&&type!=rw::PRIMTYPEPOLYLINE)return false;
        if(!v||n<0||n>65535||count<0||count>196605||(!indices&&count>n)){reject(__LINE__);return true;}
        if(type==rw::PRIMTYPELINELIST&&count%2){reject(__LINE__);return true;}
        if(indices)for(int i=0;i<count;++i)if(indices[i]>=n){reject(__LINE__);return true;}
        int step=type==rw::PRIMTYPELINELIST?2:1;for(int i=0;i+1<count;i+=step)line2(v,n,indices?indices[i]:i,indices?indices[i+1]:i+1);return true;
    }
    template<class Emit> static void primitives(rw::PrimitiveType type,int count,const uint16_t* indices,int vertices,Emit emit){
        if(count<0||count>196605||vertices<0||vertices>65535){reject(__LINE__);return;}
        if(indices)for(int i=0;i<count;++i)if(indices[i]>=vertices){reject(__LINE__);return;}
        auto at=[&](int i){return indices?int(indices[i]):i;};
        if(!indices&&count>vertices){reject(__LINE__);return;}
        if(type==rw::PRIMTYPETRILIST){if(count%3){reject(__LINE__);return;}for(int i=0;i<count;i+=3)emit(at(i),at(i+1),at(i+2));}
        else if(type==rw::PRIMTYPETRIFAN){for(int i=2;i<count;++i)emit(at(0),at(i-1),at(i));}
        else if(type==rw::PRIMTYPETRISTRIP){for(int i=2;i<count;++i)emit(at(i-2+(i&1)),at(i-1-(i&1)),at(i));}
        else reject(__LINE__);
    }
    static void primitive2(rw::PrimitiveType type,void* v,int n){if(lines2(type,v,n,nullptr,n))return;primitives(type,n,nullptr,n,[&](int a,int b,int c){triangle2(v,n,a,b,c);});}
    static void indexed2(rw::PrimitiveType type,void* v,int n,void* indices,int count){if(!indices){reject(__LINE__);return;}if(lines2(type,v,n,static_cast<uint16_t*>(indices),count))return;primitives(type,count,static_cast<uint16_t*>(indices),n,[&](int a,int b,int c){triangle2(v,n,a,b,c);});}
    static void transform3Masked(void* vertices,int n,rw::Matrix* world,const uint8_t* selected,bool preserve=false){auto& self=*active;if(!preserve)self.transformed.clear();
        if(!self.updating||!vertices||n<0||n>65535){reject(__LINE__);return;}auto v=static_cast<rw::cpu::Im3DVertex*>(vertices);if(v==self.transformed.data()){/* Atomic staging buffer already owns these vertices. */}else if(preserve){if(self.transformed.size()<size_t(n))self.transformed.resize(n);if(selected){for(int i=0;i<n;++i)if(selected[i])self.transformed[i]=v[i];}else std::copy(v,v+n,self.transformed.begin());}else self.transformed.assign(v,v+n);
        rw::Matrix modelView;bool combined=world&&n>=8;if(combined){
            // re3 can edit a matrix directly without updating identity hints.
            // The previous point transforms always used its numeric entries.
            auto model=*world,view=self.inverse;model.flags&=~rw::Matrix::IDENTITY;view.flags&=~rw::Matrix::IDENTITY;rw::Matrix::mult(&modelView,&model,&view);
        }
        const rw::Matrix* matrix=combined?&modelView:&self.inverse;
        const float vertical=120/rw::engine->currentCamera->viewWindow.y/self.projection.focal;
        for(int i=0;i<n;++i){if(selected&&!selected[i])continue;auto& p=self.transformed[i];rw::V3d point=p.position;
            if(world&&!combined)rw::V3d::transformPoints(&point,&p.position,1,world);
            rw::V3d::transformPoints(&p.position,&point,1,matrix);
            // RenderWare's camera right vector points screen-left.
            p.position.x=-p.position.x;
            p.position.y*=vertical;
        }
    }
    // Atomic-only source path. Keep generic Im3D's hot loop untouched.
    // Caller supplies >=8 vertices, valid position/UV arrays and resized buffers.
    static void transformAtomicDirect(const rw::V3d* positions,const rw::TexCoords* uv,int n,rw::Matrix* world,const uint8_t* selected){
        auto& self=*active;if(self.transformed.size()<size_t(n))self.transformed.resize(n);
        rw::Matrix modelView;if(world){auto model=*world,view=self.inverse;model.flags&=~rw::Matrix::IDENTITY;view.flags&=~rw::Matrix::IDENTITY;rw::Matrix::mult(&modelView,&model,&view);}
        const rw::Matrix* matrix=world?&modelView:&self.inverse;
        const float vertical=120/rw::engine->currentCamera->viewWindow.y/self.projection.focal;
        const float sx=160/self.projection.focal,sy=120/self.projection.focal;
        const bool inside=self.modelInside;auto target=self.transformed.data();auto shadow=self.atomicVertices.data();auto codes=self.clipCodes.data();unsigned done=0;
        for(int i=0;i<n;++i){if(selected&&!selected[i])continue;auto& p=target[i];rw::V3d point=positions[i];
            rw::V3d::transformPoints(&p.position,&point,1,matrix);p.position.x=-p.position.x;p.position.y*=vertical;
            p.u=uv[i].u;p.v=uv[i].v;shadow[i].u=p.u;shadow[i].v=p.v;
            auto q=p.position;codes[i]=inside?0:uint8_t((q.z<self.projection.nearZ?1u:0u)|(q.z>self.projection.farZ?2u:0u)|(q.x < -sx*q.z?4u:0u)|(q.x > sx*q.z?8u:0u)|(q.y < -sy*q.z?16u:0u)|(q.y > sy*q.z?32u:0u));++done;
        }self.directVertexCount+=done;
    }
    static void transform3(void* vertices,int n,rw::Matrix* world,uint32_t){transform3Masked(vertices,n,world,nullptr);}
    static void triangle3Checked(int a,int b,int c,bool faceVerified,bool atomicVertices=false,const ScreenState* prepared=nullptr,const Texture* preparedTexture=nullptr,const WorldTriangleSetup* setup=nullptr){auto& self=*active;
        if(!self.updating||a<0||b<0||c<0||size_t(std::max({a,b,c}))>=self.transformed.size()){reject(__LINE__);return;}
        ScreenState state;const Texture* tex=preparedTexture;
        if(prepared)state=*prepared;else{if(!screenState(state))return;auto raster=static_cast<rw::Raster*>(get(rw::TEXTURERASTER));tex=raster?texture(raster):nullptr;if(raster&&!tex)return;}
        auto v0=self.transformed[a],v1=self.transformed[b],v2=self.transformed[c];
        // Keep the verified opaque path for existing map/car assets. Use the
        // general clipped path only for color gradients or different states.
        bool flat=v0.r==v1.r&&v0.r==v2.r&&v0.g==v1.g&&v0.g==v2.g&&v0.b==v1.b&&v0.b==v2.b;
        bool fadingFlat=!self.quality&&!self.preservePrelight&&!flat&&self.flatFadingEnabled&&atomicVertices&&state.blend==1&&state.zTest&&tex&&tex->decodedOpaque&&v0.a==v1.a&&v0.a==v2.a;
        auto smallRange=[](unsigned a,unsigned b,unsigned c){return std::max({a,b,c})-std::min({a,b,c})<=24;};
        bool distantFlat=!flat&&(self.distanceShadingAtomic||self.farSurface)&&!self.preservePrelight&&atomicVertices&&state.zTest&&state.blend<=1&&tex&&tex->decodedOpaque&&v0.a==v1.a&&v0.a==v2.a&&std::min({v0.position.z,v1.position.z,v2.position.z})>=(self.farSurface?40:24)&&smallRange(v0.r,v1.r,v2.r)&&smallRange(v0.g,v1.g,v2.g)&&smallRange(v0.b,v1.b,v2.b);
        if(distantFlat||(!self.quality&&!self.preservePrelight&&((!state.blend&&v0.a==255&&v1.a==255&&v2.a==255)||fadingFlat))){
            if(distantFlat)++self.distanceShaded;
            flat=true;if(fadingFlat)++self.flattenedFading;
            uint8_t r=uint8_t((unsigned(v0.r)+v1.r+v2.r)/3),g=uint8_t((unsigned(v0.g)+v1.g+v2.g)/3),b=uint8_t((unsigned(v0.b)+v1.b+v2.b)/3);
            v0.setColor(r,g,b,v0.a);v1.setColor(r,g,b,v1.a);v2.setColor(r,g,b,v2.a);
        }
        // librw blends texture alpha independently of the vertex-alpha flag.
        // Keep FLAT's vertex colour averaging before selecting the blend path.
        if(self.gameCompat&&self.textureAlphaEnabled&&tex&&!tex->decodedOpaque&&state.blend==0){
            if(!screenState(state,1,state.addressU,state.addressV))return;
            if(state.blend)++self.textureAlphaTriangles;
        }
        // Smooth only monochrome black alpha masks in world materials.
        // These baked overlays need alpha interpolation, not four RGB samples.
        if(self.gameCompat&&atomicVertices&&self.quality&&tex&&!tex->decodedOpaque&&tex->decodedUniform&&tex->pixels[0]==0&&state.blend==1&&value(rw::TEXTUREFILTER)==rw::Texture::LINEAR)state.linear=true;
        bool fast=flat&&!state.linear&&v0.a==255&&v1.a==255&&v2.a==255&&state.zTest&&state.zWrite&&!state.blend&&(state.cull==rw::CULLNONE||(v0.position.z>=self.projection.nearZ&&v1.position.z>=self.projection.nearZ&&v2.position.z>=self.projection.nearZ))&&state.alphaRef<=255&&
            (!tex||(tex->width<=128&&tex->height<=128&&!(tex->width&(tex->width-1))&&!(tex->height&(tex->height-1))));
        if(fast){
            if(state.cull!=rw::CULLNONE&&!faceVerified){float ax=v0.position.x/v0.position.z,ay=-v0.position.y/v0.position.z;
                float bx=v1.position.x/v1.position.z,by=-v1.position.y/v1.position.z,cx=v2.position.x/v2.position.z,cy=-v2.position.y/v2.position.z;
                float area=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax);
                if((state.cull==rw::CULLBACK&&area<0)||(state.cull==rw::CULLFRONT&&area>0)){++self.triangles3;return;}
            }
            uint16_t color=rgb(v0.r,v0.g,v0.b);
            Vec3 points[3]={{v0.position.x,v0.position.y,v0.position.z},{v1.position.x,v1.position.y,v1.position.z},{v2.position.x,v2.position.y,v2.position.z}};
            if(tex){
                bool inside=self.prepareAtomics&&atomicVertices&&!(self.clipCodes[a]|self.clipCodes[b]|self.clipCodes[c])&&self.projection.focal>=1&&self.projection.focal<=10000;
                if(inside){int indices[3]={a,b,c};for(unsigned i=0;i<3;++i){unsigned index=unsigned(indices[i]);auto& ready=self.projectionReady[index];
                    // Bit 0: position projected. Bits 1/2: flat validation valid/invalid.
                    if(!(ready&6)){auto v=self.transformed[index];auto p=v.position;bool valid=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::fabs(p.x)<=1e6f&&std::fabs(p.y)<=1e6f&&std::fabs(p.z)<=1e6f&&std::isfinite(v.u)&&std::isfinite(v.v)&&std::fabs(v.u)<=1e5f&&std::fabs(v.v)<=1e5f;ready|=valid?2:4;}
                    inside&=!(ready&4);
                }}
                if(inside){ScreenVertex screen[3];int indices[3]={a,b,c};rw::cpu::Im3DVertex vertices[3]={v0,v1,v2};
                    for(unsigned i=0;i<3;++i){unsigned index=unsigned(indices[i]);auto v=vertices[i];
                        if(!(self.projectionReady[index]&1)){float iz=1/v.position.z;self.screenVertices[index]={160+self.projection.focal*v.position.x*iz,120-self.projection.focal*v.position.y*iz,iz};self.projectionReady[index]|=1;}else ++self.flatProjectionReused;
                        auto p=self.screenVertices[index];screen[i]={p.x,p.y,p.z,v.u,v.v,0,0,0,0,0};}
                    SoftwareDevice::drawPreparedFlat(self.renderer,screen,color,*tex,state.addressU,state.addressV,state.alphaRef,setup);
                }else{Vec2 uv[3]={{v0.u,v0.v},{v1.u,v1.v},{v2.u,v2.v}};self.renderer.texturedTriangle(points,uv,color,*tex,state.addressU,state.addressV,self.projection,state.alphaRef);}
            }
            else self.renderer.triangle(points,color,self.projection);
        }else if(atomicVertices&&!(self.clipCodes[a]|self.clipCodes[b]|self.clipCodes[c])){
            // Fully inside the six planes: the unclipped projection is exactly
            // the one shadedTriangle would produce. Reuse positions/UV only;
            // colour may differ between materials sharing a vertex.
            bool verified=self.preparedScreen;ScreenVertex points[3];int indices[3]={a,b,c};rw::cpu::Im3DVertex vertices[3]={v0,v1,v2};
            for(int i=0;i<3;++i){unsigned index=unsigned(indices[i]);auto v=vertices[i];
                if(!(self.projectionReady[index]&1)){float iz=1/v.position.z;
                    self.screenVertices[index]={160+self.projection.focal*v.position.x*iz,120-self.projection.focal*v.position.y*iz,iz};self.projectionReady[index]|=1;}
                auto p=self.screenVertices[index];
                if(verified){auto& ready=self.projectionReady[index];
                    // Bits 3/4 certify the projected position and raw UV for
                    // this transform. Material colours originate as uint8_t.
                    if(!(ready&24)){bool valid=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::fabs(p.x)<=1e6f&&std::fabs(p.y)<=1e6f&&p.z>=0&&p.z<=1e6f&&std::isfinite(v.u)&&std::isfinite(v.v)&&std::fabs(v.u)<=1e5f&&std::fabs(v.v)<=1e5f;ready|=valid?8:16;}
                    verified=!(ready&16);
                }
                points[i]={p.x,p.y,p.z,v.u,v.v,float(v.r),float(v.g),float(v.b),float(v.a),0};
            }
            state.perspectiveColor=true;if(verified)SoftwareDevice::drawPreparedScreen(self.renderer,points,tex,state,setup);else self.renderer.screenTriangle(points,tex,state);++self.shaded;
        }else{ShadedVertex points[3];int i=0;
            for(auto v:{v0,v1,v2})points[i++]={{v.position.x,v.position.y,v.position.z},{v.u,v.v},float(v.r),float(v.g),float(v.b),float(v.a)};
            self.renderer.shadedTriangle(points,tex,state,self.projection);++self.shaded;
        }++self.triangles3;
    }

    static void triangle3(int a,int b,int c){triangle3Checked(a,b,c,false);}
    static void primitive3(rw::PrimitiveType type){int n=int(active->transformed.size());primitives(type,n,nullptr,n,triangle3);}
    static void indexed3(rw::PrimitiveType type,void* indices,int count){if(!indices){reject(__LINE__);return;}primitives(type,count,static_cast<uint16_t*>(indices),int(active->transformed.size()),triangle3);}
    static void end3(){active->transformed.clear();}
    static rw::bool32 renderFast(rw::Raster* r,int x,int y){if(!active->updating||!texture(r)){reject(__LINE__);return 0;}
        auto previous=get(rw::TEXTURERASTER);state(rw::TEXTURERASTER,r);rw::cpu::Im2DVertex v[4];
        for(int i=0;i<4;++i){v[i].x=float(x+(i&1)*r->width);v[i].y=float(y+(i>>1)*r->height);v[i].u=float(i&1);v[i].v=float(i>>1);}
        triangle2(v,4,0,1,2);triangle2(v,4,2,1,3);state(rw::TEXTURERASTER,previous);return 1;
    }
    static int system(rw::DeviceReq req,void* arg,int n){switch(req){
        case rw::DEVICEOPEN:case rw::DEVICECLOSE:case rw::DEVICEINIT:case rw::DEVICETERM:case rw::DEVICEFINALIZE:return 1;
        case rw::DEVICEGETNUMSUBSYSTEMS:case rw::DEVICEGETNUMVIDEOMODES:return 1;
        case rw::DEVICEGETCURRENTSUBSYSTEM:case rw::DEVICEGETCURRENTVIDEOMODE:return 0;
        case rw::DEVICESETSUBSYSTEM:case rw::DEVICESETVIDEOMODE:case rw::DEVICESETMULTISAMPLINGLEVELS:return n==0;
        case rw::DEVICEGETMAXMULTISAMPLINGLEVELS:case rw::DEVICEGETMULTISAMPLINGLEVELS:return 0;
        case rw::DEVICEGETSUBSSYSTEMINFO:if(arg&&n==0){auto info=static_cast<rw::SubSystemInfo*>(arg);std::strcpy(info->name,"Miyoo CPU 320x240");return 1;}return 0;
        case rw::DEVICEGETVIDEOMODEINFO:if(arg&&n==0){*static_cast<rw::VideoMode*>(arg)={320,240,16,0};return 1;}return 0;
        default:reject(__LINE__);return 0;}}
    static bool deform(rw::Atomic* a,rw::Skin* skin){
        auto& self=*active;auto g=a->geometry;
        if(skin->numBones<1||skin->numBones>256||!skin->indices||!skin->weights||!skin->inverseMatrices){reject(__LINE__);return false;}
        auto hier=rw::Skin::getHierarchy(a);
        rw::Matrix identity;identity.setIdentity();auto object=a->getFrame()?a->getFrame()->getLTM():&identity;
        if(hier){
            if(hier->numNodes!=skin->numBones||!hier->matrices||(hier->flags&rw::HAnimHierarchy::NOMATRICES)){reject(__LINE__);return false;}
            rw::Matrix inverseObject;rw::Matrix::invert(&inverseObject,object);
            for(int i=0;i<skin->numBones;++i){
                rw::Matrix bind;std::memcpy(&bind,skin->inverseMatrices+i*16,sizeof(bind));bind.flags=0;
                // Match librw GL3: inverse bind, bone pose, then inverse atomic
                // transform for a hierarchy which supplies world matrices.
                rw::Matrix pose=hier->matrices[i];pose.flags=0;
                if(!(hier->flags&rw::HAnimHierarchy::LOCALSPACEMATRICES)){rw::Matrix tmp;rw::Matrix::mult(&tmp,&pose,&inverseObject);pose=tmp;}
                rw::Matrix::mult(&self.boneMatrices[i],&bind,&pose);
            }
        }else for(int i=0;i<skin->numBones;++i)self.boneMatrices[i].setIdentity();
        self.skinPositions.resize(g->numVertices);
        bool normals=g->morphTargets[0].normals!=nullptr;self.skinNormals.resize(normals?g->numVertices:0);
        for(int i=0;i<g->numVertices;++i){
            rw::V3d p={0,0,0},n={0,0,0};float total=0;
            for(int j=0;j<4;++j){float w=skin->weights[i*4+j];if(!std::isfinite(w)||w<0||w>1.001f){reject(__LINE__);return false;}if(w==0)continue;
                unsigned index=skin->indices[i*4+j];if(index>=unsigned(skin->numBones)){reject(__LINE__);return false;}
                rw::V3d part;rw::V3d::transformPoints(&part,&g->morphTargets[0].vertices[i],1,&self.boneMatrices[index]);
                p.x+=w*part.x;p.y+=w*part.y;p.z+=w*part.z;total+=w;
                if(normals){rw::V3d::transformVectors(&part,&g->morphTargets[0].normals[i],1,&self.boneMatrices[index]);n.x+=w*part.x;n.y+=w*part.y;n.z+=w*part.z;}
            }
            if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||std::fabs(total-1)>0.01f){reject(__LINE__);return false;}
            self.skinPositions[i]=p;if(normals)self.skinNormals[i]=n;
        }
        ++self.skins;self.skinVertices+=g->numVertices;return true;
    }
    static int earlyTriangle(const rw::Triangle& t,WorldTriangleSetup& setup){auto& self=*active;Vec3 points[3];
        for(unsigned j=0;j<3;++j){unsigned index=t.v[j];auto& ready=self.projectionReady[index];auto v=self.transformed[index];auto p=v.position;
            if(!(ready&1)){float iz=1/p.z;self.screenVertices[index]={160+self.projection.focal*p.x*iz,120-self.projection.focal*p.y*iz,iz};ready|=1;}
            auto q=self.screenVertices[index];
            // Same projected position/raw UV certificate as the shaded path.
            // The flat path still performs its separate source-space check.
            if(!(ready&24)){bool valid=std::isfinite(q.x)&&std::isfinite(q.y)&&std::isfinite(q.z)&&std::fabs(q.x)<=1e6f&&std::fabs(q.y)<=1e6f&&q.z>0&&q.z<=1e6f&&std::isfinite(v.u)&&std::isfinite(v.v)&&std::fabs(v.u)<=1e5f&&std::fabs(v.v)<=1e5f;ready|=valid?8:16;}
            if(ready&16)return 0;points[j]=q;
        }
        int result=SoftwareDevice::queryWorldVisibility(self.renderer,points,setup,self.earlyWideEnabled);if(result){++self.earlyQueries;if(result==2)++self.earlyHidden;}return result;
    }
    static void atomic(rw::ObjPipeline*,rw::Atomic* a){if(!a||!a->geometry||!active->updating){reject(__LINE__);return;}
        auto g=a->geometry;if(active->worldLodSelector){auto replacement=active->worldLodSelector(a);if(replacement&&replacement!=g){++active->worldLodCalls;active->worldLodSaved+=g->numTriangles-replacement->numTriangles;g=replacement;}}if(g->numVertices<1||g->numVertices>65535||g->numTriangles<1||g->numTriangles>65535||(g->flags&rw::Geometry::NATIVE)||g->numMorphTargets!=1||!g->morphTargets[0].vertices||!g->triangles){reject(__LINE__);return;}
        auto& self=*active;++self.atomics;if(self.gameCompat&&a->pipeline==rw::matFXGlobals.dummypipe)++self.matFx;
        rw::Skin* skin=self.gameCompat&&rw::skinGlobals.geoOffset>0?rw::Skin::get(g):nullptr;
        self.distanceShadingAtomic=self.distanceShadingEnabled&&!skin&&!a->clump;
        self.drawOrder.clear();self.selectedVertices.clear();
        self.modelInside=false;bool clustered=!skin&&selectClusters(a,g);
        if(clustered&&self.drawOrder.empty())return;
        if(!clustered){self.drawOrder.resize(g->numTriangles);for(int i=0;i<g->numTriangles;++i)self.drawOrder[i]=i;}
        const uint8_t* selected=self.selectedVertices.empty()?nullptr:self.selectedVertices.data();
        if(skin&&!deform(a,skin))return;
        auto positions=skin?self.skinPositions.data():g->morphTargets[0].vertices;
        auto normals=skin?self.skinNormals.data():g->morphTargets[0].normals;
        rw::WorldLights lightData{};rw::Light* directions[8];rw::Light* locals[8];
        bool lit=(g->flags&rw::Geometry::LIGHT)&&(skin||self.quality||!self.gameCompat);
        if(lit){
            lightData.directionals=directions;lightData.numDirectionals=8;lightData.locals=locals;lightData.numLocals=8;
            if(rw::engine->currentWorld)static_cast<rw::World*>(rw::engine->currentWorld)->enumerateLights(a,&lightData);
            if(lightData.numLocals&&!self.gameCompat){reject(__LINE__);return;} // Point/spot lights are a later pipeline stage.
            if((g->flags&rw::Geometry::NORMALS)&&!g->morphTargets[0].normals){reject(__LINE__);return;}
            // No directional contribution means normals cannot affect RGB.
            // Keep ambient/material/prelight computation and all geometry intact.
            bool ambientOnly=self.ambientShortcutEnabled&&lightData.numDirectionals==0;
            if((g->flags&rw::Geometry::NORMALS)&&!ambientOnly)self.directLight.assign(g->numVertices,{0,0,0});else self.directLight.clear();
            if(ambientOnly&&(g->flags&rw::Geometry::NORMALS))self.ambientSkipped+=g->numVertices;
            Vec3 unitDirections[8],lightColors[8];
            for(int j=0;j<lightData.numDirectionals;++j){auto light=directions[j];if(!light->getFrame()){reject(__LINE__);return;}auto d=light->getFrame()->getLTM()->at;float length=rw::length(d);
                if(!std::isfinite(length)||length<1e-8f){reject(__LINE__);return;}unitDirections[j]={d.x/length,d.y/length,d.z/length};lightColors[j]={light->color.red,light->color.green,light->color.blue};}
            if((g->flags&rw::Geometry::NORMALS)&&!ambientOnly){
                rw::Matrix identity;identity.setIdentity();auto matrix=a->getFrame()?a->getFrame()->getLTM():&identity;rw::Matrix inverse;
                rw::Matrix::invert(&inverse,matrix);
                auto normalCache=!skin?normalsFor(g,normals,inverse):nullptr;
                Vec3 axisLight[6]={};if(self.farNormal&&!skin&&!a->clump){for(unsigned axis=0;axis<6;++axis){rw::V3d n={axis/2==0?1.f:0.f,axis/2==1?1.f:0.f,axis/2==2?1.f:0.f};if(axis&1)n=rw::scale(n,-1);rw::V3d v={rw::dot(inverse.right,n),rw::dot(inverse.up,n),rw::dot(inverse.at,n)};float len=rw::length(v);if(!std::isfinite(len)||len<1e-8f)continue;v=rw::scale(v,1/len);for(int j=0;j<lightData.numDirectionals;++j){auto d=unitDirections[j],c=lightColors[j];float t=std::max(0.f,-(v.x*d.x+v.y*d.y+v.z*d.z));axisLight[axis].x+=t*c.x;axisLight[axis].y+=t*c.y;axisLight[axis].z+=t*c.z;}}}
                for(int i=0;i<g->numVertices;++i){if(selected&&!selected[i])continue;rw::V3d normal;
                    if(self.farNormal&&!skin&&!a->clump){auto n=normals[i];unsigned axis=normalAxis(n.x,n.y,n.z);if(axis<6){self.directLight[i]=axisLight[axis];++self.farNormals;continue;}}

                    if(normalCache&&normalCache->normalReady[i]){auto n=normalCache->unitNormals[i];normal={n.x,n.y,n.z};++self.normalsReused;}
                    else{auto n=normals[i];normal={rw::dot(inverse.right,n),rw::dot(inverse.up,n),rw::dot(inverse.at,n)};
                        float len=rw::length(normal);if(!std::isfinite(len)){reject(__LINE__);return;}if(len>1e-8f)normal=rw::scale(normal,1/len);
                        if(normalCache){normalCache->unitNormals[i]={normal.x,normal.y,normal.z};normalCache->normalReady[i]=1;++self.normalsPrepared;}}

                    for(int j=0;j<lightData.numDirectionals;++j){auto d=unitDirections[j],color=lightColors[j];float amount=std::max(0.f,-(normal.x*d.x+normal.y*d.y+normal.z*d.z));
                        self.directLight[i].x+=amount*color.x;self.directLight[i].y+=amount*color.y;self.directLight[i].z+=amount*color.z;}
                }
            }
        }
        if(self.traceTextures)for(int i=0;i<g->matList.numMaterials;++i){auto mat=g->matList.materials[i];auto tex=mat?mat->texture:nullptr;if(!tex||!tex->raster)continue;auto p=data(tex->raster);if(!p||p->traced)continue;p->traced=true;auto t=texture(tex->raster);if(t&&!t->decodedOpaque)std::printf("ALPHA TEXTURE name=%.32s width=%u height=%u uniform=%d first_rgb=%u\n",tex->name,t->width,t->height,t->decodedUniform,t->pixels[0]);}
        auto saved=get(rw::TEXTURERASTER),savedU=get(rw::TEXTUREADDRESSU),savedV=get(rw::TEXTUREADDRESSV),savedAlpha=get(rw::VERTEXALPHA);
        // Read source positions/UV straight into their final transformed buffer.
        // Keep the generic Im3D and small/missing-UV paths unchanged.
        bool direct=self.directAtomicEnabled&&g->numVertices>=8&&g->numTexCoordSets&&g->texCoords[0];
        if(self.atomicVertices.size()<size_t(g->numVertices))self.atomicVertices.resize(g->numVertices);
        self.clipCodes.resize(g->numVertices);
        if(!direct){
            for(int i=0;i<g->numVertices;++i){if(selected&&!selected[i])continue;auto& v=self.atomicVertices[i];v.position=positions[i];
                if(g->numTexCoordSets&&g->texCoords[0]){v.u=g->texCoords[0][i].u;v.v=g->texCoords[0][i].v;}}
        }
        self.transformed.swap(self.atomicScratch);
        if(self.transferEnabled&&!direct){self.transformed.swap(self.atomicVertices);self.transferredVertices+=g->numVertices;}
        if(direct)transformAtomicDirect(positions,g->texCoords[0],g->numVertices,a->getFrame()?a->getFrame()->getLTM():nullptr,selected);
        else transform3Masked(self.transferEnabled?self.transformed.data():self.atomicVertices.data(),g->numVertices,a->getFrame()?a->getFrame()->getLTM():nullptr,selected,true);
        // Shared vertices reuse the six-plane classification. Reject triangles
        // outside one common plane before material/colour/sampler preparation.
        self.screenVertices.resize(g->numVertices);self.projectionReady.assign(g->numVertices,0);
        self.clipCodes.resize(g->numVertices);float sx=160/self.projection.focal,sy=120/self.projection.focal;
        if(!direct&&self.modelInside)std::fill(self.clipCodes.begin(),self.clipCodes.end(),0);
        else if(!direct)for(int i=0;i<g->numVertices;++i){if(selected&&!selected[i])continue;auto p=self.transformed[i].position;self.clipCodes[i]=uint8_t((p.z<self.projection.nearZ?1u:0u)|(p.z>self.projection.farZ?2u:0u)|(p.x < -sx*p.z?4u:0u)|(p.x > sx*p.z?8u:0u)|(p.y < -sy*p.z?16u:0u)|(p.y > sy*p.z?32u:0u));}
        bool commonColor=g->matList.numMaterials>0&&g->matList.materials[0];
        for(int i=1;commonColor&&i<g->matList.numMaterials;++i){auto a=g->matList.materials[0],b=g->matList.materials[i];commonColor=b&&a->color.red==b->color.red&&a->color.green==b->color.green&&a->color.blue==b->color.blue&&a->color.alpha==b->color.alpha&&(!lit||(a->surfaceProps.ambient==b->surfaceProps.ambient&&a->surfaceProps.diffuse==b->surfaceProps.diffuse));}
        auto commonMaterial=commonColor?g->matList.materials[0]:nullptr;
        auto vertexColorCache=commonMaterial&&lit&&!skin&&!a->clump&&(!(g->flags&rw::Geometry::NORMALS)||lightData.numDirectionals==0)?colorsFor(g,commonMaterial,lit,lightData.ambient):nullptr;
        unsigned colorCacheHits=0,colorCachePrepared=0;
        bool whitePrelight=self.whitePrelightEnabled&&!lit&&(g->flags&rw::Geometry::PRELIT)&&g->colors&&commonMaterial&&(!(g->flags&rw::Geometry::MODULATE)||(commonMaterial->color.red==255&&commonMaterial->color.green==255&&commonMaterial->color.blue==255));
        // Distinct materials may still share colour/light parameters, or a
        // run of faces may repeatedly use one material. A last-key byte per
        // shared vertex avoids recalculation without reordering any triangles.
        uint8_t materialColorKey[32]={};bool grouped=self.groupedColors&&!commonColor&&g->matList.numMaterials>0&&g->matList.numMaterials<=32;
        if(grouped)for(int i=0;i<g->matList.numMaterials;++i){auto m=g->matList.materials[i];if(!m||(lit&&(!std::isfinite(m->surfaceProps.ambient)||!std::isfinite(m->surfaceProps.diffuse))))continue;
            materialColorKey[i]=uint8_t(i+1);
            for(int j=0;j<i;++j){auto previous=g->matList.materials[j];if(!previous||!materialColorKey[j])continue;
                if(m->color.red==previous->color.red&&m->color.green==previous->color.green&&m->color.blue==previous->color.blue&&m->color.alpha==previous->color.alpha&&(!lit||(std::memcmp(&m->surfaceProps.ambient,&previous->surfaceProps.ambient,sizeof(float))==0&&std::memcmp(&m->surfaceProps.diffuse,&previous->surfaceProps.diffuse,sizeof(float))==0))){materialColorKey[i]=materialColorKey[j];break;}}
        }
        if(commonColor||grouped){try{self.colorReady.assign(g->numVertices,0);}catch(const std::bad_alloc&){commonColor=false;grouped=false;}}
        unsigned cull=unsigned(value(rw::CULLMODE));bool faces=cull==rw::CULLBACK||cull==rw::CULLFRONT;
        if(faces){self.projected.resize(g->numVertices);for(int i=0;i<g->numVertices;++i){if(selected&&!selected[i])continue;auto p=self.transformed[i].position;if(p.z>=self.projection.nearZ){float inverse=1/p.z;self.projected[i]={p.x*inverse,-p.y*inverse};}else self.projected[i]={0,0};}}
        if(reinterpret_cast<uintptr_t>(savedAlpha)&&!value(rw::ZWRITEENABLE)){
            auto distance=[&](int i){auto t=g->triangles[i];for(auto v:t.v)if(v>=g->numVertices)return -1e30f;return self.transformed[t.v[0]].position.z+self.transformed[t.v[1]].position.z+self.transformed[t.v[2]].position.z;};
            std::stable_sort(self.drawOrder.begin(),self.drawOrder.end(),[&](int a,int b){return distance(a)>distance(b);});
        }
        bool staged=self.prepareAtomics&&g->matList.numMaterials>0&&g->matList.numMaterials<=256;
        if(staged){try{self.materialDraws.resize(g->matList.numMaterials);}catch(const std::bad_alloc&){staged=false;}if(staged)for(auto& m:self.materialDraws)m.ready=false;}
        ScreenState baseState[2];int baseReady[2]={0,0};
        bool early=self.earlyVisibilityEnabled&&staged&&self.preparedScreen&&self.worldTextured&&value(rw::ZTESTENABLE)&&self.projection.focal>=1&&self.projection.focal<=10000&&SoftwareDevice::canQueryWorldVisibility(self.renderer);
        unsigned whitePrepared=0;unsigned commonAlpha=whitePrelight&&(g->flags&rw::Geometry::MODULATE)?commonMaterial->color.alpha:255;
        for(int i:self.drawOrder){auto t=g->triangles[i];if(t.matId>=g->matList.numMaterials){reject(__LINE__);continue;}auto mat=g->matList.materials[t.matId];
            if(!mat){reject(__LINE__);continue;}
            if(t.v[0]<g->numVertices&&t.v[1]<g->numVertices&&t.v[2]<g->numVertices&&(self.clipCodes[t.v[0]]&self.clipCodes[t.v[1]]&self.clipCodes[t.v[2]])){++self.earlyCulls;++self.triangles3;continue;}
            bool faceVerified=false;
            if(faces&&t.v[0]<g->numVertices&&t.v[1]<g->numVertices&&t.v[2]<g->numVertices&&self.transformed[t.v[0]].position.z>=self.projection.nearZ&&self.transformed[t.v[1]].position.z>=self.projection.nearZ&&self.transformed[t.v[2]].position.z>=self.projection.nearZ){auto a=self.projected[t.v[0]],b=self.projected[t.v[1]],c=self.projected[t.v[2]];
                float first=(b.u-a.u)*(c.v-a.v),second=(b.v-a.v)*(c.u-a.u),area=first-second;
                // Nearly degenerate/nonfinite projections use the original path.
                if(std::isfinite(area)&&std::fabs(area)>1e-6f*(1+std::fabs(first)+std::fabs(second))){if((cull==rw::CULLBACK&&area<0)||(cull==rw::CULLFRONT&&area>0)){++self.backfaceCulls;++self.triangles3;continue;}faceVerified=true;}}
            WorldTriangleSetup preparedVisibility;const WorldTriangleSetup* setup=nullptr;
            if(early&&mat->texture&&mat->texture->raster&&t.v[0]<g->numVertices&&t.v[1]<g->numVertices&&t.v[2]<g->numVertices&&!(self.clipCodes[t.v[0]]|self.clipCodes[t.v[1]]|self.clipCodes[t.v[2]])){
                int result=earlyTriangle(t,preparedVisibility);if(result==2){++self.triangles3;continue;}if(result==1)setup=&preparedVisibility;
            }
            const uint8_t colorKey=commonColor?1:grouped?materialColorKey[t.matId]:0;
            bool valid=true;
            for(int j=0;j<3;++j){if(t.v[j]>=g->numVertices){valid=false;break;}
                if(colorKey&&self.colorReady[t.v[j]]==colorKey){++self.colorReused;if(grouped)++self.groupedColorReused;continue;}
                auto color=mat->color;
                    if(vertexColorCache&&vertexColorCache->vertexColorReady[t.v[j]]){color=vertexColorCache->vertexColors[t.v[j]];unsigned alpha=(g->flags&rw::Geometry::MODULATE)?mat->color.alpha:255;if(alpha!=255)color.alpha=uint8_t(unsigned(color.alpha)*alpha/255);++colorCacheHits;}
                    else if(whitePrelight){auto prelit=g->colors[t.v[j]];color=prelit;if(commonAlpha!=255)color.alpha=uint8_t(unsigned(prelit.alpha)*commonAlpha/255);++whitePrepared;}
                    else if(lit){
                    if(!std::isfinite(mat->surfaceProps.ambient)||!std::isfinite(mat->surfaceProps.diffuse)){reject(__LINE__);valid=false;break;}
                    rw::RGBA base=(g->flags&rw::Geometry::PRELIT)&&g->colors?g->colors[t.v[j]]:rw::RGBA{0,0,0,255};
                    rw::RGBA material=(g->flags&rw::Geometry::MODULATE)?mat->color:rw::RGBA{255,255,255,255};auto light=self.directLight.empty()?Vec3{0,0,0}:self.directLight[t.v[j]];
                    auto channel=[&](unsigned prelit,float ambient,float direct,unsigned tint){float v=prelit/255.f+ambient*mat->surfaceProps.ambient+direct*mat->surfaceProps.diffuse;return uint8_t(std::max(0.f,std::min(1.f,v))*tint+.5f);};
                    color={channel(base.red,lightData.ambient.red,light.x,material.red),channel(base.green,lightData.ambient.green,light.y,material.green),channel(base.blue,lightData.ambient.blue,light.z,material.blue),uint8_t(unsigned(base.alpha)*material.alpha/255)};
                }else if((g->flags&rw::Geometry::PRELIT)&&g->colors){auto prelit=g->colors[t.v[j]];
                    if(g->flags&rw::Geometry::MODULATE){color.red=uint8_t(unsigned(color.red)*prelit.red/255);color.green=uint8_t(unsigned(color.green)*prelit.green/255);color.blue=uint8_t(unsigned(color.blue)*prelit.blue/255);color.alpha=uint8_t(unsigned(color.alpha)*prelit.alpha/255);}else color=prelit;}
                if(vertexColorCache&&!vertexColorCache->vertexColorReady[t.v[j]]){vertexColorCache->vertexColors[t.v[j]]=color;vertexColorCache->vertexColors[t.v[j]].alpha=g->colors[t.v[j]].alpha;vertexColorCache->vertexColorReady[t.v[j]]=1;++colorCachePrepared;}
                self.transformed[t.v[j]].setColor(color.red,color.green,color.blue,color.alpha);if(colorKey)self.colorReady[t.v[j]]=colorKey;}
            if(!valid){reject(__LINE__);continue;}
            auto tex=self.worldTextured?mat->texture:nullptr;
            bool alpha=reinterpret_cast<uintptr_t>(savedAlpha)!=0;
            for(int j=0;j<3;++j)alpha|=self.transformed[t.v[j]].a!=255;
            if(staged){auto& entry=self.materialDraws[t.matId];
                if(!entry.ready){entry.ready=true;entry.valid=true;entry.texture=tex&&tex->raster?texture(tex->raster):nullptr;
                    if(tex){entry.addressU=uint8_t(tex->getAddressU());entry.addressV=uint8_t(tex->getAddressV());if(tex->raster&&!entry.texture)entry.valid=false;}else{entry.addressU=uint8_t(reinterpret_cast<uintptr_t>(savedU));entry.addressV=uint8_t(reinterpret_cast<uintptr_t>(savedV));}
                    if(entry.addressU<1||entry.addressU>4||entry.addressV<1||entry.addressV>4){reject(__LINE__);entry.valid=false;}++self.materialPrepared;
                }else ++self.materialReused;
                if(!entry.valid)continue;
                if(!baseReady[alpha])baseReady[alpha]=screenState(baseState[alpha],alpha,entry.addressU,entry.addressV)?1:-1;
                if(baseReady[alpha]<0)continue;
                ScreenState state=baseState[alpha];state.addressU=entry.addressU;state.addressV=entry.addressV;
                triangle3Checked(t.v[0],t.v[1],t.v[2],faceVerified,true,&state,entry.texture,setup);
            }else{
                state(rw::TEXTURERASTER,tex?tex->raster:nullptr);
                if(tex){state(rw::TEXTUREADDRESSU,reinterpret_cast<void*>(uintptr_t(tex->getAddressU())));state(rw::TEXTUREADDRESSV,reinterpret_cast<void*>(uintptr_t(tex->getAddressV())));}
                state(rw::VERTEXALPHA,reinterpret_cast<void*>(uintptr_t(alpha)));
                triangle3Checked(t.v[0],t.v[1],t.v[2],faceVerified,true);
            }
        }self.cachedColors+=colorCacheHits;self.preparedColors+=colorCachePrepared;self.whitePrelightCount+=whitePrepared;if(self.transferEnabled&&!direct)self.transformed.swap(self.atomicVertices);self.transformed.swap(self.atomicScratch);state(rw::TEXTURERASTER,saved);state(rw::TEXTUREADDRESSU,savedU);state(rw::TEXTUREADDRESSV,savedV);state(rw::VERTEXALPHA,savedAlpha);
    }

    void initialize(bool (*attachPlugins)()){gameCompat=attachPlugins!=nullptr;clusterEnabled=gameCompat;if(!rw::Engine::init())throw std::runtime_error("librw init failed");
        if(attachPlugins&&!attachPlugins())throw std::runtime_error("Game plugin attach failed");
        offset=rw::Raster::registerPlugin(sizeof(RasterData*),0x0253f2a0,rasterCtor,rasterDtor,nullptr);
        geometryOffset=rw::Geometry::registerPlugin(sizeof(GeometryCache*),0x0253f2a1,geometryCtor,geometryDtor,geometryCopy);
        if(offset<0||geometryOffset<0||!rw::Engine::open(nullptr))throw std::runtime_error("librw plugin/open failed");
        rw::engine->device={0,1,beginUpdate,endUpdate,clear,show,renderFast,state,get,line2,triangle2,primitive2,indexed2,transform3,primitive3,indexed3,end3,system};
        auto driver=rw::engine->driver[rw::PLATFORM_NULL];driver->rasterCreate=create;driver->rasterLock=lock;driver->rasterUnlock=unlock;
        driver->rasterLockPalette=palette;driver->rasterUnlockPalette=paletteEnd;driver->rasterNumLevels=levels;
        driver->imageFindRasterFormat=findFormat;driver->rasterFromImage=fromImage;driver->rasterToImage=toImage;
        pipeline=rw::ObjPipeline::create();pipeline->impl.render=atomic;driver->defaultPipeline=pipeline;
        if(!rw::Engine::start())throw std::runtime_error("librw start failed");
        // Preserve MatFX ownership/plugin data, draw its base pass on the CPU.
        if(gameCompat&&rw::matFXGlobals.dummypipe)rw::matFXGlobals.dummypipe->impl.render=atomic;
        if(gameCompat&&rw::skinGlobals.dummypipe)rw::skinGlobals.dummypipe->impl.render=atomic;
        rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);
        rw::SetRenderState(rw::TEXTUREADDRESS,3);rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::NEAREST);
        rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);
        rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,128);
        world=rw::World::create();ambient=rw::Light::create(rw::Light::AMBIENT);sun=rw::Light::create(rw::Light::DIRECTIONAL);sunFrame=rw::Frame::create();
        if(!world||!ambient||!sun||!sunFrame)throw std::runtime_error("CPU world/light allocation failed");
        ambient->setColor(.25f,.26f,.30f);sun->setColor(.75f,.70f,.60f);rw::Matrix direction;direction.setIdentity();direction.at={-.35f,-.85f,-.39f};direction.flags=0;sunFrame->transform(&direction,rw::COMBINEREPLACE);sun->setFrame(sunFrame);world->addLight(ambient);world->addLight(sun);
        cam=rw::Camera::create();world->addCamera(cam);frame=rw::Frame::create();cam->setFrame(frame);
        image=rw::Raster::create(320,240,16,rw::Raster::CAMERA|rw::Raster::C565);depth=rw::Raster::create(320,240,32,rw::Raster::ZBUFFER|rw::Raster::D32);
        if(!image||!depth)throw std::runtime_error("CPU camera raster failed");cam->frameBuffer=image;cam->zBuffer=depth;
        const int widths[3]={320,320,74},heights[3]={23,23,74};
        for(int i=0;i<3;++i){overlays[i]=rw::Raster::create(widths[i],heights[i],16,rw::Raster::TEXTURE|rw::Raster::C565);if(!overlays[i])throw std::runtime_error("CPU HUD raster failed");}
    }
    ~Impl(){if(cam){if(updating)cam->endUpdate();if(world)world->removeCamera(cam);cam->setFrame(nullptr);cam->destroy();}if(frame)frame->destroy();
        if(ambient){if(ambient->world)ambient->world->removeLight(ambient);ambient->destroy();}if(sun){if(sun->world)sun->world->removeLight(sun);sun->setFrame(nullptr);sun->destroy();}if(sunFrame)sunFrame->destroy();if(world)world->destroy();
        for(auto raster:overlays)if(raster)raster->destroy();if(image)image->destroy();if(depth)depth->destroy();
        if(pipeline)pipeline->destroy();
        if(rw::Engine::state==rw::Engine::Started)rw::Engine::stop();
        if(rw::Engine::state==rw::Engine::Opened)rw::Engine::close();
        if(rw::Engine::state==rw::Engine::Initialized)rw::Engine::term();active=nullptr;offset=geometryOffset=-1;
    }
};
SoftwareDevice::Impl* SoftwareDevice::Impl::active=nullptr;
int SoftwareDevice::Impl::offset=-1;
int SoftwareDevice::Impl::geometryOffset=-1;
SoftwareDevice::SoftwareDevice(Renderer& r,bool (*attachPlugins)()):impl_(new Impl(r)){impl_->initialize(attachPlugins);}
SoftwareDevice::~SoftwareDevice()=default;
void SoftwareDevice::begin(const Camera& camera){if(impl_->updating)throw std::runtime_error("Nested software camera begin");
    rw::Matrix m;m.setIdentity();float co=std::cos(camera.yaw),si=std::sin(camera.yaw);
    m.right={-co,0,si};m.up={0,1,0};m.at={si,0,co};m.pos={camera.position.x,camera.position.y,camera.position.z};m.flags=0;
    impl_->frame->transform(&m,rw::COMBINEREPLACE);impl_->cam->setNearPlane(camera.nearZ);impl_->cam->setFarPlane(camera.farZ);
    rw::V2d window{160/camera.focal,120/camera.focal};impl_->cam->setViewWindow(&window);impl_->cam->beginUpdate();
    if(!impl_->updating)throw std::runtime_error("Invalid software camera");
}
void SoftwareDevice::end(){impl_->cam->endUpdate();}
void SoftwareDevice::checkpoint(const CheckpointRun& run,const Camera&,bool glow){
    auto triangles=run.marker();std::vector<rw::cpu::Im3DVertex> vertices;vertices.reserve(triangles.size()*3);
    for(auto tri:triangles)for(auto p:tri.v){rw::cpu::Im3DVertex vertex;vertex.position={p.x,p.y,p.z};
        unsigned r=(tri.color>>11)&31,g=(tri.color>>5)&63,b=tri.color&31;vertex.setColor((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2),255);vertices.push_back(vertex);}
    if(vertices.empty())return;rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);rw::SetRenderState(rw::VERTEXALPHA,0);
    rw::im3d::Transform(vertices.data(),int(vertices.size()),nullptr,rw::im3d::ALLOPAQUE|rw::im3d::VERTEXXYZ|rw::im3d::VERTEXRGBA);
    rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();
    if(glow)checkpointGlow(run);
}
void SoftwareDevice::checkpointGlow(const CheckpointRun& run){
    if(run.finished())return;
    // A small translucent gradient marks the next gate. Draw after opaque
    // objects with depth testing but no depth writes; HUD remains a later pass.
    const int settings[]={rw::ZWRITEENABLE,rw::VERTEXALPHA,rw::SRCBLEND,rw::DESTBLEND,rw::ALPHATESTFUNC,rw::CULLMODE};
    void* saved[6];for(int i=0;i<6;++i)saved[i]=rw::GetRenderStatePtr(settings[i]);
    rw::SetRenderState(rw::ZWRITEENABLE,0);rw::SetRenderState(rw::VERTEXALPHA,1);
    rw::SetRenderState(rw::SRCBLEND,rw::BLENDSRCALPHA);rw::SetRenderState(rw::DESTBLEND,rw::BLENDONE);
    rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);
    auto center=run.route()[run.completed()];rw::cpu::Im3DVertex sheet[4];
    for(int i=0;i<4;++i){sheet[i].position={center.x+((i&1)?6.f:-6.f),center.y+.15f+((i>>1)?3.f:0.f),center.z};sheet[i].setColor(255,(i>>1)?170:215,35,(i>>1)?0:95);}
    uint16_t indices[6]={0,1,2,2,1,3};rw::im3d::Transform(sheet,4,nullptr,rw::im3d::EVERYTHING);
    rw::im3d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,indices,6);rw::im3d::End();
    for(int i=0;i<6;++i)rw::SetRenderStatePtr(settings[i],saved[i]);
}
void SoftwareDevice::hud(const std::vector<uint16_t>& frame){if(frame.size()!=320*240)throw std::runtime_error("Invalid HUD image size");
    rw::SetRenderState(rw::ZTESTENABLE,0);rw::SetRenderState(rw::ZWRITEENABLE,0);rw::SetRenderState(rw::VERTEXALPHA,0);
    const int x[3]={0,0,243},y[3]={0,217,139};
    for(int i=0;i<3;++i){auto raster=impl_->overlays[i];auto pixels=raster->lock(0,rw::Raster::LOCKWRITE);if(!pixels)throw std::runtime_error("HUD raster lock failed");
        for(int row=0;row<raster->height;++row)std::memcpy(pixels+row*raster->stride,frame.data()+(y[i]+row)*320+x[i],size_t(raster->width)*2);raster->unlock(0);
        rw::SetRenderStatePtr(rw::TEXTURERASTER,raster);rw::cpu::Im2DVertex v[4];
        for(int j=0;j<4;++j){v[j].x=float(x[i]+(j&1)*raster->width);v[j].y=float(y[i]+(j>>1)*raster->height);v[j].u=float(j&1);v[j].v=float(j>>1);}
        uint16_t indices[6]={0,1,2,2,1,3};rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,v,4,indices,6);
    }rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);
}
uint64_t SoftwareDevice::unsupported() const{return impl_->failures;}
uint64_t SoftwareDevice::im2dTriangles() const{return impl_->triangles2;}
uint64_t SoftwareDevice::im3dTriangles() const{return impl_->triangles3;}
rw::World* SoftwareDevice::world() const{return impl_->world;}
void SoftwareDevice::highQuality(bool enabled){impl_->quality=enabled;}
void SoftwareDevice::distanceShading(bool enabled){impl_->distanceShadingEnabled=enabled;}
uint64_t SoftwareDevice::distanceShadingTriangles()const{return impl_->distanceShaded;}
void SoftwareDevice::ambientShortcut(bool enabled){impl_->ambientShortcutEnabled=enabled;}
uint64_t SoftwareDevice::ambientVerticesSkipped()const{return impl_->ambientSkipped;}
void SoftwareDevice::whitePrelight(bool enabled){impl_->whitePrelightEnabled=enabled;}
uint64_t SoftwareDevice::whitePrelightVertices()const{return impl_->whitePrelightCount;}
void SoftwareDevice::lighting(Vec3 ambient,Vec3 sun,Vec3 direction){
    for(float v:{ambient.x,ambient.y,ambient.z,sun.x,sun.y,sun.z})if(!std::isfinite(v)||v<0||v>4)throw std::runtime_error("Invalid lighting color");
    float len=std::sqrt(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);if(!std::isfinite(len)||len<1e-8f)throw std::runtime_error("Invalid sun direction");
    impl_->ambient->setColor(ambient.x,ambient.y,ambient.z);impl_->sun->setColor(sun.x,sun.y,sun.z);rw::Matrix matrix;matrix.setIdentity();matrix.at={direction.x/len,direction.y/len,direction.z/len};matrix.flags=0;impl_->sunFrame->transform(&matrix,rw::COMBINEREPLACE);
}
void SoftwareDevice::worldTextures(bool enabled){impl_->worldTextured=enabled;}
uint64_t SoftwareDevice::shadedTriangles() const{return impl_->shaded;}
uint64_t SoftwareDevice::atomicCalls() const{return impl_->atomics;}
rw::Camera* SoftwareDevice::camera() const{return impl_->cam;}
}

namespace soft {uint64_t SoftwareDevice::matFxCalls() const{return impl_->matFx;}}

namespace soft {uint64_t SoftwareDevice::skinCalls() const{return impl_->skins;} uint64_t SoftwareDevice::skinnedVertices() const{return impl_->skinVertices;}}

namespace soft {uint64_t SoftwareDevice::culledTriangles() const{return impl_->earlyCulls;}}

namespace soft {uint64_t SoftwareDevice::backfaceTriangles() const{return impl_->backfaceCulls;}}

namespace soft {uint64_t SoftwareDevice::reusedGroupedColors()const{return impl_->groupedColorReused;}
uint64_t SoftwareDevice::reusedColors() const{return impl_->colorReused;}}

namespace soft {
void SoftwareDevice::traceTextures(bool enabled){impl_->traceTextures=enabled;}
void SoftwareDevice::normalCache(bool enabled){impl_->normalCacheEnabled=enabled;}
uint64_t SoftwareDevice::cachedNormalsReused()const{return impl_->normalsReused;}
uint64_t SoftwareDevice::cachedNormalsPrepared()const{return impl_->normalsPrepared;}
size_t SoftwareDevice::normalCacheBytes()const{return impl_->normalCacheBytes;}
void SoftwareDevice::directAtomic(bool enabled){impl_->directAtomicEnabled=enabled;}
uint64_t SoftwareDevice::directAtomicVertices()const{return impl_->directVertexCount;}
void SoftwareDevice::atomicTransfer(bool enabled){impl_->transferEnabled=enabled;}
void SoftwareDevice::textureAlpha(bool enabled){impl_->textureAlphaEnabled=enabled;}
uint64_t SoftwareDevice::transferredAtomicVertices()const{return impl_->transferredVertices;}
uint64_t SoftwareDevice::textureAlphaTriangles()const{return impl_->textureAlphaTriangles;}
void SoftwareDevice::effectFiltering(bool enabled){impl_->filteredEffects=enabled;}
void SoftwareDevice::bakedLighting(bool enabled){impl_->preservePrelight=enabled;}
void SoftwareDevice::printTextureInfo(rw::Raster* raster,const char* label){auto t=Impl::texture(raster);if(!t)return;unsigned min=255,max=0,zero=0;for(unsigned a:t->alpha){min=std::min(min,a);max=std::max(max,a);zero+=!a;}std::printf("TEXTURE %s width=%u height=%u opaque=%d uniform=%d alpha_min=%u alpha_max=%u alpha_zero=%u ink=%d,%d,%d,%d\n",label,t->width,t->height,t->decodedOpaque,t->decodedUniform,min,max,zero,t->inkX0,t->inkY0,t->inkX1,t->inkY1);}
void SoftwareDevice::hierarchicalBounds(bool enabled){impl_->hierarchyEnabled=enabled;}
uint64_t SoftwareDevice::modelBoundsTests()const{return impl_->modelTests;}
uint64_t SoftwareDevice::modelBoundsCulls()const{return impl_->modelCulls;}
uint64_t SoftwareDevice::fullyInsideModels()const{return impl_->insideModels;}
uint64_t SoftwareDevice::savedPlaneTests()const{return impl_->planeTestsSaved;}
bool SoftwareDevice::canQueryWorldVisibility(Renderer& renderer){return renderer.canQueryWorldVisibility();}
int SoftwareDevice::queryWorldVisibility(Renderer& renderer,const Vec3* screen,WorldTriangleSetup& setup,bool wide){return renderer.queryWorldVisibility(screen,setup,wide);}
void SoftwareDevice::earlyWideVisibility(bool enabled){impl_->earlyWideEnabled=enabled;}
void SoftwareDevice::earlyVisibility(bool enabled){impl_->earlyVisibilityEnabled=enabled;}
uint64_t SoftwareDevice::earlyVisibilityQueries()const{return impl_->earlyQueries;}
uint64_t SoftwareDevice::earlyVisibilityHidden()const{return impl_->earlyHidden;}
void SoftwareDevice::modelClusters(bool enabled){impl_->clusterEnabled=enabled;}
uint64_t SoftwareDevice::culledClusters() const{return impl_->clusterCulls;}
uint64_t SoftwareDevice::clusterTests() const{return impl_->clusterTests;}
uint64_t SoftwareDevice::clusterTriangles() const{return impl_->clusterTriangles;}
uint64_t SoftwareDevice::skippedVertices() const{return impl_->verticesSkipped;}
uint64_t SoftwareDevice::clusterRebuilds() const{return impl_->cacheRebuilds;}
uint64_t SoftwareDevice::dynamicGeometryFallbacks() const{return impl_->dynamicFallbacks;}
size_t SoftwareDevice::clusterCacheBytes() const{return impl_->cacheBytes;}
}

namespace soft {
void SoftwareDevice::groupedColors(bool enabled){impl_->groupedColors=enabled;}
void SoftwareDevice::preparedScreenValidation(bool enabled){impl_->preparedScreen=enabled;}
void SoftwareDevice::preparedAtomics(bool enabled){impl_->prepareAtomics=enabled;}
uint64_t SoftwareDevice::preparedMaterials() const{return impl_->materialPrepared;}
uint64_t SoftwareDevice::reusedMaterials() const{return impl_->materialReused;}
uint64_t SoftwareDevice::reusedFlatProjections() const{return impl_->flatProjectionReused;}
}

namespace soft {
void SoftwareDevice::drawPreparedScreen(Renderer& renderer,const ScreenVertex* vertices,const Texture* texture,const ScreenState& state,const WorldTriangleSetup* setup){renderer.screenTriangleChecked(vertices,texture,state,true,setup);}
void SoftwareDevice::drawPreparedFlat(Renderer& renderer,const ScreenVertex* vertices,uint16_t tint,const Texture& texture,uint8_t addressU,uint8_t addressV,unsigned alphaRef,const WorldTriangleSetup* setup){renderer.texturedProjectedPrepared(vertices,tint,texture,addressU,addressV,alphaRef,setup);}
}

namespace soft {void SoftwareDevice::flatFading(bool enabled){impl_->flatFadingEnabled=enabled;}uint64_t SoftwareDevice::flatFadingTriangles()const{return impl_->flattenedFading;}}

void soft::SoftwareDevice::fastClusterBounds(bool enabled){impl_->fastClusterBoundsEnabled=enabled;}

void soft::SoftwareDevice::worldLodSelector(rw::Geometry* (*selector)(rw::Atomic*)){impl_->worldLodSelector=selector;}
uint64_t soft::SoftwareDevice::worldLodAtomics()const{return impl_->worldLodCalls;}
uint64_t soft::SoftwareDevice::worldLodInputSaved()const{return impl_->worldLodSaved;}
bool soft::SoftwareDevice::geometryBounds(rw::Geometry* g,ModelBounds& b){if(!g||g->numVertices<1||g->numTriangles<1||g->numMorphTargets!=1||!g->morphTargets[0].vertices||!g->triangles||(g->flags&rw::Geometry::NATIVE))return false;auto cache=Impl::boundsFor(g);if(!cache||cache->dynamic||!cache->whole.valid)return false;b=cache->whole;return true;}
bool soft::SoftwareDevice::opaqueGeometry(rw::Geometry* g,bool ignoreMaterialAlpha){
 if(!g||g->numVertices<1||g->numTriangles<1||g->numMorphTargets!=1||!g->morphTargets[0].vertices||(g->flags&rw::Geometry::NATIVE))return false;
 if(rw::skinGlobals.geoOffset>0&&rw::Skin::get(g))return false;
 auto cache=Impl::boundsFor(g);if(!cache||cache->dynamic)return false;bool prelit=(g->flags&rw::Geometry::PRELIT)&&g->colors;
 if(!cache->opacityReady||cache->opacityColors!=g->colors||cache->opacityPrelit!=prelit||(g->lockedSinceInst&rw::Geometry::LOCKPRELIGHT)){
  if((g->lockedSinceInst&rw::Geometry::LOCKPRELIGHT)&&cache->simplified){Impl::active->simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();cache->simplified=nullptr;cache->simplifiedCharge=0;cache->simplifyAttempted=false;}
  if(g->lockedSinceInst&rw::Geometry::LOCKPRELIGHT){cache->vertexColorValid=false;cache->vertexColorPrimed=false;}
  cache->opacityReady=true;cache->opacityColors=g->colors;cache->opacityPrelit=prelit;cache->opaqueVertices=true;
  if(prelit)for(int i=0;i<g->numVertices;++i)if(g->colors[i].alpha!=255){cache->opaqueVertices=false;break;}
  // CPU colour preparation reads prelight every draw; only this cross-frame
  // opacity certificate consumes its dirty flag. Normals/UV flags are retained.
  g->lockedSinceInst&=~rw::Geometry::LOCKPRELIGHT;
 }
 if(!cache->opaqueVertices||g->matList.numMaterials<1)return false;
 for(int i=0;i<g->matList.numMaterials;++i){auto m=g->matList.materials[i];if(!m||(!ignoreMaterialAlpha&&m->color.alpha!=255))return false;if(m->texture&&m->texture->raster){auto t=Impl::texture(m->texture->raster);if(!t||!t->decodedOpaque)return false;}}
 return true;
}

void soft::SoftwareDevice::simplificationBudget(unsigned count){impl_->simplifyBudget=count;}
uint64_t soft::SoftwareDevice::simplifiedWorldBuilds()const{return impl_->simplifiedBuilds;}
size_t soft::SoftwareDevice::simplifiedWorldBytes()const{return impl_->simplifiedBytes;}
double soft::SoftwareDevice::simplificationPeakMs()const{return impl_->simplifyPeakMs;}
rw::Geometry* soft::SoftwareDevice::simplifiedWorld(rw::Geometry* g,const Vec3* protectedCamera,float protectedRadius,bool intermediate){auto& self=*Impl::active;
 unsigned changed=g->lockedSinceInst&(rw::Geometry::LOCKPRELIGHT|rw::Geometry::LOCKNORMALS|rw::Geometry::LOCKTEXCOORDSALL);
 auto cache=Impl::boundsFor(g);if(!cache||cache->dynamic)return g;
 if(cache->simplifiedIntermediate!=intermediate){if(cache->simplified){self.simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();cache->simplified=nullptr;cache->simplifiedCharge=0;}std::vector<ModelBounds>().swap(cache->changedBounds);cache->simplifyAttempted=false;cache->hasSimplifyCamera=false;cache->simplifiedIntermediate=intermediate;}
 if(protectedCamera&&cache->hasSimplifyCamera&&!cache->simplified){auto c=cache->simplifyCamera;float x=c.x-protectedCamera->x,y=c.y-protectedCamera->y,z=c.z-protectedCamera->z;if(x*x+y*y+z*z>2500)cache->simplifyAttempted=false;}
 if(changed){if(changed&rw::Geometry::LOCKPRELIGHT){cache->vertexColorValid=false;cache->vertexColorPrimed=false;}if(cache->simplified){self.simplifiedBytes-=cache->simplifiedCharge;cache->simplified->destroy();cache->simplified=nullptr;cache->simplifiedCharge=0;std::vector<ModelBounds>().swap(cache->changedBounds);}cache->simplifyAttempted=false;std::fill(cache->normalReady.begin(),cache->normalReady.end(),0);}
 if(!worldGeometrySuitable(g))return g;
 if(cache->simplified){unsigned dynamic=rw::Geometry::MODULATE|rw::Geometry::LIGHT;cache->simplified->flags=(cache->simplified->flags&~dynamic)|(g->flags&dynamic);return cache->simplified;}if(cache->simplifyAttempted||!self.simplifyBudget)return g;
 --self.simplifyBudget;cache->simplifyAttempted=true;if(protectedCamera){cache->simplifyCamera=*protectedCamera;cache->hasSimplifyCamera=true;}
 auto start=std::chrono::steady_clock::now();SimplifiedWorld result;try{result=simplifyWorldGeometry(g,Impl::simplifiedLimit-self.simplifiedBytes,protectedCamera,protectedRadius,intermediate);}catch(const std::bad_alloc&){result.reason=10;}if(std::getenv("GTA3_SIMPLIFY_TRACE"))std::printf("SIMPLIFY BUILD vertices=%d triangles=%u materials=%d output=%u error=%.4f reason=%u\n",g->numVertices,result.input,g->matList.numMaterials,result.output,result.error,result.reason);self.simplifyPeakMs=std::max(self.simplifyPeakMs,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
 // Attributes are consumed by CPU preparation; invalidate the optional normal
 // cache before consuming NORMALS. No GPU instancer is used on this platform.
 g->lockedSinceInst&=~(rw::Geometry::LOCKPRELIGHT|rw::Geometry::LOCKNORMALS|rw::Geometry::LOCKTEXCOORDSALL);
 if(result.geometry){cache->simplifiedError=result.error;cache->changedBounds=std::move(result.changedBounds);cache->simplified=result.geometry;cache->simplifiedCharge=result.bytes;self.simplifiedBytes+=result.bytes;++self.simplifiedBuilds;}
 return cache->simplified?cache->simplified:g;
}

bool soft::SoftwareDevice::worldGeometrySuitable(rw::Geometry* g){
 if(!g||g->numVertices<16||g->numTriangles<96||g->numTriangles>4096||g->numMorphTargets!=1||!g->morphTargets[0].vertices||!g->triangles||(g->flags&rw::Geometry::NATIVE)||g->numTexCoordSets>1)return false;
 if(rw::skinGlobals.geoOffset>0&&rw::Skin::get(g))return false;
 auto cache=Impl::boundsFor(g);if(!cache||cache->dynamic||!cache->whole.valid)return false;
 if(g->matList.numMaterials<1||g->matList.numMaterials>32)return false;
 for(int i=0;i<g->matList.numMaterials;++i){auto m=g->matList.materials[i];if(!m)return false;}
 return true;
}

bool soft::SoftwareDevice::simplifiedSafeAt(rw::Geometry* g,Vec3 c,float distance){auto cache=Impl::geometryCache(g);return cache&&cache->simplified&&safeChangedGeometry(cache->changedBounds,c,distance);}

bool soft::SoftwareDevice::simplifiedWorldRejected(rw::Geometry* g,bool intermediate){auto cache=g?Impl::geometryCache(g):nullptr;return cache&&cache->simplifiedIntermediate==intermediate&&cache->simplifyAttempted&&!cache->simplified&&!(g->lockedSinceInst&(rw::Geometry::LOCKVERTICES|rw::Geometry::LOCKPOLYGONS|rw::Geometry::LOCKPRELIGHT|rw::Geometry::LOCKNORMALS|rw::Geometry::LOCKTEXCOORDSALL));}

float soft::SoftwareDevice::simplifiedError(rw::Geometry* g){auto cache=Impl::geometryCache(g);return cache?cache->simplifiedError:0;}

void soft::SoftwareDevice::vertexColorCache(bool enabled){impl_->vertexColorCacheEnabled=enabled;}
uint64_t soft::SoftwareDevice::cachedVertexColors()const{return impl_->cachedColors;}
uint64_t soft::SoftwareDevice::preparedVertexColors()const{return impl_->preparedColors;}
size_t soft::SoftwareDevice::vertexColorCacheBytes()const{return impl_->vertexColorBytes;}

namespace soft {void SoftwareDevice::farNormalLighting(bool enabled){impl_->farNormal=enabled;}bool SoftwareDevice::farNormalLighting()const{return impl_->farNormal;}uint64_t SoftwareDevice::farNormalVertices()const{return impl_->farNormals;}}

namespace soft {void SoftwareDevice::farSurfaceLighting(bool enabled){impl_->farSurface=enabled;}bool SoftwareDevice::farSurfaceLighting()const{return impl_->farSurface;}}
