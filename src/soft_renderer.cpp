#include <cstdio>
#include "tile_depth.hpp"
#include "depth_tile_scan.hpp"
#include "sky_delta.hpp"
#include "rain_composite.hpp"
#include "expand_rows.hpp"
#include "raster_span.hpp"
#include <stdexcept>
#include "soft_renderer.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>

namespace soft {
Vec3 view(Vec3 p, const Camera& c) {
    p.x-=c.position.x; p.y-=c.position.y; p.z-=c.position.z;
    const float co=std::cos(c.yaw), si=std::sin(c.yaw);
    return {co*p.x-si*p.z, p.y, si*p.x+co*p.z};
}
Renderer::Renderer(): color_(width*height), depth_(width*height) { clear(0); }
void Renderer::renderScale(unsigned scale){
 if(scale<1||scale>4)throw std::invalid_argument("Invalid render scale");
 renderWorldWidth(scale==1?320:scale==2?160:scale==3?240:80);
}
void Renderer::renderWorldWidth(unsigned width){
 if(width!=80&&width!=160&&width!=200&&width!=240&&width!=280&&width!=320)throw std::invalid_argument("Invalid world width");
 const float factor=320.f/width;flushDeferred();endRain();endSky();if(requestedScale_==factor)return;
 requestedScale_=factor;scale_=factor;expandedValid_=false;clear(0);
}
bool Renderer::beginRain(unsigned target){flushDeferred();
    endSky();if(rainActive_||nativeUi_||hudOverlay_||target!=120||renderWidth()<=120)return false;
    constexpr unsigned lowW=120,lowH=90;const unsigned worldW=renderWidth(),worldH=renderHeight();
    try{rainColor_.resize(lowW*lowH);rainDepth_.resize(lowW*lowH);rainMinimum_.resize(lowW*lowH);rainRGB_.resize(lowW*lowH);}catch(const std::bad_alloc&){++stats_.rainFallbacks;return false;}
    std::fill(rainColor_.begin(),rainColor_.end(),0);std::fill(rainMinimum_.begin(),rainMinimum_.end(),std::numeric_limits<float>::infinity());std::fill(rainRGB_.begin(),rainRGB_.end(),std::array<uint16_t,3>{0,0,0});
    // Each low cell owns precisely the full pixels which expand back to it.
    // Maximum world reciprocal depth conservatively rejects occluded rain.
    unsigned first[lowW+1];for(unsigned x=0;x<=lowW;++x)first[x]=(x*worldW+lowW-1)/lowW;
    if(rainComposite_&&worldW==240&&worldH==180){for(unsigned y=0;y<lowH;++y)rainDepthRow2x2(depth_.data()+y*2*320,depth_.data()+(y*2+1)*320,rainDepth_.data()+y*lowW,lowW);}
    else for(unsigned y=0;y<lowH;++y){unsigned y0=(y*worldH+lowH-1)/lowH,y1=((y+1)*worldH+lowH-1)/lowH;
        for(unsigned x=0;x<lowW;++x){float closest=0;for(unsigned fy=y0;fy<y1;++fy)for(unsigned fx=first[x];fx<first[x+1];++fx)closest=std::max(closest,depth_[fy*320+fx]);rainDepth_[y*lowW+x]=closest;}}
    rainWorldWidth_=worldW;rainScale_=scale_;color_.swap(rainColor_);depth_.swap(rainDepth_);rainActive_=true;scale_=320.f/120;expandedValid_=false;invalidateOcclusion();++stats_.rainPasses;return true;
}
void Renderer::cancelRain(){if(!rainActive_)return;color_.swap(rainColor_);depth_.swap(rainDepth_);rainActive_=false;scale_=rainScale_;expandedValid_=false;invalidateOcclusion();}
void Renderer::endRain(){
    if(!rainActive_)return;const unsigned worldW=rainWorldWidth_,worldH=worldW*3/4;
    color_.swap(rainColor_);depth_.swap(rainDepth_);rainActive_=false;scale_=rainScale_;
    unsigned columns[320];for(unsigned x=0;x<worldW;++x)columns[x]=x*120/worldW;
    uint64_t written=0;
    if(rainComposite_){uint16_t red[320],green[320],blue[320];float minimum[320];unsigned previous=~0u,first=0,last=0;
        for(unsigned y=0;y<worldH;++y){unsigned row=y*90/worldH;
            if(row!=previous){previous=row;first=worldW;last=0;for(unsigned x=0;x<worldW;++x){unsigned src=row*120+columns[x];auto sum=rainRGB_[src];red[x]=sum[0];green[x]=sum[1];blue[x]=sum[2];minimum[x]=rainMinimum_[src];if(sum[0]|sum[1]|sum[2]){first=std::min(first,x);last=x;}}}
            if(first<=last){unsigned count=last-first+1;written+=rainCompositeRow(color_.data()+y*320+first,depth_.data()+y*320+first,red+first,green+first,blue+first,minimum+first,count);stats_.rainVectorPixels+=count;}
        }
    }else for(unsigned y=0;y<worldH;++y)for(unsigned x=0;x<worldW;++x){unsigned src=(y*90/worldH)*120+columns[x];auto sum=rainRGB_[src];if(!(sum[0]|sum[1]|sum[2]))continue;size_t dst=y*320+x;
        // Whole-triangle minimum depth plus an error allowance also protects
        // building/car silhouettes when a triangle slopes within a low cell.
        if(!(depth_[dst]<rainMinimum_[src]))continue;
        unsigned old=color_[dst],r=old>>11,g=(old>>5)&63,b=old&31;color_[dst]=rgb(std::min(255u,((r<<3)|(r>>2))+sum[0]),std::min(255u,((g<<2)|(g>>4))+sum[1]),std::min(255u,((b<<3)|(b>>2))+sum[2]));++written;
    }stats_.rainCompositePixels+=written;expandedValid_=false;invalidateOcclusion();
}
bool Renderer::beginSky(unsigned target){flushDeferred();
    endRain();
    if(skyActive_||nativeUi_||hudOverlay_||(target!=160&&target!=120)||target>=renderWidth())return false;
    skyScale_=scale_;skyWorldWidth_=renderWidth();skyWidth_=target;
    skyBase_.resize(width*height);color_.swap(skyBase_);
    const unsigned lowH=target*3/4,worldH=skyWorldWidth_*3/4;
    unsigned columns[160];for(unsigned x=0;x<target;++x)columns[x]=x*skyWorldWidth_/target;
    for(unsigned y=0;y<lowH;++y){size_t row=(y*worldH/lowH)*width;for(unsigned x=0;x<target;++x)color_[y*width+x]=skyBase_[row+columns[x]];}
    scale_=float(width)/target;skyActive_=true;expandedValid_=false;invalidateOcclusion();++stats_.skyPasses;return true;
}
void Renderer::cancelSky(){if(!skyActive_)return;color_.swap(skyBase_);scale_=skyScale_;skyActive_=false;expandedValid_=false;invalidateOcclusion();}
void Renderer::endSky(){
    if(!skyActive_)return;
    const unsigned lowW=skyWidth_,lowH=lowW*3/4,worldW=skyWorldWidth_,worldH=worldW*3/4;
    // Read the complete immutable original before writing destinations: a
    // destination can also be a later low-resolution reference sample.
    // Encode signed RGB565-component deltas into 6/7/6-bit fields (19 bits).
    // A bounded low-resolution delta plane; no depth copy.
    std::vector<uint32_t>& deltas=skyDeltas_;deltas.resize(lowW*lowH);
    unsigned sourceColumns[160];for(unsigned x=0;x<lowW;++x)sourceColumns[x]=x*worldW/lowW;
    for(unsigned y=0;y<lowH;++y)for(unsigned x=0;x<lowW;++x){
        unsigned result=color_[y*width+x],base=skyBase_[(y*worldH/lowH)*width+sourceColumns[x]];
        int r=int(result>>11)-int(base>>11),g=int((result>>5)&63)-int((base>>5)&63),b=int(result&31)-int(base&31);
        deltas[y*lowW+x]=unsigned(r+31)|(unsigned(g+63)<<6)|(unsigned(b+31)<<13);
    }
    const uint32_t zero=31u|(63u<<6)|(31u<<13);
    unsigned targetColumns[320];for(unsigned x=0;x<worldW;++x)targetColumns[x]=x*lowW/worldW;uint64_t composite=0;
    if(skyComposite_){uint32_t expanded[320];unsigned previous=~0u,nonzero=0,first=0,last=0;
        for(unsigned y=0;y<worldH;++y){unsigned lowY=y*lowH/worldH;
            if(lowY!=previous){previous=lowY;nonzero=0;first=worldW;last=0;for(unsigned x=0;x<worldW;++x){expanded[x]=deltas[lowY*lowW+targetColumns[x]];if(expanded[x]!=zero){++nonzero;first=std::min(first,x);last=x;}}}
            if(nonzero){skyDeltaRow(skyBase_.data()+y*width+first,expanded+first,last-first+1);stats_.skyVectorPixels+=last-first+1;}
            composite+=nonzero;
        }
    }else
    for(unsigned y=0;y<worldH;++y)for(unsigned x=0;x<worldW;++x){
        uint32_t delta=deltas[(y*lowH/worldH)*lowW+targetColumns[x]];if(delta==zero)continue;
        size_t dst=y*width+x;unsigned old=skyBase_[dst];
        auto clamp=[](int v,int top){return unsigned(std::max(0,std::min(top,v)));};
        unsigned r=clamp(int(old>>11)+int(delta&63)-31,31);
        unsigned g=clamp(int((old>>5)&63)+int((delta>>6)&127)-63,63);
        unsigned b=clamp(int(old&31)+int((delta>>13)&63)-31,31);
        skyBase_[dst]=uint16_t((r<<11)|(g<<5)|b);++composite;
    }
    stats_.skyCompositePixels+=composite;color_.swap(skyBase_);scale_=skyScale_;skyActive_=false;expandedValid_=false;invalidateOcclusion();
}
void Renderer::resolveFullResolution(){flushDeferred();endRain();endSky();invalidateOcclusion();if(hudOverlay_)flushHudOverlay();
    if(nativeUi_||scale_==1)return;color_=pixels();
    // Expand backwards in place so unread reduced-depth samples remain intact.
    for(int y=height-1;y>=0;--y)for(int x=width-1;x>=0;--x)
        depth_[size_t(y)*stride()+x]=depth_[size_t(unsigned(y)*renderHeight()/height)*width+unsigned(x)*renderWidth()/width];
    scale_=1;expandedValid_=false;
}
void Renderer::restoreWorld(){invalidateOcclusion();if(!nativeUi_&&!hudOverlay_)return;hudOverlay_=false;color_.swap(uiColor_);depth_.swap(uiDepth_);nativeUi_=false;}
void Renderer::resolveNativeUi(){flushDeferred();invalidateOcclusion();if(nativeUi_)return;resolveFullResolution();uiColor_.resize(640*480);uiDepth_.resize(640*480);
    if(rowExpansion_){expandRows565(uiColor_.data(),640,640,480,color_.data(),320,320,240);stats_.rowExpandPixels+=640*480;for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x)uiDepth_[y*640+x]=depth_[(y/2)*320+x/2];}else
    for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x){size_t src=(y/2)*320+x/2,dst=y*640+x;uiColor_[dst]=color_[src];uiDepth_[dst]=depth_[src];}
    color_.swap(uiColor_);depth_.swap(uiDepth_);nativeUi_=true;scale_=.5f;expandedValid_=false;
}
void Renderer::beginBackground(unsigned width){flushDeferred();invalidateOcclusion();
    // Menus may overlay a world frame whose HUD already used the native target.
    if(width!=640&&nativeUi_){auto image=color_;restoreWorld();color_.resize(320*240);depth_.assign(320*240,0.f);
        for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x)color_[y*320+x]=image[y*2*640+x*2];scale_=1;expandedValid_=false;}
    beginHud(width);
}
void Renderer::beginHud(unsigned width){flushDeferred();endRain();endSky();invalidateOcclusion();
    if(width!=240){beginUi(width==640);return;}
    if(hudOverlay_){clearUiDepth();return;}
    if(nativeUi_)throw std::logic_error("HUD overlay must precede native menu");
    if(scale_!=1)color_=pixels();scale_=1;
    uiColor_.resize(320*240);uiDepth_.resize(320*240);
    color_.swap(uiColor_);depth_.swap(uiDepth_);
    clearHudLayer();std::fill(depth_.begin(),depth_.end(),0.f);
    hudOverlay_=true;scale_=4.f/3.f;expandedValid_=false;
}
// Conservative triangle bounds include transparent/depth-rejected pixels too.
// Every color write to hudLayer_ goes through screenTriangleChecked; blend5
// only writes radar depth. Track even in full-reference mode for live toggles.
void Renderer::markHudBounds(int x0,int y0,int x1,int y1){
    x0=std::max(0,x0);x1=std::min(239,x1);if(x0>x1)return;
    for(int y=std::max(0,y0);y<=std::min(179,y1);++y){hudFirst_[y]=std::min(hudFirst_[y],uint16_t(x0));hudLast_[y]=std::max(hudLast_[y],uint16_t(x1));}
}
void Renderer::clearHudLayer(){
    if(!sparseHud_||hudLayer_.size()!=320*240)hudLayer_.assign(320*240,0);
    else for(unsigned y=0;y<180;++y)if(hudFirst_[y]<=hudLast_[y])std::fill(hudLayer_.begin()+y*320+hudFirst_[y],hudLayer_.begin()+y*320+hudLast_[y]+1,0u);
    hudFirst_.fill(240);hudLast_.fill(0);
}
void Renderer::overlayPixel(size_t index,unsigned r,unsigned g,unsigned b,unsigned a,unsigned blend){
    if(blend==5)return; // Radar mask affects depth only.
    if(opaqueOverlay_){
        if(blend==0||a==255){hudLayer_[index]=0xff000000u|(r<<16)|(g<<8)|b;return;}
        if(!a)return;
    }
    uint32_t old=hudLayer_[index];unsigned oa=old>>24,orr=(old>>16)&255,og=(old>>8)&255,ob=old&255;
    if(blend==0){a=255;}else{
        r=(r*a+orr*(255-a)+127)/255;g=(g*a+og*(255-a)+127)/255;b=(b*a+ob*(255-a)+127)/255;
        a=a+(oa*(255-a)+127)/255;
    }
    hudLayer_[index]=(a<<24)|(r<<16)|(g<<8)|b;
}
void Renderer::flushHudOverlay(){invalidateOcclusion();
    if(!hudOverlay_)return;auto image=pixels();std::vector<float> depth(320*240);
    for(unsigned y=0;y<240;++y)for(unsigned x=0;x<320;++x)depth[y*320+x]=depth_[(y*180/240)*320+x*240/320];
    color_.swap(uiColor_);depth_.swap(uiDepth_);hudOverlay_=false;
    color_=std::move(image);depth_=std::move(depth);scale_=1;expandedValid_=false;
}
void Renderer::beginUi(bool native){flushDeferred();endRain();endSky();invalidateOcclusion();if(hudOverlay_)flushHudOverlay();if(nativeUi_){clearUiDepth();return;}
    if(native){unsigned w=renderWidth(),h=renderHeight();uiColor_.resize(640*480);uiDepth_.resize(640*480);std::fill(uiDepth_.begin(),uiDepth_.end(),0.f);
        if(rowExpansion_){expandRows565(uiColor_.data(),640,640,480,color_.data(),320,w,h);stats_.rowExpandPixels+=640*480;}else
        for(unsigned y=0;y<480;++y){size_t src=size_t(y*h/480)*320;for(unsigned x=0;x<640;++x)uiColor_[y*640+x]=color_[src+x*w/640];}
        color_.swap(uiColor_);depth_.swap(uiDepth_);nativeUi_=true;scale_=.5f;
    }else{if(scale_!=1)color_=pixels();scale_=1;clearUiDepth();}expandedValid_=false;
}
const std::vector<uint16_t>& Renderer::pixels() const {
    const_cast<Renderer*>(this)->flushDeferred();
    if(rainActive_)const_cast<Renderer*>(this)->endRain();
    if(hudOverlay_){
        if(!expandedValid_){expanded_=uiColor_;
            unsigned sampled=0;
            for(unsigned y=0;y<240;++y){unsigned row=y*180/240,first=0,last=319;
                if(sparseHud_){if(hudFirst_[row]>hudLast_[row])continue;first=(unsigned(hudFirst_[row])*320+239)/240;last=((unsigned(hudLast_[row])+1)*320+239)/240-1;}
                sampled+=last-first+1;
                for(unsigned x=first;x<=last;++x){uint32_t layer=hudLayer_[row*320+x*240/320];unsigned a=layer>>24;if(!a)continue;
                if(opaqueOverlay_&&a==255){expanded_[y*320+x]=rgb((layer>>16)&255,(layer>>8)&255,layer&255);continue;}
                uint16_t base=uiColor_[y*320+x];unsigned r=base>>11,g=(base>>5)&63,b=base&31;r=(r<<3)|(r>>2);g=(g<<2)|(g>>4);b=(b<<3)|(b>>2);
                r=std::min(255u,((layer>>16)&255)+(r*(255-a)+127)/255);g=std::min(255u,((layer>>8)&255)+(g*(255-a)+127)/255);b=std::min(255u,(layer&255)+(b*(255-a)+127)/255);
                expanded_[y*320+x]=rgb(r,g,b);
            }}auto& counts=const_cast<Stats&>(stats_);counts.hudCompositeSamples+=sampled;if(sparseHud_)++counts.hudSparseResolves;expandedValid_=true;
        }return expanded_;
    }
    if(nativeUi_||scale_==1)return color_;
    if(!expandedValid_){expanded_.resize(width*height);
        if(rowExpansion_){expandRows565(expanded_.data(),stride(),width,height,color_.data(),width,renderWidth(),renderHeight());const_cast<Stats&>(stats_).rowExpandPixels+=width*height;}else
        for(int y=0;y<height;++y)for(int x=0;x<width;++x)
            expanded_[size_t(y)*stride()+x]=color_[size_t(unsigned(y)*renderHeight()/height)*width+unsigned(x)*renderWidth()/width];
        expandedValid_=true;
    }return expanded_;
}
void Renderer::clear(uint16_t c) {flushDeferred();finishLazyFrame();cancelRain();cancelSky();radarClip_=false;invalidateOcclusion();occlusionBuilds_=0;
    restoreWorld();scale_=requestedScale_;expandedValid_=false;
    std::fill(color_.begin(),color_.end(),c);
    std::fill(depth_.begin(),depth_.end(),0.f); stats_={};deferredCommands_=deferredBatches_=deferredFallbacks_=0;
}
void Renderer::clearBuffers(uint16_t c,bool image,bool depth){flushDeferred();finishLazyFrame();if(image)cancelRain();else endRain();cancelSky();radarClip_=false;invalidateOcclusion();occlusionBuilds_=0;
    restoreWorld();scale_=requestedScale_;expandedValid_=false;
    if(image)std::fill(color_.begin(),color_.end(),c);
    if(depth)std::fill(depth_.begin(),depth_.end(),0.f);
    stats_={};deferredCommands_=deferredBatches_=deferredFallbacks_=0;
}
static bool finite(Vec3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
void Renderer::triangle(const Vec3* v, uint16_t color, const Camera& c) {
    ++stats_.submitted;
    if (!std::isfinite(c.nearZ)||!std::isfinite(c.farZ)||!std::isfinite(c.focal)||
        c.nearZ<0.001f||c.farZ<=c.nearZ||c.farZ>10000.f||c.focal<1.f||c.focal>10000.f) {
        ++stats_.rejected; return;
    }
    // Reject unbounded input; prevents overflowing interpolation arithmetic.
    for (int i=0;i<3;++i) if (!finite(v[i]) || std::fabs(v[i].x)>1e6f ||
        std::fabs(v[i].y)>1e6f || std::fabs(v[i].z)>1e6f) { ++stats_.rejected; return; }
    Vec3 a[16], b[16]; std::copy(v,v+3,a); int n=3;
    const float sx=width*0.5f/c.focal, sy=height*0.5f/c.focal;
    unsigned combined=0,common=63;for(int i=0;i<3;++i){unsigned code=frustumCode(v[i],c);combined|=code;common&=code;}
    if(common){++stats_.rejected;return;}
    for (int plane=0;plane<6 && n && combined; ++plane) {
        auto distance=[&](Vec3 p) {
            switch(plane) {
                case 0:return p.z-c.nearZ; case 1:return c.farZ-p.z;
                case 2:return p.x+sx*p.z; case 3:return sx*p.z-p.x;
                case 4:return p.y+sy*p.z; default:return sy*p.z-p.y;
            }
        };
        int m=0; Vec3 prev=a[n-1]; float dp=distance(prev);
        for (int i=0;i<n;++i) {
            Vec3 cur=a[i]; float dc=distance(cur);
            if ((dp>=0)!=(dc>=0)) {
                float t=dp/(dp-dc);
                b[m++]={prev.x+t*(cur.x-prev.x),prev.y+t*(cur.y-prev.y),prev.z+t*(cur.z-prev.z)};
            }
            if (dc>=0) b[m++]=cur;
            prev=cur; dp=dc;
        }
        n=m; std::copy(b,b+n,a);
    }
    if(n<3) { ++stats_.rejected; return; }
    for(int i=1;i<n-1;++i) raster(a[0],a[i],a[i+1],color,c.focal);
}
void Renderer::worldTriangle(const Triangle& t, const Camera& c) {
    Vec3 v[3]; for(int i=0;i<3;++i) v[i]=view(t.v[i],c); triangle(v,t.color,c);
}
// Edge equations increment across a scanline; no per-pixel divisions.
void Renderer::raster(Vec3 a, Vec3 b, Vec3 c, uint16_t color, float focal) {
    if(deferredActive_&&!deferredReplaying_){ScreenVertex v[3]{};Vec3 p[3]={a,b,c};for(unsigned i=0;i<3;++i){v[i].x=p[i].x;v[i].y=p[i].y;v[i].reciprocalZ=p[i].z;}if(deferCommand(v,nullptr,ScreenState{},true,nullptr,2,color,0,0,0,focal))return;}
    expandedValid_=false;
    auto project=[&](Vec3 p) { float iz=1.f/p.z;
        return Vec3{(width*0.5f+focal*p.x*iz)/scale_,(height*0.5f-focal*p.y*iz)/scale_,iz}; };
    a=project(a); b=project(b); c=project(c);
    auto edge=[](Vec3 p,Vec3 q,float x,float y) { return (q.x-p.x)*(y-p.y)-(q.y-p.y)*(x-p.x); };
    float area=edge(a,b,c.x,c.y);
    if(std::fabs(area)<1e-5f) return;
    if(area<0) { std::swap(b,c); area=-area; }
    int x0=std::max(0,rasterCeil(std::min({a.x,b.x,c.x})-0.5f));
    int x1=std::min(int(renderWidth())-1,rasterFloor(std::max({a.x,b.x,c.x})-0.5f));
    int y0=std::max(0,rasterCeil(std::min({a.y,b.y,c.y})-0.5f));
    int y1=std::min(int(renderHeight())-1,rasterFloor(std::max({a.y,b.y,c.y})-0.5f));
    if(x0>x1||y0>y1) return;
    ++stats_.rasterized;
    const float inv=1.f/area;
    const float dx0=b.y-c.y, dx1=c.y-a.y, dx2=a.y-b.y;
    const float dy0=c.x-b.x, dy1=a.x-c.x, dy2=b.x-a.x;
    float row0=edge(b,c,x0+0.5f,y0+0.5f), row1=edge(c,a,x0+0.5f,y0+0.5f), row2=edge(a,b,x0+0.5f,y0+0.5f);
    // Top-left ownership avoids drawing both triangles on a shared edge.
    auto topLeft=[](Vec3 p,Vec3 q) { return q.y<p.y || (q.y==p.y && q.x>p.x); };
    bool tl0=topLeft(b,c),tl1=topLeft(c,a),tl2=topLeft(a,b);
    for(;y0<replayLo_;++y0){row0+=dy0;row1+=dy1;row2+=dy2;}y1=std::min(y1,replayHi_-1);if(y0>y1)return;
    for(int y=y0;y<=y1;++y) {
        int first,last;rasterSpan(row0,row1,row2,dx0,dx1,dx2,x1-x0+1,first,last);
        float e0=row0+first*dx0,e1=row1+first*dx1,e2=row2+first*dx2;
        stats_.spanSkippedPixels+=unsigned(x1-x0+1)-unsigned(std::max(0,last-first+1));
        for(int x=x0+first;x<=x0+last;++x) {
            if((e0>0||(e0==0&&tl0))&&(e1>0||(e1==0&&tl1))&&(e2>0||(e2==0&&tl2))) {
                float z=(e0*a.z+e1*b.z+e2*c.z)*inv;
                size_t idx=size_t(y)*stride()+x;
                if(z>depth_[idx]) { depth_[idx]=z; color_[idx]=color; ++stats_.pixels; }
            }
            e0+=dx0; e1+=dx1; e2+=dx2;
        }
        row0+=dy0; row1+=dy1; row2+=dy2;
    }
}
// Reuse the existing immutable tile minima; do not scan depth or alter the
// large-triangle scan schedule. The same conservative recurrence bound applies.
// Demand-filled conservative minima, refreshed at bounded query milestones.
// A cached minimum stays safe while depth only increases. All target swaps,
// clears and unconditional/screen-depth writes invalidate its generation.
// Share storage with legacy snapshots: both scans see current depth minima,
// so either writer can only improve an existing certificate on this target.
bool Renderer::lazyOccluded(int x0,int y0,int x1,int y1,float row,float dx,float dy){
    if(y0<replayLo_||y1>=replayHi_)return false; // Other worker rows are intentionally not seeded.
    if(renderWidth()>320||renderHeight()>240)return false;
    int columns=x1/8-x0/8+1,rows=y1/8-y0/8+1;
    if(columns*rows>16)return false;
    double ex=double(dx)*(x1-x0),ey=double(dy)*(y1-y0);
    double upper=double(row)+std::max(0.0,ex)+std::max(0.0,ey);
    upper+=.002*(std::fabs(double(row))+std::fabs(ex)+std::fabs(ey))+1e-7;
    if(!std::isfinite(upper)||!(upper>0))return false;++stats_.lazyQueries;++lazyFrameProbes_;
    // Any one bbox sample can disprove full occlusion; no min scan needed.
    if(!(upper<depth_[size_t((y0+y1)/2)*stride()+(x0+x1)/2]))return false;
    for(int ty=y0/8;ty<=y1/8;++ty)for(int tx=x0/8;tx<=x1/8;++tx){unsigned index=ty*40+tx;
        if(lazyStamp_[index]!=lazyGeneration_){float minimum=std::numeric_limits<float>::infinity();
            if(tileScan_){unsigned w=std::min(8u,renderWidth()-tx*8),h=std::min(8u,renderHeight()-ty*8);minimum=depthTileMinimum(depth_.data()+size_t(ty*8)*stride()+tx*8,stride(),w,h);stats_.tileScanPixels+=w*h;}
            else for(unsigned y=ty*8;y<std::min(renderHeight(),unsigned(ty*8+8));++y)for(unsigned x=tx*8;x<std::min(renderWidth(),unsigned(tx*8+8));++x){minimum=std::min(minimum,depth_[size_t(y)*stride()+x]);}
            occlusionDepth_[index]=minimum;lazyStamp_[index]=lazyGeneration_;++stats_.lazyTiles;stats_.lazySamples+=std::min(8u,renderWidth()-tx*8)*std::min(8u,renderHeight()-ty*8);
        }
        if(!(upper<occlusionDepth_[index]))return false;
    }++stats_.lazyHidden;++lazyFrameHidden_;if(lazyFrameHidden_>=8&&lazyFrameHidden_*4>=lazyFrameProbes_)lazyFull_=true;return true;
}
bool Renderer::occludedSmall(int x0,int y0,int x1,int y1,float row,float dx,float dy){
    if(y0<replayLo_||y1>=replayHi_)return false; // Other worker rows are intentionally not seeded.
    if(!occlusionEnabled_||!occlusionValid_||nativeUi_||hudOverlay_)return false;
    float minimum=std::numeric_limits<float>::infinity();
    for(int ty=y0/8;ty<=y1/8;++ty)for(int tx=x0/8;tx<=x1/8;++tx){
        float depth=occlusionDepth_[ty*40+tx];if(!(depth>0))return false;
        minimum=std::min(minimum,depth);
    }
    double ex=double(dx)*(x1-x0),ey=double(dy)*(y1-y0);
    double upper=double(row)+std::max(0.0,ex)+std::max(0.0,ey);
    upper+=0.002*(std::fabs(double(row))+std::fabs(ex)+std::fabs(ey))+1e-7;
    if(!std::isfinite(upper)||!(upper>0)||!(upper<minimum))return false;
    ++stats_.smallTileTriangles;return true;
}
bool Renderer::occluded(int x0,int y0,int x1,int y1,float row,float dx,float dy){
    if(y0<replayLo_||y1>=replayHi_)return false; // Other worker rows are intentionally not seeded.
    if(!occlusionEnabled_||nativeUi_||hudOverlay_||(x1-x0+1)*(y1-y0+1)<256)return false;
    ++occlusionCalls_;
    // At most two scans per world frame. A stale minimum remains safe while
    // reciprocal depth can only increase. UI/unconditional writes invalidate it.
    if(occlusionBuilds_<2&&(occlusionCalls_==64||occlusionCalls_==256)){
        const unsigned w=renderWidth(),h=renderHeight();
        for(unsigned ty=0;ty<(h+7)/8;++ty)for(unsigned tx=0;tx<(w+7)/8;++tx){
            float minimum=std::numeric_limits<float>::infinity();
            if(tileScan_)minimum=depthTileMinimum(depth_.data()+ty*8*stride()+tx*8,stride(),std::min(8u,w-tx*8),std::min(8u,h-ty*8));
            else for(unsigned y=ty*8;y<std::min(h,ty*8+8);++y)
                for(unsigned x=tx*8;x<std::min(w,tx*8+8);++x)minimum=std::min(minimum,depth_[y*stride()+x]);
            occlusionDepth_[ty*40+tx]=minimum;
        }
        if(tileScan_)stats_.tileScanPixels+=uint64_t(w)*h;
        occlusionValid_=true;++occlusionBuilds_;++stats_.occlusionBuilds;
    }
    if(!occlusionValid_)return false;
    // Bound the actual incremental interpolant over the entire bounding box,
    // including accumulated float rounding, rather than trusting vertex extrema.
    // <=640+480 additions have <0.001 relative forward error; use twice that.
    double ex=double(dx)*(x1-x0),ey=double(dy)*(y1-y0);
    double upper=double(row)+std::max(0.0,ex)+std::max(0.0,ey);
    upper+=0.002*(std::fabs(double(row))+std::fabs(ex)+std::fabs(ey))+1e-7;
    if(!std::isfinite(upper)||upper<=0)return false;
    ++stats_.occlusionChecks;
    for(int ty=y0/8;ty<=y1/8;++ty)for(int tx=x0/8;tx<=x1/8;++tx){
        double depth=occlusionDepth_[ty*40+tx];
        if(upper<depth)continue;
        if(!tileDepth_||!(depth>0))return false;
        double local=tileDepthUpper(row,dx,dy,x1-x0,y1-y0,
            std::max(x0,tx*8)-x0,std::min(x1,tx*8+7)-x0,
            std::max(y0,ty*8)-y0,std::min(y1,ty*8+7)-y0);
        if(!std::isfinite(local)||!(local<depth))return false;
    }
    ++stats_.occludedTriangles;return true;
}
bool Renderer::writePPM(const std::string& path) const {
    std::ofstream f(path,std::ios::binary); f<<"P6\n"<<outputWidth()<<" "<<outputHeight()<<"\n255\n";
    for(uint16_t c:pixels()) { unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;
        char p[3]={char((r<<3)|(r>>2)),char((g<<2)|(g>>4)),char((b<<3)|(b>>2))}; f.write(p,3); }
    return bool(f);
}
}

