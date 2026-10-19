#include "raster_span.hpp"
#include "hidden_samples.hpp"
#include "packed_world.hpp"
#include "soft_renderer.hpp"
#include <algorithm>
#include <cmath>
namespace soft {
namespace {
struct Vertex {Vec3 p;Vec2 uv;};
int texel(float v,int size,uint8_t mode) {
    // Exported dimensions are powers of two. Convert before wrapping so
    // negative/repeated coordinates preserve the RenderWare addressing mode.
    float f=v*size;int n=int(f);if(float(n)>f)--n;
    if(mode==2){n&=2*size-1;return n<size?n:2*size-1-n;}
    if(mode==3||mode==4)return std::max(0,std::min(size-1,n));
    return n&(size-1);
}
uint16_t modulate(uint16_t tex,uint16_t tint) {
    unsigned r=(((tex>>11)&31)*((tint>>11)&31)+15)/31;
    unsigned g=(((tex>>5)&63)*((tint>>5)&63)+31)/63;
    unsigned b=((tex&31)*(tint&31)+15)/31;
    return uint16_t((r<<11)|(g<<5)|b);
}
}
void Renderer::texturedTriangle(const Vec3* p,const Vec2* uv,uint16_t tint,const Texture& tex,
                               uint8_t addressU,uint8_t addressV,const Camera& c,unsigned alphaRef) {
    ++stats_.submitted;
    if(alphaRef>255||!tex.width||!tex.height||tex.width>128||tex.height>128||
       (tex.width&(tex.width-1))||(tex.height&(tex.height-1))||
       tex.pixels.size()!=size_t(tex.width)*tex.height||tex.alpha.size()!=tex.pixels.size()||
       !std::isfinite(c.nearZ)||!std::isfinite(c.farZ)||!std::isfinite(c.focal)||
       c.nearZ<.001f||c.farZ<=c.nearZ||c.farZ>10000||c.focal<1||c.focal>10000){++stats_.rejected;return;}
    Vertex a[16],b[16];int n=3;
    for(int i=0;i<3;++i){
        if(!std::isfinite(p[i].x)||!std::isfinite(p[i].y)||!std::isfinite(p[i].z)||
           std::fabs(p[i].x)>1e6f||std::fabs(p[i].y)>1e6f||std::fabs(p[i].z)>1e6f||
           !std::isfinite(uv[i].u)||!std::isfinite(uv[i].v)||std::fabs(uv[i].u)>1e5f||std::fabs(uv[i].v)>1e5f)
            {++stats_.rejected;return;}
        a[i]={p[i],uv[i]};
    }
    float sx=width*.5f/c.focal,sy=height*.5f/c.focal;
    unsigned combined=0,common=63;for(int i=0;i<3;++i){unsigned code=frustumCode(p[i],c);combined|=code;common&=code;}
    if(common){++stats_.rejected;return;}
    for(int plane=0;plane<6&&n&&combined;++plane){
        auto distance=[&](Vec3 v){switch(plane){case 0:return v.z-c.nearZ;case 1:return c.farZ-v.z;
            case 2:return v.x+sx*v.z;case 3:return sx*v.z-v.x;case 4:return v.y+sy*v.z;default:return sy*v.z-v.y;}};
        int m=0;auto prev=a[n-1];float dp=distance(prev.p);
        for(int i=0;i<n;++i){auto cur=a[i];float dc=distance(cur.p);
            if((dp>=0)!=(dc>=0)){float t=dp/(dp-dc);
                b[m++]={{prev.p.x+t*(cur.p.x-prev.p.x),prev.p.y+t*(cur.p.y-prev.p.y),prev.p.z+t*(cur.p.z-prev.p.z)},
                        {prev.uv.u+t*(cur.uv.u-prev.uv.u),prev.uv.v+t*(cur.uv.v-prev.uv.v)}};}
            if(dc>=0)b[m++]=cur;prev=cur;dp=dc;
        }
        n=m;std::copy(b,b+n,a);
    }
    if(n<3){++stats_.rejected;return;}
    for(int i=1;i<n-1;++i){Vec3 v[3]={a[0].p,a[i].p,a[i+1].p};Vec2 t[3]={a[0].uv,a[i].uv,a[i+1].uv};
        rasterTexture(v,t,tint,tex,addressU,addressV,c.focal,alphaRef);}
}
void Renderer::texturedProjectedTriangle(const ScreenVertex* input,uint16_t tint,const Texture& tex,uint8_t addressU,uint8_t addressV,unsigned alphaRef){
    if(alphaRef>255||!tex.width||!tex.height||tex.width>128||tex.height>128||(tex.width&(tex.width-1))||(tex.height&(tex.height-1))||tex.pixels.size()!=size_t(tex.width)*tex.height||tex.alpha.size()!=tex.pixels.size()){++stats_.submitted;++stats_.rejected;return;}
    for(unsigned i=0;i<3;++i){auto p=input[i];if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.reciprocalZ)||!std::isfinite(p.u)||!std::isfinite(p.v)||std::fabs(p.x)>1e6f||std::fabs(p.y)>1e6f||p.reciprocalZ<=0||p.reciprocalZ>1e6f||std::fabs(p.u)>1e5f||std::fabs(p.v)>1e5f){++stats_.submitted;++stats_.rejected;return;}}
    texturedProjectedPrepared(input,tint,tex,addressU,addressV,alphaRef);
}
// Private bridge from SoftwareDevice: camera, texture storage and shared
// vertices are validated before this call. No public unchecked renderer API.
void Renderer::texturedProjectedPrepared(const ScreenVertex* input,uint16_t tint,const Texture& tex,uint8_t addressU,uint8_t addressV,unsigned alphaRef,const WorldTriangleSetup* setup){
    ++stats_.submitted;Vec3 v[3];Vec2 uv[3];for(unsigned i=0;i<3;++i){auto p=input[i];v[i]=setup?setup->points[i]:Vec3{p.x/scale_,p.y/scale_,p.reciprocalZ};uv[i]={p.u*p.reciprocalZ,p.v*p.reciprocalZ};}
    rasterTextureProjected(v,uv,tint,tex,addressU,addressV,alphaRef,setup);
}
void Renderer::rasterTexture(const Vec3* points,const Vec2* uv,uint16_t tint,const Texture& tex,
                             uint8_t modeU,uint8_t modeV,float focal,unsigned alphaRef) {
    expandedValid_=false;
    Vec3 v[3];Vec2 t[3];
    for(int i=0;i<3;++i){float iz=1/points[i].z;
        v[i]={(width*.5f+focal*points[i].x*iz)/scale_,(height*.5f-focal*points[i].y*iz)/scale_,iz};t[i]={uv[i].u*iz,uv[i].v*iz};}
    rasterTextureProjected(v,t,tint,tex,modeU,modeV,alphaRef);
}
void Renderer::rasterTextureProjected(const Vec3* input,const Vec2* uvOverZ,uint16_t tint,const Texture& tex,uint8_t modeU,uint8_t modeV,unsigned alphaRef,const WorldTriangleSetup* setup){
    if(deferredActive_&&!deferredReplaying_){ScreenVertex v[3]{};for(unsigned i=0;i<3;++i){v[i].x=input[i].x;v[i].y=input[i].y;v[i].reciprocalZ=input[i].z;v[i].u=uvOverZ[i].u;v[i].v=uvOverZ[i].v;}if(deferCommand(v,&tex,ScreenState{},true,setup,1,tint,modeU,modeV,alphaRef))return;}
    expandedValid_=false;Vec3 v[3]={input[0],input[1],input[2]};Vec2 t[3]={uvOverZ[0],uvOverZ[1],uvOverZ[2]};
    auto edge=[](Vec3 p,Vec3 q,float x,float y){return (q.x-p.x)*(y-p.y)-(q.y-p.y)*(x-p.x);};
    float area;int x0,x1,y0,y1;
    if(setup){for(unsigned i=0;i<3;++i)v[i]=setup->points[i];if(setup->swapped)std::swap(t[1],t[2]);area=setup->area;x0=setup->x0;x1=setup->x1;y0=setup->y0;y1=setup->y1;}
    else{area=edge(v[0],v[1],v[2].x,v[2].y);if(std::fabs(area)<1e-5f)return;
        if(area<0){std::swap(v[1],v[2]);std::swap(t[1],t[2]);area=-area;}
        x0=std::max(0,rasterCeil(std::min({v[0].x,v[1].x,v[2].x})-.5f));x1=std::min(int(renderWidth())-1,rasterFloor(std::max({v[0].x,v[1].x,v[2].x})-.5f));
        y0=std::max(0,rasterCeil(std::min({v[0].y,v[1].y,v[2].y})-.5f));y1=std::min(int(renderHeight())-1,rasterFloor(std::max({v[0].y,v[1].y,v[2].y})-.5f));
        if(x0>x1||y0>y1)return;
    }auto a=v[0],b=v[1],c=v[2];++stats_.rasterized;
    const bool repeat=modeU==1&&modeV==1,clamp=(modeU==3||modeU==4)&&(modeV==3||modeV==4),packed=packedWorld_&&(repeat||clamp);
    TintTable* table=nullptr;
    if(!packed&&tint!=65535&&area>=256&&(x1-x0+1)*(y1-y0+1)>=256){
        table=&tintTables_[(tint^(tint>>6))&15];
        if(!table->valid||table->tint!=tint){
            for(unsigned i=0;i<32;++i){table->red[i]=uint16_t(((i*(tint>>11)+15)/31)<<11);table->blue[i]=uint16_t((i*(tint&31)+15)/31);}
            for(unsigned i=0;i<64;++i)table->green[i]=uint16_t(((i*((tint>>5)&63)+31)/63)<<5);
            table->valid=true;table->tint=tint;
        }
    }
    float inv,dx[3],dy[3],row[3],zx,zy,rz;bool tl[3];
    if(setup){inv=setup->inv;for(unsigned i=0;i<3;++i){dx[i]=setup->dx[i];dy[i]=setup->dy[i];row[i]=setup->row[i];tl[i]=setup->tl[i];}zx=setup->zx;zy=setup->zy;rz=setup->z;}
    else{inv=1/area;float ex[3]={b.y-c.y,c.y-a.y,a.y-b.y},ey[3]={c.x-b.x,a.x-c.x,b.x-a.x},er[3]={edge(b,c,x0+.5f,y0+.5f),edge(c,a,x0+.5f,y0+.5f),edge(a,b,x0+.5f,y0+.5f)};
        auto topLeft=[](Vec3 p,Vec3 q){return q.y<p.y||(q.y==p.y&&q.x>p.x);};bool et[3]={topLeft(b,c),topLeft(c,a),topLeft(a,b)};
        for(unsigned i=0;i<3;++i){dx[i]=ex[i];dy[i]=ey[i];row[i]=er[i];tl[i]=et[i];}
        zx=(dx[0]*a.z+dx[1]*b.z+dx[2]*c.z)*inv;zy=(dy[0]*a.z+dy[1]*b.z+dy[2]*c.z)*inv;rz=(row[0]*a.z+row[1]*b.z+row[2]*c.z)*inv;
    }
    if(y0>=replayLo_&&y1<replayHi_&&!(setup&&setup->depthChecked)&&smallDepth_&&(x1-x0+1)*(y1-y0+1)<256){++stats_.hiddenSampleChecks;
        if(x1-x0+1<16&&(x1-x0+1)*(y1-y0+1)<=16?hiddenSamples(depth_.data()+size_t(y0)*stride()+x0,stride(),x1-x0+1,y1-y0+1,row,dx,dy,tl,rz,zx,zy):occludedSmall(x0,y0,x1,y1,rz,zx,zy)){++stats_.hiddenSampleTriangles;return;}}
    float ux=(dx[0]*t[0].u+dx[1]*t[1].u+dx[2]*t[2].u)*inv,uy=(dy[0]*t[0].u+dy[1]*t[1].u+dy[2]*t[2].u)*inv;
    float vx=(dx[0]*t[0].v+dx[1]*t[1].v+dx[2]*t[2].v)*inv,vy=(dy[0]*t[0].v+dy[1]*t[1].v+dy[2]*t[2].v)*inv;
    float ru=(row[0]*t[0].u+row[1]*t[1].u+row[2]*t[2].u)*inv;
    float rv=(row[0]*t[0].v+row[1]*t[1].v+row[2]*t[2].v)*inv;
    if(flatOcclusion_&&!(setup&&setup->depthChecked)&&occluded(x0,y0,x1,y1,rz,zx,zy))return;
    for(;y0<replayLo_;++y0){for(unsigned i=0;i<3;++i)row[i]+=dy[i];rz+=zy;ru+=uy;rv+=vy;}
    y1=std::min(y1,replayHi_-1);if(y0>y1)return;
    const bool affineRow=zx==0;
    if(packed){const PackedTintTables* colors=tint==65535?nullptr:&PackedTintTables::get();
        PackedWorldJob job={color_.data(),depth_.data(),stride(),stats_,tex,x0,x1,y0,y1,{row[0],row[1],row[2]},{dx[0],dx[1],dx[2]},{dy[0],dy[1],dy[2]},{tl[0],tl[1],tl[2]},rz,ru,rv,zx,ux,vx,zy,uy,vy,affineRow,depthRows_,edgeRuns_,texelRuns_,packetSampler_,depthBlocks_,alphaRef,colors?colors->five+(tint>>11)*32:nullptr,colors?colors->six+((tint>>5)&63)*64:nullptr,colors?colors->five+(tint&31)*32:nullptr};
        auto before=stats_.pixels;
        if(opaqueTextures_&&tex.decodedOpaque){
        if(batchCounters_){
        if(tint==65535){if(repeat)packedWorld<true,true,true,true>(job);else packedWorld<false,true,true,true>(job);}
        else{if(repeat)packedWorld<true,false,true,true>(job);else packedWorld<false,false,true,true>(job);}
        }else{
        if(tint==65535){if(repeat)packedWorld<true,true,false,true>(job);else packedWorld<false,true,false,true>(job);}
        else{if(repeat)packedWorld<true,false,false,true>(job);else packedWorld<false,false,false,true>(job);}
        }
        }else{
        if(batchCounters_){
        if(tint==65535){if(repeat)packedWorld<true,true>(job);else packedWorld<false,true>(job);}
        else{if(repeat)packedWorld<true,false>(job);else packedWorld<false,false>(job);}
        }else{
        if(tint==65535){if(repeat)packedWorld<true,true,false>(job);else packedWorld<false,true,false>(job);}
        else{if(repeat)packedWorld<true,false,false>(job);else packedWorld<false,false,false>(job);}
        }
        }
        stats_.packedWorldPixels+=stats_.pixels-before;return;
    }
    for(int y=y0;y<=y1;++y){float rowInverse=affineRow?1/rz:0;int first,last;rasterSpan(row[0],row[1],row[2],dx[0],dx[1],dx[2],x1-x0+1,first,last);
        float e[3]={row[0]+first*dx[0],row[1]+first*dx[1],row[2]+first*dx[2]},z=rz,u=ru,w=rv;
        for(int skip=0;skip<first;++skip){z+=zx;u+=ux;w+=vx;}
        stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
        for(int x=x0+first;x<=x0+last;++x){
            if((e[0]>0||(e[0]==0&&tl[0]))&&(e[1]>0||(e[1]==0&&tl[1]))&&(e[2]>0||(e[2]==0&&tl[2]))){
                size_t index=size_t(y)*stride()+x;
                if(z>depth_[index]){float iz=affineRow?rowInverse:1/z;if(affineRow)++stats_.perspectiveSaved;
                    size_t sample=size_t(texel(w*iz,tex.height,modeV))*tex.width+texel(u*iz,tex.width,modeU);
                    if(tex.alpha[sample]>=alphaRef){depth_[index]=z;uint16_t pixel=tex.pixels[sample];color_[index]=tint==65535?pixel:table?uint16_t(table->red[pixel>>11]|table->green[(pixel>>5)&63]|table->blue[pixel&31]):modulate(pixel,tint);++stats_.pixels;}
                }
            }
            for(int i=0;i<3;++i)e[i]+=dx[i];z+=zx;u+=ux;w+=vx;
        }
        for(int i=0;i<3;++i)row[i]+=dy[i];rz+=zy;ru+=uy;rv+=vy;
    }
}
}
