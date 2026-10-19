#include "raster_span.hpp"
#include "hidden_samples.hpp"
#include "additive_bounds.hpp"
#include "flat_blend.hpp"
#include "flat_span.hpp"
#include "covered_run.hpp"
#include "black_fade.hpp"
#include "bilinear_sample.hpp"
#include "vector_filter.hpp"
#include "filter_address.hpp"
#include "nearest_world.hpp"
#include "constant_alpha.hpp"
#include "soft_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <type_traits>
namespace soft {
namespace {
int sample(float uv,int size,unsigned address){
    if(address==3||address==4){float f=std::max(-1.f,std::min(2.f,uv))*size;int n=int(f);if(float(n)>f)--n;return std::max(0,std::min(size-1,n));}
    if(!(size&(size-1))){float f=uv*size;int n=int(f);if(float(n)>f)--n;
        if(address==2){n&=2*size-1;return n<size?n:2*size-1-n;}return n&(size-1);}
    float period=address==2?2.f:1.f;float wrapped=std::fmod(uv,period);if(wrapped<0)wrapped+=period;
    int n=std::min(int(period*size)-1,int(wrapped*size));return address==2&&n>=size?2*size-1-n:n;
}
// Integer neighbours already represent texel centres. Avoid eight float
// divisions per filtered sample; preserve clamp, wrap and mirrored wrap.
int texel(int n,int size,unsigned address){
    if(address==3||address==4)return std::max(0,std::min(size-1,n));
    int period=address==2?2*size:size;
    if(!(period&(period-1)))n&=period-1;
    else {n%=period;if(n<0)n+=period;}
    return address==2&&n>=size?period-1-n:n;
}
inline void filteredTaps(const Texture& t,const size_t* idx,size_t x0,size_t x1,unsigned fx,unsigned fy,uint16_t& color,unsigned& alpha,bool rejectZero,bool opaque,bool uniform,bool vectorFilter,bool opacityShortcut,unsigned vertexAlpha,unsigned alphaRef,const FilterAddress* axes,Stats& stats){
    if(opaque)alpha=255;
    else if(axes&&x1==x0+1){unsigned a,b,c,d;alphaPair(t.alpha.data()+idx[0],a,b);alphaPair(t.alpha.data()+idx[2],c,d);alpha=bilinearChannel(a,b,c,d,fx,fy);++stats.packedAlphaPixels;}
    else alpha=bilinearChannel(t.alpha[idx[0]],t.alpha[idx[1]],t.alpha[idx[2]],t.alpha[idx[3]],fx,fy);
    // Font/background transparent pixels usually fail alpha test. Determine
    // alpha first so invisible samples need no RGB fetch/decode/interpolation.
    // Alpha-test zero still preserves original RGB (including blend=0).
    if(rejectZero&&alpha==0){color=0;++stats.filteredAlphaOnly;return;}
    if(opacityShortcut&&!opaque&&!uniform&&(vertexAlpha==255?alpha:vertexAlpha*alpha/255)<alphaRef){color=0;++stats.finalAlphaOnly;return;}
    if(uniform){color=t.pixels[0];++stats.filteredFlat;return;}
    unsigned pixels[4]={t.pixels[idx[0]],t.pixels[idx[1]],t.pixels[idx[2]],t.pixels[idx[3]]};
    if(pixels[0]==pixels[1]&&pixels[0]==pixels[2]&&pixels[0]==pixels[3]){color=uint16_t(pixels[0]);++stats.filteredFlat;return;}
    if(vectorFilter){color=vectorBilinear565(pixels,fx,fy);++stats.vectorFilteredPixels;return;}
    unsigned r[4],g[4],b[4];for(unsigned i=0;i<4;++i){unsigned c=pixels[i],rr=c>>11,gg=(c>>5)&63,bb=c&31;r[i]=(rr<<3)|(rr>>2);g[i]=(gg<<2)|(gg>>4);b[i]=(bb<<3)|(bb>>2);}
    color=rgb(bilinearChannel(r[0],r[1],r[2],r[3],fx,fy),bilinearChannel(g[0],g[1],g[2],g[3],fx,fy),bilinearChannel(b[0],b[1],b[2],b[3],fx,fy));
}
void filtered(float u,float v,const Texture& t,unsigned au,unsigned av,uint16_t& color,unsigned& alpha,bool rejectZero,bool opaque,bool uniform,bool vectorFilter,bool fastCoordinates,bool opacityShortcut,unsigned vertexAlpha,unsigned alphaRef,const FilterAddress* axes,Stats& stats,bool constantShortcut){
    // A certified constant opaque texture returns the same color/alpha for
    // every tap and address. Skip coordinates and neighbour fetch entirely.
    if(constantShortcut&&opaque&&uniform){color=t.pixels[0];alpha=255;++stats.filteredFlat;return;}
    float x=u*t.width-.5f,y=v*t.height-.5f;int ix=fastCoordinates?exactFilterFloor(x):int(std::floor(x)),iy=fastCoordinates?exactFilterFloor(y):int(std::floor(y));if(fastCoordinates)++stats.fastFilterCoordinates;unsigned fx=unsigned((x-ix)*256),fy=unsigned((y-iy)*256);
    size_t x0,x1,y0,y1;
    if(axes){int a,b,c,d;axes[0].pair(ix,a,b);axes[1].pair(iy,c,d);x0=a;x1=b;y0=size_t(c)*t.width;y1=size_t(d)*t.width;++stats.preparedFilterPixels;}
    else{x0=texel(ix,t.width,au);x1=texel(ix+1,t.width,au);y0=size_t(texel(iy,t.height,av))*t.width;y1=size_t(texel(iy+1,t.height,av))*t.width;}
    size_t idx[4]={y0+x0,y0+x1,y1+x0,y1+x1};
    filteredTaps(t,idx,x0,x1,fx,fy,color,alpha,rejectZero,opaque,uniform,vectorFilter,opacityShortcut,vertexAlpha,alphaRef,axes,stats);
}
// v is constant across this affine scanline. Hoist its floor/addressing;
// x still comes from the original repeated float UV additions per pixel.
struct FilteredRow {
    const Texture& texture;const FilterAddress* axes;size_t y0=0,y1=0;unsigned fy=0;bool fast;
    FilteredRow(float v,const Texture& t,const FilterAddress* prepared,bool fastCoordinates,bool enabled):texture(t),axes(prepared),fast(fastCoordinates){
        if(!enabled)return;float y=v*t.height-.5f;int iy=fast?exactFilterFloor(y):int(std::floor(y));fy=unsigned((y-iy)*256);
        int a,b;axes[1].pair(iy,a,b);y0=size_t(a)*t.width;y1=size_t(b)*t.width;
    }
    void coordinates(float u,size_t* idx,size_t& x0,size_t& x1,unsigned& fx,Stats& stats)const{
        float x=u*texture.width-.5f;int ix=fast?exactFilterFloor(x):int(std::floor(x));fx=unsigned((x-ix)*256);
        if(fast)++stats.fastFilterCoordinates;int a,b;axes[0].pair(ix,a,b);x0=a;x1=b;++stats.preparedFilterPixels;++stats.filterRowPixels;
        idx[0]=y0+x0;idx[1]=y0+x1;idx[2]=y1+x0;idx[3]=y1+x1;
    }
    unsigned mask(float u,Stats& stats)const{
        size_t idx[4],x0,x1;unsigned fx;coordinates(u,idx,x0,x1,fx,stats);unsigned a,b,c,d;
        if(x1==x0+1){alphaPair(texture.alpha.data()+idx[0],a,b);alphaPair(texture.alpha.data()+idx[2],c,d);++stats.packedAlphaPixels;}
        else{a=texture.alpha[idx[0]];b=texture.alpha[idx[1]];c=texture.alpha[idx[2]];d=texture.alpha[idx[3]];}
        return bilinearChannel(a,b,c,d,fx,fy);
    }
    void sample(float u,uint16_t& color,unsigned& alpha,bool rejectZero,bool opaque,bool uniform,bool vectorFilter,bool opacityShortcut,unsigned vertexAlpha,unsigned alphaRef,Stats& stats,bool constantShortcut)const{
        if(constantShortcut&&opaque&&uniform){color=texture.pixels[0];alpha=255;++stats.filteredFlat;return;}
        size_t idx[4],x0,x1;unsigned fx;coordinates(u,idx,x0,x1,fx,stats);
        filteredTaps(texture,idx,x0,x1,fx,fy,color,alpha,rejectZero,opaque,uniform,vectorFilter,opacityShortcut,vertexAlpha,alphaRef,axes,stats);
    }
};
unsigned channel(float v){return unsigned(std::max(0.f,std::min(255.f,v+.5f)));}
// Black source-over shadows consume only the alpha plane. Retain the exact
// four taps and integer interpolation without preparing unused RGB or blend
// options for the general filtered sampler.
inline unsigned filteredMask(float u,float v,const Texture& t,unsigned au,unsigned av,bool fastCoordinates,const FilterAddress* axes,Stats& stats){
    float x=u*t.width-.5f,y=v*t.height-.5f;int ix=fastCoordinates?exactFilterFloor(x):int(std::floor(x)),iy=fastCoordinates?exactFilterFloor(y):int(std::floor(y));
    if(fastCoordinates)++stats.fastFilterCoordinates;unsigned fx=unsigned((x-ix)*256),fy=unsigned((y-iy)*256);
    size_t x0,x1,y0,y1;
    if(axes){int a,b,c,d;axes[0].pair(ix,a,b);axes[1].pair(iy,c,d);x0=a;x1=b;y0=size_t(c)*t.width;y1=size_t(d)*t.width;++stats.preparedFilterPixels;}
    else{x0=texel(ix,t.width,au);x1=texel(ix+1,t.width,au);y0=size_t(texel(iy,t.height,av))*t.width;y1=size_t(texel(iy+1,t.height,av))*t.width;}
    unsigned a,b,c,d;
    if(axes&&x1==x0+1){alphaPair(t.alpha.data()+y0+x0,a,b);alphaPair(t.alpha.data()+y1+x0,c,d);++stats.packedAlphaPixels;}
    else{a=t.alpha[y0+x0];b=t.alpha[y0+x1];c=t.alpha[y1+x0];d=t.alpha[y1+x1];}
    return bilinearChannel(a,b,c,d,fx,fy);
}
}
// The game supplies the radar's transformed viewport. Clip only its map pass,
// independently of later HUD depth writes and at the active HUD resolution.
void Renderer::beginRadar(float l,float t,float r,float b){flushDeferred();
 radarClip_=std::isfinite(l)&&std::isfinite(t)&&std::isfinite(r)&&std::isfinite(b)&&std::fabs(l)<1e6f&&std::fabs(t)<1e6f&&std::fabs(r)<1e6f&&std::fabs(b)<1e6f&&r-l>1e-5f&&b-t>1e-5f;
 radarCx_=(l+r)*.5f;radarCy_=(t+b)*.5f;radarRx_=(r-l)*.5f;radarRy_=(b-t)*.5f;
}
void Renderer::radarSpan(int y,int x0,int& first,int& last)const{
 double cy=radarCy_/scale_,ry=radarRy_/scale_,cx=radarCx_/scale_,rx=radarRx_/scale_;
 double v=(y+.5-cy)/ry;if(std::fabs(v)>=1){first=1;last=0;return;}
 double extent=rx*std::sqrt(std::max(0.,1-v*v));
 first=std::max(first,int(std::ceil(cx-extent-.5))-x0);
 last=std::min(last,int(std::floor(cx+extent-.5))-x0);
 if(first>last){first=1;last=0;}
}
// Private position-only query for an immediately submitted, fully clipped-in
// textured atomic. 0: unavailable, 1: usable setup, 2: no possible sample.
// depthChecked distinguishes a tested small face from prepared Z alone.
// Callers may reuse result1 only until the next depth/target mutation.
int Renderer::queryWorldVisibility(const Vec3* input,WorldTriangleSetup& setup,bool wide){
    if(!canQueryWorldVisibility())return 0;
    Vec3 a=input[0],b=input[1],c=input[2];
    // Projected position/UV validation is cached per vertex by SoftwareDevice.
    for(auto v:{&a,&b,&c}){v->x/=scale_;v->y/=scale_;}
    auto edge=[](Vec3 p,Vec3 q,float x,float y){return (q.x-p.x)*(y-p.y)-(q.y-p.y)*(x-p.x);};
    float area=edge(a,b,c.x,c.y);if(!std::isfinite(area))return 0;
    if(std::fabs(area)<1e-5f){++stats_.hiddenSampleChecks;++stats_.hiddenSampleTriangles;return 2;}
    bool swapped=area<0;if(swapped){std::swap(b,c);area=-area;}
    int x0=std::max(0,rasterCeil(std::min({a.x,b.x,c.x})-.5f)),x1=std::min(int(renderWidth())-1,rasterFloor(std::max({a.x,b.x,c.x})-.5f));
    int y0=std::max(0,rasterCeil(std::min({a.y,b.y,c.y})-.5f)),y1=std::min(int(renderHeight())-1,rasterFloor(std::max({a.y,b.y,c.y})-.5f));
    if(x0>x1||y0>y1){++stats_.hiddenSampleChecks;++stats_.hiddenSampleTriangles;return 2;}
    float inv=1/area;
    float row[3]={edge(b,c,x0+.5f,y0+.5f),edge(c,a,x0+.5f,y0+.5f),edge(a,b,x0+.5f,y0+.5f)};
    float dx[3]={b.y-c.y,c.y-a.y,a.y-b.y},dy[3]={c.x-b.x,a.x-c.x,b.x-a.x};
    auto topLeft=[](Vec3 p,Vec3 q){return q.y<p.y||(q.y==p.y&&q.x>p.x);};bool tl[3]={topLeft(b,c),topLeft(c,a),topLeft(a,b)};
    float z=(row[0]*a.z+row[1]*b.z+row[2]*c.z)*inv;
    float zx=(dx[0]*a.z+dx[1]*b.z+dx[2]*c.z)*inv,zy=(dy[0]*a.z+dy[1]*b.z+dy[2]*c.z)*inv;
    if(lazyDepth_&&lazyThisFrame_&&occlusionEnabled_&&(x1-x0+1)*(y1-y0+1)>16&&sampleLazyDepth()&&lazyOccluded(x0,y0,x1,y1,z,zx,zy))return 2;
    bool checked=(x1-x0+1)*(y1-y0+1)<256;
    if(checked){++stats_.hiddenSampleChecks;
        bool hidden=x1-x0+1<16&&(x1-x0+1)*(y1-y0+1)<=16?hiddenSamples(depth_.data()+size_t(y0)*stride()+x0,stride(),x1-x0+1,y1-y0+1,row,dx,dy,tl,z,zx,zy):occludedSmall(x0,y0,x1,y1,z,zx,zy);
        if(hidden){++stats_.hiddenSampleTriangles;return 2;}
    }
    if(!checked&&wide){checked=true;if(occluded(x0,y0,x1,y1,z,zx,zy))return 2;}
    // Only surviving faces need a setup packet written to the stack.
    setup.points[0]=a;setup.points[1]=b;setup.points[2]=c;setup.x0=x0;setup.x1=x1;setup.y0=y0;setup.y1=y1;setup.area=area;setup.inv=inv;setup.swapped=swapped;
    for(unsigned k=0;k<3;++k){setup.row[k]=row[k];setup.dx[k]=dx[k];setup.dy[k]=dy[k];setup.tl[k]=tl[k];}setup.z=z;setup.zx=zx;setup.zy=zy;setup.depthChecked=checked;return 1;
}
void Renderer::screenTriangle(const ScreenVertex* input,const Texture* texture,const ScreenState& s){screenTriangleChecked(input,texture,s,false);}
void Renderer::screenTriangleChecked(const ScreenVertex* input,const Texture* texture,const ScreenState& s,bool verified,const WorldTriangleSetup* setup){
    if(deferredActive_&&!deferredReplaying_&&deferCommand(input,texture,s,verified,setup,0))return;
    if(skyActive_&&(s.zTest||s.zWrite))endSky();
    if(rainActive_&&!(texture&&texture->decodedOpaque&&!s.linear&&!s.screenDepth&&s.zTest&&!s.zWrite&&s.blend==3)){endRain();++stats_.rainFallbacks;}
    if(hudOverlay_&&s.blend>=2&&s.blend<=4){flushHudOverlay();++stats_.hudFallbacks;}
    if(s.zWrite&&(s.screenDepth||!s.zTest))invalidateOcclusion();
    expandedValid_=false;++stats_.submitted;
    if(s.blend>5||s.alphaRef>255||s.cull<1||s.cull>3){++stats_.rejected;return;}
    if(texture&&(!texture->width||!texture->height||texture->width>1024||texture->height>1024||texture->pixels.size()!=size_t(texture->width)*texture->height||texture->alpha.size()!=texture->pixels.size())){++stats_.rejected;return;}
    ScreenVertex a=input[0],b=input[1],c=input[2];
    if(verified)stats_.verifiedScreenVertices+=3;
    else for(auto p:{a,b,c})if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.reciprocalZ)||!std::isfinite(p.u)||!std::isfinite(p.v)||std::fabs(p.x)>1e6f||std::fabs(p.y)>1e6f||p.reciprocalZ<0||p.reciprocalZ>1e6f||std::fabs(p.u)>1e5f||std::fabs(p.v)>1e5f||!std::isfinite(p.r)||!std::isfinite(p.g)||!std::isfinite(p.b)||!std::isfinite(p.a)||p.r<0||p.r>255||p.g<0||p.g>255||p.b<0||p.b>255||p.a<0||p.a>255){++stats_.rejected;return;}
    auto edge=[](ScreenVertex p,ScreenVertex q,float x,float y){return (q.x-p.x)*(y-p.y)-(q.y-p.y)*(x-p.x);};
    float area;int x0,x1,y0,y1;bool tl0,tl1,tl2;float inv,row0,row1,row2;
    if(setup){
        if((s.cull==2&&setup->swapped)||(s.cull==3&&!setup->swapped))return;
        if(setup->swapped)std::swap(b,c);ScreenVertex* v[3]={&a,&b,&c};
        for(unsigned i=0;i<3;++i){v[i]->x=setup->points[i].x;v[i]->y=setup->points[i].y;}
        area=setup->area;x0=setup->x0;x1=setup->x1;y0=setup->y0;y1=setup->y1;inv=setup->inv;
        tl0=setup->tl[0];tl1=setup->tl[1];tl2=setup->tl[2];row0=setup->row[0];row1=setup->row[1];row2=setup->row[2];
    }else{
        for(auto* v:{&a,&b,&c}){v->x/=scale_;v->y/=scale_;}
        area=edge(a,b,c.x,c.y);if(std::fabs(area)<1e-5f)return;
        if((s.cull==2&&area<0)||(s.cull==3&&area>0))return;
        if(area<0){std::swap(b,c);area=-area;}
        x0=std::max(0,rasterCeil(std::min({a.x,b.x,c.x})-.5f));x1=std::min(int(renderWidth())-1,rasterFloor(std::max({a.x,b.x,c.x})-.5f));
        y0=std::max(0,rasterCeil(std::min({a.y,b.y,c.y})-.5f));y1=std::min(int(renderHeight())-1,rasterFloor(std::max({a.y,b.y,c.y})-.5f));
        if(x0>x1||y0>y1)return;
        auto topLeft=[](ScreenVertex p,ScreenVertex q){return q.y<p.y||(q.y==p.y&&q.x>p.x);};
        tl0=topLeft(b,c);tl1=topLeft(c,a);tl2=topLeft(a,b);inv=1/area;
        row0=edge(b,c,x0+.5f,y0+.5f);row1=edge(c,a,x0+.5f,y0+.5f);row2=edge(a,b,x0+.5f,y0+.5f);
    }++stats_.rasterized;
    // Evaluate depth with precisely the original expressions before preparing
    // UV/colour attributes and material tables. No vertex-extrema shortcut:
    // the occlusion certificate bounds the actual float recurrence over bbox.
    bool testSmall=y0>=replayLo_&&y1<replayHi_&&!(setup&&setup->depthChecked)&&smallDepth_&&s.zTest&&!s.screenDepth&&(x1-x0+1)*(y1-y0+1)<256;
    float earlyRow=0,earlyDx=0,earlyDy=0;bool preparedDepth=(setup||earlyDepth_||testSmall)&&s.zTest&&!s.screenDepth;
    if(preparedDepth){if(setup){earlyRow=setup->z;earlyDx=setup->zx;earlyDy=setup->zy;}else{earlyRow=(row0*a.reciprocalZ+row1*b.reciprocalZ+row2*c.reciprocalZ)*inv;
        earlyDx=((b.y-c.y)*a.reciprocalZ+(c.y-a.y)*b.reciprocalZ+(a.y-b.y)*c.reciprocalZ)*inv;
        earlyDy=((c.x-b.x)*a.reciprocalZ+(a.x-c.x)*b.reciprocalZ+(b.x-a.x)*c.reciprocalZ)*inv;}
        if(testSmall){float edges[3]={row0,row1,row2},ex[3]={b.y-c.y,c.y-a.y,a.y-b.y},ey[3]={c.x-b.x,a.x-c.x,b.x-a.x};bool tl[3]={tl0,tl1,tl2};++stats_.hiddenSampleChecks;
            if(x1-x0+1<16&&(x1-x0+1)*(y1-y0+1)<=16?hiddenSamples(depth_.data()+size_t(y0)*stride()+x0,stride(),x1-x0+1,y1-y0+1,edges,ex,ey,tl,earlyRow,earlyDx,earlyDy):occludedSmall(x0,y0,x1,y1,earlyRow,earlyDx,earlyDy)){++stats_.hiddenSampleTriangles;return;}}
        if(earlyDepth_&&!(setup&&setup->depthChecked)&&occluded(x0,y0,x1,y1,earlyRow,earlyDx,earlyDy)){++stats_.earlyDepthTriangles;return;}}
    const unsigned flatR=channel(a.r),flatG=channel(a.g),flatB=channel(a.b),flatA=channel(a.a);
    // Integer vertex colours which agree remain constant after perspective
    // interpolation. Keep the general path for fractional/zero-depth inputs.
    const bool constantColor=(!s.perspectiveColor||(a.reciprocalZ>1e-8f&&b.reciprocalZ>1e-8f&&c.reciprocalZ>1e-8f&&a.r==float(flatR)&&a.g==float(flatG)&&a.b==float(flatB)&&a.a==float(flatA)))&&a.r==b.r&&a.r==c.r&&a.g==b.g&&a.g==c.g&&a.b==b.b&&a.b==c.b&&a.a==b.a&&a.a==c.a;
    // Certified constant colours use flatA in every raster path. Multiplying
    // by a uint8 texture alpha cannot raise it above flatA. Zero vertex alpha
    // also stays exactly zero through all finite interpolants, even with RGB
    // gradients. Reject before UV/filter preparation only when alpha test fails;
    // alphaRef=0, varying/fractional perspective alpha retain the old loops.
    if(alphaReject_&&s.alphaRef&&((constantColor&&flatA<s.alphaRef)||(a.a==0&&b.a==0&&c.a==0))){++stats_.alphaRejectedTriangles;return;}
    if(hudOverlay_&&s.blend!=5)markHudBounds(x0,y0,x1,y1);
    const bool constantDepth=s.screenDepth&&a.depthZ==b.depthZ&&a.depthZ==c.depthZ;
    const float flatDepth=1-a.depthZ;
    const bool flatFill=constantColor&&!texture&&!hudOverlay_&&area>=256&&(x1-x0+1)*(y1-y0+1)>=256;
    // Tables are constructed only when useful.
    FlatBlend flatTable;if(flatFill)flatTable=FlatBlend(flatR,flatG,flatB,flatA,s.blend);
    // Full-screen tints/fades need neither UV nor colour interpolation. Keep
    // the same coverage/depth rules, with a dedicated integer pixel loop.
    if(flatFill&&((!s.zTest&&!s.zWrite)||constantDepth)){
        for(;y0<replayLo_;++y0){row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;}y1=std::min(y1,replayHi_-1);if(y0>y1)return;
        for(int y=y0;y<=y1;++y){int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_&&s.screenDepth)radarSpan(y,x0,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y);
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            const float edgeValues[3]={e0,e1,e2},steps[3]={b.y-c.y,c.y-a.y,a.y-b.y};uint8_t tail=0;
            if(flatSpans_&&(s.blend==0||s.blend==1)&&!s.zTest&&!s.zWrite&&!radarClip_&&flatA>=s.alphaRef&&coveredEdgeRun(edgeValues,steps,first,last,&tail)){
                for(int x=x0+first;x<x0+first+3;++x){if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){color_[size_t(y)*stride()+x]=flatTable(color_[size_t(y)*stride()+x]);++stats_.pixels;}e0+=steps[0];e1+=steps[1];e2+=steps[2];}
                unsigned count=unsigned(last-first-5);flatSourceOver(color_.data()+size_t(y)*stride()+x0+first+3,count,flatR,flatG,flatB,s.blend==0?255:flatA,flatTable);stats_.pixels+=count;stats_.flatSpanPixels+=count;
                for(unsigned k=0;k<3;++k)if(tail&(1u<<k)){size_t index=size_t(y)*stride()+x0+last-2+k;color_[index]=flatTable(color_[index]);++stats_.pixels;}
                row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;continue;
            }
            for(int x=x0+first;x<=x0+last;++x){
                if(flatA>=s.alphaRef&&(e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){size_t index=size_t(y)*stride()+x;
                    if(!s.zTest||flatDepth>=depth_[index]){color_[index]=flatTable(color_[index]);if(s.zWrite)depth_[index]=flatDepth;++stats_.pixels;}
                }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;
            }row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;
        }return;
    }
    const bool white=constantColor&&flatR==255&&flatG==255&&flatB==255;
    ColorTable* tintTable=nullptr;
    if(texture&&constantColor&&!white&&!hudOverlay_&&area>=256&&(x1-x0+1)*(y1-y0+1)>=256){
        uint32_t key=(flatR<<16)|(flatG<<8)|flatB;
        tintTable=&colorTables_[(key^(key>>6)^(key>>12))&15];
        if(!tintTable->valid||tintTable->key!=key){
            for(unsigned i=0;i<32;++i){unsigned expanded=(i<<3)|(i>>2);
                tintTable->red[i]=uint16_t(((expanded*flatR/255)>>3)<<11);
                tintTable->blue[i]=uint16_t((expanded*flatB/255)>>3);}
            for(unsigned i=0;i<64;++i){unsigned expanded=(i<<2)|(i>>4);
                tintTable->green[i]=uint16_t(((expanded*flatG/255)>>2)<<5);}
            tintTable->valid=true;tintTable->key=key;
        }
    }
    // Sprites share one reciprocal depth: affine UV is exactly the same
    // projection, without a division at every covered pixel.
    const bool perspectiveUV=texture&&(a.reciprocalZ!=b.reciprocalZ||a.reciprocalZ!=c.reciprocalZ)&&(a.reciprocalZ>0||b.reciprocalZ>0||c.reciprocalZ>0);
    // Prepare only interpolants consumed by this material. Constant colours
    // use integer channels; screen depth is separate and absent for 3D passes.
    const unsigned attributes=leanSetup_?(constantColor?3:7):8;
    float values[3][8],row[8]={0},dx[8]={0},dy[8]={0};ScreenVertex vertices[3]={a,b,c};
    for(int i=0;i<3;++i){auto v=vertices[i];float colorScale=s.perspectiveColor?v.reciprocalZ:1,uvScale=perspectiveUV?v.reciprocalZ:1;
        values[i][0]=v.reciprocalZ;values[i][1]=v.u*uvScale;values[i][2]=v.v*uvScale;
        if(attributes>3){values[i][3]=v.r*colorScale;values[i][4]=v.g*colorScale;values[i][5]=v.b*colorScale;values[i][6]=v.a*colorScale;}
        values[i][7]=s.screenDepth?1-v.depthZ:0;
    }
    if(preparedDepth){row[0]=earlyRow;dx[0]=earlyDx;dy[0]=earlyDy;}
    for(unsigned i=preparedDepth?1:0;i<attributes;++i){
        row[i]=(row0*values[0][i]+row1*values[1][i]+row2*values[2][i])*inv;
        dx[i]=((b.y-c.y)*values[0][i]+(c.y-a.y)*values[1][i]+(a.y-b.y)*values[2][i])*inv;
        dy[i]=((c.x-b.x)*values[0][i]+(a.x-c.x)*values[1][i]+(b.x-a.x)*values[2][i])*inv;
    }
    if(attributes<8&&s.screenDepth&&!constantDepth){const unsigned i=7;
        row[i]=(row0*values[0][i]+row1*values[1][i]+row2*values[2][i])*inv;
        dx[i]=((b.y-c.y)*values[0][i]+(c.y-a.y)*values[1][i]+(a.y-b.y)*values[2][i])*inv;
        dy[i]=((c.x-b.x)*values[0][i]+(a.x-c.x)*values[1][i]+(b.x-a.x)*values[2][i])*inv;
    }
    if(leanSetup_)stats_.setupSavedAttributes+=8-attributes-unsigned(s.screenDepth&&!constantDepth);
    // Preparing Z does not replace the existing large-face occlusion query.
    bool occlusionTested=earlyDepth_||testSmall||(setup&&setup->depthChecked);
    if(!occlusionTested&&s.zTest&&!s.screenDepth&&occluded(x0,y0,x1,y1,row[0],dx[0],dy[0]))return;
    for(;y0<replayLo_;++y0){row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(unsigned i=0;i<8;++i)row[i]+=dy[i];}
    y1=std::min(y1,replayHi_-1);if(y0>y1)return;
    const bool rowReciprocal=dx[0]==0&&(perspectiveUV||(s.perspectiveColor&&!constantColor));
    bool repeat=texture&&s.addressU==1&&s.addressV==1&&!(texture->width&(texture->width-1))&&!(texture->height&(texture->height-1));
    bool clamp=texture&&(s.addressU==3||s.addressU==4)&&(s.addressV==3||s.addressV==4);
    if(preparedWorld_&&texture&&(repeat||clamp)&&s.perspectiveColor&&!s.linear&&!hudOverlay_&&!s.screenDepth&&s.zTest&&(s.zWrite||(transparentWorld_&&!(vectorFilter_&&(filterStages_&4))))&&s.blend<=1){
        FlatAlphaTable* alphaTable=nullptr;
        // Opaque texel alpha + one integer vertex colour/opacity for the whole
        // triangle. Fuse modulation and source-over without intermediate RGB565
        // quantisation. The bounded cache owns components, never raster pointers.
        if(flatAlpha_&&constantColor&&texture->decodedOpaque&&s.blend==1&&flatA>0&&flatA<255&&area>=2048&&(x1-x0+1)*(y1-y0+1)>=2048){
            uint32_t key=(flatA<<24)|(flatR<<16)|(flatG<<8)|flatB;
            if(flatAlphaTables_.empty())flatAlphaTables_.resize(4);
            alphaTable=&flatAlphaTables_[(key^(key>>8)^(key>>16)^(key>>24))&3];
            if(!alphaTable->valid||alphaTable->key!=key){
                auto build=[&](uint8_t* table,unsigned size,unsigned color,unsigned shift){
                    for(unsigned src=0;src<size;++src){unsigned expanded=(src<<shift)|(src>>(8-2*shift));unsigned lit=expanded*color/255;
                        for(unsigned dst=0;dst<size;++dst){unsigned background=(dst<<shift)|(dst>>(8-2*shift));table[src*size+dst]=uint8_t(((lit*flatA+background*(255-flatA)+127)/255)>>shift);}}
                };
                build(alphaTable->red,32,flatR,3);build(alphaTable->green,64,flatG,2);build(alphaTable->blue,32,flatB,3);
                alphaTable->key=key;alphaTable->valid=true;++stats_.flatAlphaBuilds;
            }
        }
        NearestWorldJob job={color_.data(),depth_.data(),stride(),stats_,*texture,x0,x1,y0,y1,{row0,row1,row2},{b.y-c.y,c.y-a.y,a.y-b.y},{c.x-b.x,a.x-c.x,b.x-a.x},{tl0,tl1,tl2},row,dx,dy,flatR,flatG,flatB,flatA,s.alphaRef,s.blend,white,perspectiveUV,rowReciprocal,exactAdvance_,depthRows_&&gradientDepthRows_,s.zWrite,nearestCoverage_,nearestOpacity_&&screenCertificates_&&texture->decodedOpaque,modulationTables_?&ModulationTables::get():nullptr,alphaTable?alphaTable->red:nullptr,alphaTable?alphaTable->green:nullptr,alphaTable?alphaTable->blue:nullptr,tintTable?tintTable->red:nullptr,tintTable?tintTable->green:nullptr,tintTable?tintTable->blue:nullptr,false,whiteBlend_,worldChannels_,rgbRegister_};
        if(constantAlpha_&&!constantColor&&a.a==b.a&&a.a==c.a&&a.a==float(flatA)&&(x1-x0+1)*(y1-y0+1)>=256&&certifyConstantAlpha(row,dx,dy,unsigned(x1-x0+1),unsigned(y1-y0+1),flatA)){job.fixedAlpha=true;job.alpha=flatA;stats_.constantAlphaPixels+=uint64_t(x1-x0+1)*(y1-y0+1);}
        if(alphaTable){auto before=stats_.pixels;if(repeat)nearestWorldDraw<true,true,false,true,false,true>(job);else nearestWorldDraw<true,false,false,true,false,true>(job);stats_.preparedWorldPixels+=stats_.pixels-before;if(!s.zWrite)stats_.transparentWorldPixels+=stats_.pixels-before;return;}
        if(effectSpans_&&particleSpans_&&zeroMasks_&&s.blend==1&&((constantColor&&flatR==0&&flatG==0&&flatB==0)||(texture->decodedUniform&&texture->pixels[0]==0))){
            auto before=stats_.pixels;
            if(constantColor){if(repeat)nearestWorldDraw<true,true,false,true,true>(job);else nearestWorldDraw<true,false,false,true,true>(job);}
            else{if(repeat)nearestWorldDraw<false,true,false,true,true>(job);else nearestWorldDraw<false,false,false,true,true>(job);}
            stats_.preparedWorldPixels+=stats_.pixels-before;if(!s.zWrite)stats_.transparentWorldPixels+=stats_.pixels-before;return;
        }
        auto before=stats_.pixels;
        if(batchCounters_&&batchGeneral_){
        if(modulationTables_){
            if(constantColor){if(repeat)nearestWorldDraw<true,true,true>(job);else nearestWorldDraw<true,false,true>(job);}
            else{if(repeat)nearestWorldDraw<false,true,true>(job);else nearestWorldDraw<false,false,true>(job);}
            ++stats_.modulationTriangles;
        }else{
            if(constantColor){if(repeat)nearestWorldDraw<true,true>(job);else nearestWorldDraw<true,false>(job);}
            else{if(repeat)nearestWorldDraw<false,true>(job);else nearestWorldDraw<false,false>(job);}
        }
        }else{
        if(modulationTables_){
            if(constantColor){if(repeat)nearestWorldDraw<true,true,true,false>(job);else nearestWorldDraw<true,false,true,false>(job);}
            else{if(repeat)nearestWorldDraw<false,true,true,false>(job);else nearestWorldDraw<false,false,true,false>(job);}
            ++stats_.modulationTriangles;
        }else{
            if(constantColor){if(repeat)nearestWorldDraw<true,true,false,false>(job);else nearestWorldDraw<true,false,false,false>(job);}
            else{if(repeat)nearestWorldDraw<false,true,false,false>(job);else nearestWorldDraw<false,false,false,false>(job);}
        }
        }
        stats_.preparedWorldPixels+=stats_.pixels-before;if(!s.zWrite)stats_.transparentWorldPixels+=stats_.pixels-before;return;
    }
    // Common world material: nearest texture, reciprocal depth, opaque/source-alpha blend.
    // Keep only the required interpolants and avoid the UI/general-state
    // branches inside every pixel. Partial alpha still uses the exact blend.
    if(texture&&!s.linear&&!hudOverlay_&&!s.screenDepth&&s.zTest&&s.zWrite&&s.blend<=1){
        for(int y=y0;y<=y1;++y){int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_&&s.screenDepth)radarSpan(y,x0,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),z=row[0],u=row[1],v=row[2],col[4]={row[3],row[4],row[5],row[6]};
            for(int skip=0;skip<first;++skip){z+=dx[0];u+=dx[1];v+=dx[2];if(!constantColor)for(unsigned i=0;i<4;++i)col[i]+=dx[i+3];}
            float rowInverse=rowReciprocal&&row[0]>1e-8f?1/row[0]:1;
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            for(int x=x0+first;x<=x0+last;++x){
                if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){size_t index=size_t(y)*stride()+x;
                    if(z>depth_[index]){float iz=rowReciprocal?rowInverse:(perspectiveUV||(s.perspectiveColor&&!constantColor))&&z>1e-8f?1/z:1;
                        if(rowReciprocal)++stats_.perspectiveSaved;
                        float uvScale=perspectiveUV?iz:1;
                        size_t t=size_t(sample(v*uvScale,texture->height,s.addressV))*texture->width+sample(u*uvScale,texture->width,s.addressU);
                        float colorScale=s.perspectiveColor?iz:1;
                        unsigned alpha=(constantColor?flatA:channel(col[3]*colorScale))*texture->alpha[t]/255;
                        if(alpha>=s.alphaRef){uint16_t tex=texture->pixels[t],pixel;
                            if(white&&(s.blend==0||alpha==255))pixel=tex;
                            else if(tintTable&&(s.blend==0||alpha==255)){pixel=uint16_t(tintTable->red[tex>>11]|tintTable->green[(tex>>5)&63]|tintTable->blue[tex&31]);++stats_.tintPixels;}
                            else{unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;
                                unsigned r=(constantColor?flatR:channel(col[0]*colorScale))*((tr<<3)|(tr>>2))/255,g=(constantColor?flatG:channel(col[1]*colorScale))*((tg<<2)|(tg>>4))/255,blue=(constantColor?flatB:channel(col[2]*colorScale))*((tb<<3)|(tb>>2))/255;
                                if(s.blend==1&&alpha!=255){uint16_t old=color_[index];unsigned dr=old>>11,dg=(old>>5)&63,db=old&31;
                                    r=(r*alpha+((dr<<3)|(dr>>2))*(255-alpha)+127)/255;
                                    g=(g*alpha+((dg<<2)|(dg>>4))*(255-alpha)+127)/255;
                                    blue=(blue*alpha+((db<<3)|(db>>2))*(255-alpha)+127)/255;}
                                pixel=rgb(r,g,blue);}
                            color_[index]=pixel;depth_[index]=z;++stats_.pixels;
                        }
                    }
                }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;z+=dx[0];u+=dx[1];v+=dx[2];if(!constantColor)for(unsigned i=0;i<4;++i)col[i]+=dx[i+3];
            }row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(int i=0;i<(constantColor?3:7);++i)row[i]+=dy[i];
        }return;
    }
    const bool opaqueTexture=screenCertificates_&&texture&&texture->decodedOpaque;
    const bool uniformTexture=screenCertificates_&&texture&&texture->decodedUniform;
    if(opaqueTexture||uniformTexture)++stats_.certifiedScreenTriangles;
    const bool constantTextureColor=constantColor&&uniformTexture;
    unsigned uniformR=0,uniformG=0,uniformB=0;
    if(constantTextureColor){unsigned tex=texture->pixels[0],tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;uniformR=(tr<<3)|(tr>>2);uniformG=(tg<<2)|(tg>>4);uniformB=(tb<<3)|(tb>>2);if(!white){uniformR=uniformR*flatR/255;uniformG=uniformG*flatG/255;uniformB=uniformB*flatB/255;}}
    const uint16_t uniformPixel=rgb(uniformR,uniformG,uniformB);
    const FilterAddress filterAxes[2]={FilterAddress(texture?texture->width:1,s.addressU),FilterAddress(texture?texture->height:1,s.addressV)};
    const FilterAddress* preparedFilter=vectorFilter_&&(filterStages_&16)?filterAxes:nullptr;
    const bool blackSource=effectSpans_&&particleSpans_&&texture&&((constantColor&&flatR==0&&flatG==0&&flatB==0)||(zeroMasks_&&uniformTexture&&texture->pixels[0]==0))&&s.blend==1&&!hudOverlay_;
    const BlackFade* blackFade=blackSource?&BlackFade::get():nullptr;
    const bool whiteTextureFast=vectorFilter_&&(filterStages_&2)&&uniformTexture&&texture->pixels[0]==65535;
    AddContribution* contribution=nullptr;
    if(additiveCache_&&texture&&texture->decodedOpaque&&constantColor&&flatA>=s.alphaRef&&(s.blend==2||s.blend==3)){
        unsigned amount=s.blend==2?flatA:255;uint64_t key=(uint64_t(s.blend)<<32)|(uint64_t(amount)<<24)|(flatR<<16)|(flatG<<8)|flatB;
        contribution=&addContributions_[(key^(key>>8)^(key>>16)^(key>>24))&15];
        if(!contribution->valid||contribution->key!=key){++stats_.additiveCacheBuilds;
            for(unsigned n=0;n<32;++n){unsigned value=(n<<3)|(n>>2);contribution->red[n]=(value*flatR/255*amount+127)/255;contribution->blue[n]=(value*flatB/255*amount+127)/255;}
            for(unsigned n=0;n<64;++n)contribution->green[n]=(((n<<2)|(n>>4))*flatG/255*amount+127)/255;
            contribution->key=key;contribution->valid=true;
        }
    }

    // Constant-tint alpha surfaces: sprites and filtered world shadows.
    // Preserve world perspective UV, screen-depth equality and reciprocal-depth
    // strict comparison. Keep the original binary32
    // attribute/edge recurrences and sampler; specialize the blend loop only.
    // Overlay compositing and depth-writing radar masks retain the old path.
    if(alphaSpans_&&texture&&constantColor&&(!s.screenDepth||constantDepth)&&!s.zWrite&&!hudOverlay_&&s.blend==1){
        uint64_t written=0,tinted=0;
        for(int y=y0;y<=y1;++y){int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_&&s.screenDepth)radarSpan(y,x0,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),u=row[1],v=row[2],z=row[0];float rowInverse=rowReciprocal&&row[0]>1e-8f?1/row[0]:1;
            for(int skip=0;skip<first;++skip){u+=dx[1];v+=dx[2];z+=dx[0];}
            const bool fixedRow=s.linear&&!perspectiveUV&&dx[2]==0&&preparedFilter&&last-first+1>=8;
            const FilteredRow rowFilter(fixedRow?v:0,*texture,filterAxes,vectorFilter_&&(filterStages_&1),fixedRow);
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            for(int x=x0+first;x<=x0+last;++x){
                if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){size_t index=size_t(y)*stride()+x;
                    if(!s.zTest||(s.screenDepth?flatDepth>=depth_[index]:z>depth_[index])){
                        float iz=rowReciprocal?rowInverse:perspectiveUV&&z>1e-8f?1/z:1;if(rowReciprocal)++stats_.perspectiveSaved;
                        float us=perspectiveUV?iz:1;
                        if(vectorFilter_&&(filterStages_&8)&&flatA<s.alphaRef)++stats_.vertexAlphaRejected;
                        else{unsigned ta;uint16_t tex;
                            if(blackFade&&s.linear){tex=0;ta=opaqueTexture?255:fixedRow?rowFilter.mask(u,stats_):filteredMask(u*us,v*us,*texture,s.addressU,s.addressV,vectorFilter_&&(filterStages_&1),preparedFilter,stats_);}
                            else if(s.linear&&fixedRow)rowFilter.sample(u,tex,ta,s.alphaRef>0,opaqueTexture,uniformTexture,vectorFilter_&&(filterStages_&4),vectorFilter_&&(filterStages_&8),flatA,s.alphaRef,stats_,worldChannels_);
                            else if(s.linear)filtered(u*us,v*us,*texture,s.addressU,s.addressV,tex,ta,s.alphaRef>0,opaqueTexture,uniformTexture,vectorFilter_&&(filterStages_&4),vectorFilter_&&(filterStages_&1),vectorFilter_&&(filterStages_&8),flatA,s.alphaRef,preparedFilter,stats_,worldChannels_);
                            else{size_t t;if(preparedFilter){t=size_t(filterAxes[1].sample(v*us))*texture->width+filterAxes[0].sample(u*us);++stats_.preparedNearestPixels;}else t=size_t(sample(v*us,texture->height,s.addressV))*texture->width+sample(u*us,texture->width,s.addressU);tex=blackSource?0:uniformTexture?texture->pixels[0]:texture->pixels[t];ta=opaqueTexture?255:texture->alpha[t];}
                            unsigned alpha=opaqueTexture?flatA:flatA*ta/255;
                            if(alpha>=s.alphaRef){
                                if(blackFade){color_[index]=(*blackFade)(color_[index],alpha);++stats_.blackFadePixels;}
                                else if(white&&alpha==255)color_[index]=tex;
                                else if(tintTable&&alpha==255){color_[index]=constantTextureColor?uniformPixel:uint16_t(tintTable->red[tex>>11]|tintTable->green[(tex>>5)&63]|tintTable->blue[tex&31]);++tinted;}
                                else{unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;
                                    unsigned r=(tr<<3)|(tr>>2),g=(tg<<2)|(tg>>4),blue=(tb<<3)|(tb>>2);
                                    if(!white){r=r*flatR/255;g=g*flatG/255;blue=blue*flatB/255;}
                                    if(alpha!=255){unsigned old=color_[index],dr=old>>11,dg=(old>>5)&63,db=old&31;
                                        dr=(dr<<3)|(dr>>2);dg=(dg<<2)|(dg>>4);db=(db<<3)|(db>>2);
                                        if(vectorFilter_&&(filterStages_&4)){color_[index]=vectorSourceOver565(r,g,blue,alpha,dr,dg,db);++stats_.vectorBlendPixels;}
                                        else color_[index]=rgb((r*alpha+dr*(255-alpha)+127)/255,(g*alpha+dg*(255-alpha)+127)/255,(blue*alpha+db*(255-alpha)+127)/255);
                                    }else color_[index]=rgb(r,g,blue);
                                }++written;
                            }
                        }
                    }
                }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;u+=dx[1];v+=dx[2];z+=dx[0];
            }row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;row[0]+=dy[0];row[1]+=dy[1];row[2]+=dy[2];
        }stats_.pixels+=written;stats_.tintPixels+=tinted;stats_.alphaSpanPixels+=written;++stats_.alphaSpanTriangles;return;
    }

    const bool inkWrap=s.addressU==1&&s.addressV==1;
    const bool inkBounds=additiveBounds_&&texture&&texture->decodedInkBounds&&(texture->inkX1<0||texture->inkX0>0||texture->inkY0>0||texture->inkX1<int(texture->width)-1||texture->inkY1<int(texture->height)-1)&&(inkWrap||((s.addressU==3||s.addressU==4)&&(s.addressV==3||s.addressV==4)));
    // Constant-colour opaque-alpha additive sprites (fire/explosions):
    // depth test remains enabled when requested; alpha/tint key is fixed.
    if(effectSpans_&&particleSpans_&&contribution&&s.screenDepth&&constantDepth&&!perspectiveUV&&!s.perspectiveColor&&!s.zWrite&&!hudOverlay_){
        uint64_t written=0,black=0,prepared=0;
        for(int y=y0;y<=y1;++y){int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_)radarSpan(y,x0,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),p[3]={row[0],row[1],row[2]};
            for(int skip=0;skip<first;++skip)for(unsigned i=0;i<3;++i)p[i]+=dx[i];
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            for(int x=x0+first;x<=x0+last;++x){if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){size_t index=size_t(y)*stride()+x;
                if(!s.zTest||flatDepth>=depth_[index]){uint16_t tex;
                    if(inkBounds&&outsideInk(p[1],p[2],*texture,s.linear,inkWrap)){tex=0;++stats_.additiveBoundsPixels;}
                    else if(s.linear){unsigned alpha;filtered(p[1],p[2],*texture,s.addressU,s.addressV,tex,alpha,s.alphaRef>0,true,uniformTexture,vectorFilter_&&(filterStages_&4),vectorFilter_&&(filterStages_&1),false,flatA,s.alphaRef,preparedFilter,stats_,worldChannels_);}
                    else{size_t t;if(preparedFilter){t=size_t(filterAxes[1].sample(p[2]))*texture->width+filterAxes[0].sample(p[1]);++prepared;}else t=size_t(sample(p[2],texture->height,s.addressV))*texture->width+sample(p[1],texture->width,s.addressU);tex=texture->pixels[t];}
                    if(!tex)++black;else{unsigned old=color_[index],dr=old>>11,dg=(old>>5)&63,db=old&31;
                        color_[index]=rgb(std::min(255u,((dr<<3)|(dr>>2))+contribution->red[tex>>11]),std::min(255u,((dg<<2)|(dg>>4))+contribution->green[(tex>>5)&63]),std::min(255u,((db<<3)|(db>>2))+contribution->blue[tex&31]));}
                    ++written;
                }
            }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;for(unsigned i=0;i<3;++i)p[i]+=dx[i];}
            row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(unsigned i=0;i<8;++i)row[i]+=dy[i];
        }stats_.pixels+=written;stats_.blackAddPixels+=black;stats_.preparedNearestPixels+=prepared;stats_.particleEffectPixels+=written;return;
    }
    if(effectSpans_&&((whiteTextureFast&&s.linear&&s.blend>=1)||(skyChannels_&&!texture&&!constantColor))&&!s.perspectiveColor&&!perspectiveUV&&!s.zTest&&!s.zWrite&&!hudOverlay_&&s.blend<=3){
        uint64_t written=0;const bool grouped=skyChannels_&&worldChannelsAvailable&&!constantColor&&(!texture||opaqueTexture);
        for(int y=y0;y<=y1;++y){int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_&&s.screenDepth)radarSpan(y,x0,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),p[8];std::copy(row,row+8,p);
            for(int skip=0;skip<first;++skip){for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];if(s.screenDepth&&!constantDepth)p[7]+=dx[7];}
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            for(int x=x0+first;x<=x0+last;++x){if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){
                unsigned rgba[4];if(grouped)worldChannels(p+3,1.f,rgba);
                unsigned alpha=constantColor?flatA:grouped?rgba[3]:channel(p[6]),ta;uint16_t tex;
                if(texture){filtered(p[1],p[2],*texture,s.addressU,s.addressV,tex,ta,s.alphaRef>0,opaqueTexture,true,false,vectorFilter_&&(filterStages_&1),false,alpha,s.alphaRef,preparedFilter,stats_,worldChannels_);
                if(!opaqueTexture)alpha=alpha*ta/255;}
                if(alpha>=s.alphaRef){unsigned r=constantColor?flatR:grouped?rgba[0]:channel(p[3]),g=constantColor?flatG:grouped?rgba[1]:channel(p[4]),blue=constantColor?flatB:grouped?rgba[2]:channel(p[5]);size_t index=size_t(y)*stride()+x;
                    if(s.blend==1&&alpha!=255){unsigned old=color_[index],dr=old>>11,dg=(old>>5)&63,db=old&31;
                        r=(r*alpha+((dr<<3)|(dr>>2))*(255-alpha)+127)/255;g=(g*alpha+((dg<<2)|(dg>>4))*(255-alpha)+127)/255;blue=(blue*alpha+((db<<3)|(db>>2))*(255-alpha)+127)/255;}
                    else if(s.blend>=2){unsigned old=color_[index],dr=old>>11,dg=(old>>5)&63,db=old&31,amount=s.blend==2?alpha:255;
                        r=std::min(255u,((dr<<3)|(dr>>2))+(r*amount+127)/255);g=std::min(255u,((dg<<2)|(dg>>4))+(g*amount+127)/255);blue=std::min(255u,((db<<3)|(db>>2))+(blue*amount+127)/255);}
                    color_[index]=rgb(r,g,blue);++written;
                }
            }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];if(s.screenDepth&&!constantDepth)p[7]+=dx[7];}
            row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(int i=0;i<8;++i)row[i]+=dy[i];
        }stats_.pixels+=written;stats_.spriteEffectPixels+=written;if(grouped)stats_.skyColorPixels+=written;return;
    }
    // ONE+ONE world effects with an opaque alpha plane. Alpha rejection can
    // precede texel fetch; no HUD, depth writes or blend dispatch in this loop.
    if((effectSpans_||rainActive_)&&(blackAdd_||rainActive_)&&opaqueTexture&&!s.linear&&!s.screenDepth&&s.zTest&&!s.zWrite&&!hudOverlay_&&s.blend==3){
        const float minDepth=std::min({a.reciprocalZ,b.reciprocalZ,c.reciprocalZ});
        const float minimumRainDepth=std::max(0.f,minDepth-(1e-4f+1e-5f*std::max({std::fabs(a.reciprocalZ),std::fabs(b.reciprocalZ),std::fabs(c.reciprocalZ)})));
        auto draw=[&](bool track){
        uint64_t written=0,black=0,prepared=0,saved=0;
        for(int y=y0;y<=y1;++y){float rowInverse=rowReciprocal&&row[0]>1e-8f?1/row[0]:1;int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);
            float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),p[8];std::copy(row,row+8,p);
            for(int skip=0;skip<first;++skip)for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];
            stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
            for(int x=x0+first;x<=x0+last;++x){if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){size_t index=size_t(y)*stride()+x;
                if(p[0]>depth_[index]){float iz=rowReciprocal?rowInverse:(perspectiveUV||(s.perspectiveColor&&!constantColor))&&p[0]>1e-8f?1/p[0]:1;if(rowReciprocal)++saved;
                    float cs=s.perspectiveColor?iz:1,us=perspectiveUV?iz:1;unsigned alpha=constantColor?flatA:channel(p[6]*cs);
                    if(alpha>=s.alphaRef){size_t t;if(preparedFilter){t=size_t(filterAxes[1].sample(p[2]*us))*texture->width+filterAxes[0].sample(p[1]*us);++prepared;}else t=size_t(sample(p[2]*us,texture->height,s.addressV))*texture->width+sample(p[1]*us,texture->width,s.addressU);
                        unsigned tex=uniformTexture?texture->pixels[0]:texture->pixels[t];
                        if(!tex)++black;else{unsigned r=constantColor?flatR:channel(p[3]*cs),g=constantColor?flatG:channel(p[4]*cs),blue=constantColor?flatB:channel(p[5]*cs);
                            if(!white){unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;r=r*((tr<<3)|(tr>>2))/255;g=g*((tg<<2)|(tg>>4))/255;blue=blue*((tb<<3)|(tb>>2))/255;}
                            else{unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;r=(tr<<3)|(tr>>2);g=(tg<<2)|(tg>>4);blue=(tb<<3)|(tb>>2);}
                            if(track){auto& sum=rainRGB_[index];sum[0]=uint16_t(std::min(255u,unsigned(sum[0])+r));sum[1]=uint16_t(std::min(255u,unsigned(sum[1])+g));sum[2]=uint16_t(std::min(255u,unsigned(sum[2])+blue));if(r||g||blue)rainMinimum_[index]=std::min(rainMinimum_[index],minimumRainDepth);}
                            else{unsigned old=color_[index],dr=old>>11,dg=(old>>5)&63,db=old&31;
                            color_[index]=rgb(std::min(255u,((dr<<3)|(dr>>2))+r),std::min(255u,((dg<<2)|(dg>>4))+g),std::min(255u,((db<<3)|(db>>2))+blue));}
                        }++written;
                    }
                }
            }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];}
            row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(int i=0;i<8;++i)row[i]+=dy[i];
        }stats_.pixels+=written;stats_.blackAddPixels+=black;stats_.preparedNearestPixels+=prepared;stats_.perspectiveSaved+=saved;stats_.additiveWorldPixels+=written;
        };if(rainActive_)draw(true);else draw(false);return;
    }

    for(int y=y0;y<=y1;++y){float rowInverse=rowReciprocal&&row[0]>1e-8f?1/row[0]:1;int first,last;rasterSpan(row0,row1,row2,b.y-c.y,c.y-a.y,a.y-b.y,x1-x0+1,first,last);if(radarClip_&&s.screenDepth)radarSpan(y,x0,first,last);
        float e0=row0+first*(b.y-c.y),e1=row1+first*(c.y-a.y),e2=row2+first*(a.y-b.y),p[8];std::copy(row,row+8,p);
        // Preserve the previous attribute accumulation at skipped pixels.
        // Only edge tests/edge increments and pixel work are eliminated.
        for(int skip=0;skip<first;++skip){for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];if(s.screenDepth&&!constantDepth)p[7]+=dx[7];}
        stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
        for(int x=x0+first;x<=x0+last;++x){if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))){
            float z=constantDepth?flatDepth:s.screenDepth?p[7]:p[0];size_t index=size_t(y)*stride()+x;
            if(!s.zTest||(s.screenDepth?z>=depth_[index]:z>depth_[index])){
                float iz=rowReciprocal?rowInverse:(perspectiveUV||(s.perspectiveColor&&!constantColor))&&p[0]>1e-8f?1/p[0]:1;
                if(rowReciprocal)++stats_.perspectiveSaved;
                float colorScale=s.perspectiveColor?iz:1,uvScale=perspectiveUV?iz:1;
                unsigned alpha=constantColor?flatA:channel(p[6]*colorScale);uint16_t tex=0;
                if(vectorFilter_&&(filterStages_&8)&&alpha<s.alphaRef){++stats_.vertexAlphaRejected;}else{
                if(texture){unsigned ta;if(s.linear)filtered(p[1]*uvScale,p[2]*uvScale,*texture,s.addressU,s.addressV,tex,ta,s.alphaRef>0,opaqueTexture,uniformTexture||blackSource,vectorFilter_&&(filterStages_&4),vectorFilter_&&(filterStages_&1),vectorFilter_&&(filterStages_&8),alpha,s.alphaRef,preparedFilter,stats_,worldChannels_);else{size_t t;if(preparedFilter){t=size_t(filterAxes[1].sample(p[2]*uvScale))*texture->width+filterAxes[0].sample(p[1]*uvScale);++stats_.preparedNearestPixels;}else t=size_t(sample(p[2]*uvScale,texture->height,s.addressV))*texture->width+sample(p[1]*uvScale,texture->width,s.addressU);tex=blackSource?0:uniformTexture?texture->pixels[0]:texture->pixels[t];ta=opaqueTexture?255:texture->alpha[t];}if(!opaqueTexture)alpha=alpha*ta/255;}
                if(alpha>=s.alphaRef){
                    const bool blackAdd=texture&&tex==0&&(s.blend==2||s.blend==3||s.blend==4);
                    if(blackAdd)++stats_.blackAddPixels;
                    if(blackFade){color_[index]=(*blackFade)(color_[index],alpha);if(s.zWrite)depth_[index]=z;++stats_.pixels;++stats_.blackFadePixels;}
                    else if(blackAdd&&blackAdd_){if(s.zWrite)depth_[index]=z;++stats_.pixels;}
                    else if(contribution){++stats_.additiveCachedPixels;uint16_t old=color_[index];unsigned dr=old>>11,dg=(old>>5)&63,db=old&31;
                        unsigned r=std::min(255u,((dr<<3)|(dr>>2))+contribution->red[tex>>11]);unsigned g=std::min(255u,((dg<<2)|(dg>>4))+contribution->green[(tex>>5)&63]);unsigned b=std::min(255u,((db<<3)|(db>>2))+contribution->blue[tex&31]);
                        color_[index]=rgb(r,g,b);if(s.zWrite)depth_[index]=z;++stats_.pixels;
                    }
                    else if(tintTable&&(s.blend==0||(s.blend==1&&alpha==255))){
                        color_[index]=constantTextureColor?uniformPixel:uint16_t(tintTable->red[tex>>11]|tintTable->green[(tex>>5)&63]|tintTable->blue[tex&31]);
                        if(s.zWrite)depth_[index]=z;++stats_.pixels;++stats_.tintPixels;
                    }else{
                    unsigned r=constantTextureColor?uniformR:constantColor?flatR:channel(p[3]*colorScale),g=constantTextureColor?uniformG:constantColor?flatG:channel(p[4]*colorScale),blue=constantTextureColor?uniformB:constantColor?flatB:channel(p[5]*colorScale);
                    if(whiteTextureFast&&!constantTextureColor)++stats_.whiteTexturePixels;
                    if(texture&&!white&&!constantTextureColor&&!whiteTextureFast){unsigned tr=(tex>>11)&31,tg=(tex>>5)&63,tb=tex&31;r=r*((tr<<3)|(tr>>2))/255;g=g*((tg<<2)|(tg>>4))/255;blue=blue*((tb<<3)|(tb>>2))/255;}
                    if(flatFill){color_[index]=flatTable(color_[index]);if(s.zWrite)depth_[index]=z;++stats_.pixels;}
                    else if(hudOverlay_){
                        if(texture&&white&&!constantTextureColor){unsigned tr=tex>>11,tg=(tex>>5)&63,tb=tex&31;r=(tr<<3)|(tr>>2);g=(tg<<2)|(tg>>4);blue=(tb<<3)|(tb>>2);}
                        overlayPixel(index,r,g,blue,alpha,s.blend);if(s.zWrite)depth_[index]=z;++stats_.pixels;
                    }else if(texture&&white&&(s.blend==0||(s.blend==1&&alpha==255))){color_[index]=tex;if(s.zWrite)depth_[index]=z;++stats_.pixels;}
                    else {if(texture&&white&&!constantTextureColor){unsigned tr=(tex>>11)&31,tg=(tex>>5)&63,tb=tex&31;r=(tr<<3)|(tr>>2);g=(tg<<2)|(tg>>4);blue=(tb<<3)|(tb>>2);}
                    if(s.blend&&!(s.blend==1&&alpha==255)){uint16_t old=color_[index];unsigned dr=(old>>11)&31,dg=(old>>5)&63,db=old&31;dr=(dr<<3)|(dr>>2);dg=(dg<<2)|(dg>>4);db=(db<<3)|(db>>2);
                        if(s.blend==1){if(vectorFilter_&&(filterStages_&4)){uint16_t blended=vectorSourceOver565(r,g,blue,alpha,dr,dg,db);r=((blended>>11)<<3);g=(((blended>>5)&63)<<2);blue=((blended&31)<<3);++stats_.vectorBlendPixels;}else{r=(r*alpha+dr*(255-alpha)+127)/255;g=(g*alpha+dg*(255-alpha)+127)/255;blue=(blue*alpha+db*(255-alpha)+127)/255;}}
                        else if(s.blend==4){r=(dr*(255-r)+127)/255;g=(dg*(255-g)+127)/255;blue=(db*(255-blue)+127)/255;}
                        else if(s.blend==5){r=dr;g=dg;blue=db;}
                        else{unsigned amount=s.blend==2?alpha:255;r=std::min(255u,dr+(r*amount+127)/255);g=std::min(255u,dg+(g*amount+127)/255);blue=std::min(255u,db+(blue*amount+127)/255);}}
                    color_[index]=rgb(r,g,blue);if(s.zWrite)depth_[index]=z;++stats_.pixels;}
                    }
                }
                }
            }
        }e0+=b.y-c.y;e1+=c.y-a.y;e2+=a.y-b.y;for(int i=0;i<(constantColor?3:7);++i)p[i]+=dx[i];if(s.screenDepth&&!constantDepth)p[7]+=dx[7];}
        row0+=c.x-b.x;row1+=a.x-c.x;row2+=b.x-a.x;for(int i=0;i<8;++i)row[i]+=dy[i];
    }
}
}