namespace soft {
void Renderer::recordCommand(const ScreenVertex* v,const Texture* texture,const ScreenState& state,bool verified,const WorldTriangleSetup* setup,unsigned kind,uint16_t tint,uint8_t au,uint8_t av,unsigned alpha,float focal){
 Command c{};std::copy(v,v+3,c.v);c.texture=texture;c.state=state;c.hasSetup=setup!=nullptr;if(setup)c.setup=*setup;c.verified=verified;c.kind=kind;c.focal=focal;c.tint=tint;c.au=au;c.av=av;c.alpha=alpha;
 c.low=0;c.high=float(renderHeight());bool finite=true;for(unsigned i=0;i<3;++i)finite=finite&&std::isfinite(v[i].y);
 if(finite){if(setup){c.low=setup->y0-1.f;c.high=setup->y1+1.f;}else{float y[3];for(unsigned i=0;i<3;++i)y[i]=kind==2?(120-focal*v[i].y/v[i].reciprocalZ)/scale_:v[i].y/(kind==1?1:scale_);c.low=std::min({y[0],y[1],y[2]})-1.f;c.high=std::max({y[0],y[1],y[2]})+1.f;}}
 commands_.push_back(c);
}
void Renderer::replayFrom(const Renderer& source,unsigned lo,unsigned hi){replayCommands(source.commands_,lo,hi);}
void Renderer::replayCommands(const std::vector<Command>& commands,unsigned lo,unsigned hi){
 deferredReplaying_=true;replayLo_=lo;replayHi_=hi;for(const auto& c:commands){if(c.high<float(lo)||c.low>=float(hi))continue;
 if(c.kind==1){Vec3 v[3];Vec2 uv[3];for(unsigned i=0;i<3;++i){v[i]={c.v[i].x,c.v[i].y,c.v[i].reciprocalZ};uv[i]={c.v[i].u,c.v[i].v};}rasterTextureProjected(v,uv,c.tint,*c.texture,c.au,c.av,c.alpha,c.hasSetup?&c.setup:nullptr);}
 else if(c.kind==2){Vec3 v[3];for(unsigned i=0;i<3;++i)v[i]={c.v[i].x,c.v[i].y,c.v[i].reciprocalZ};raster(v[0],v[1],v[2],c.tint,c.focal);}
 else screenTriangleChecked(c.v,c.texture,c.state,c.verified,c.hasSetup?&c.setup:nullptr);
 }
 deferredReplaying_=false;replayLo_=0;replayHi_=10000;
}
void Renderer::mergeRows(const Renderer& source,unsigned lo,unsigned hi){
 const size_t first=size_t(lo)*stride(),last=size_t(hi)*stride();std::copy(source.color_.begin()+first,source.color_.begin()+last,color_.begin()+first);std::copy(source.depth_.begin()+first,source.depth_.begin()+last,depth_.begin()+first);expandedValid_=false;invalidateOcclusion();
}
}

namespace soft {void Renderer::resetReplay(const Renderer& initial,unsigned lo,unsigned hi){const size_t first=size_t(lo)*stride(),last=size_t(hi)*stride();std::copy(initial.color_.begin()+first,initial.color_.begin()+last,color_.begin()+first);std::copy(initial.depth_.begin()+first,initial.depth_.begin()+last,depth_.begin()+first);stats_=initial.stats_;expandedValid_=false;invalidateOcclusion();occlusionBuilds_=0;}}
