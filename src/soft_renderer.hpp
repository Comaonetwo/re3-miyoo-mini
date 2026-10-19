#pragma once
#include <cstdint>
#include "pixel_counters.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <utility>
#include <array>
#include <memory>

namespace soft {
struct Vec3 { float x, y, z; };
struct Vec2 {float u,v;};
struct Texture {uint16_t width,height;std::vector<uint16_t> pixels;std::vector<uint8_t> alpha;
 // Set only by the raster decoder after checking all alpha bytes. Raster
 // unlock invalidates/rebuilds the decoded copy, including this certificate.
 bool decodedOpaque,decodedUniform;
 bool decodedInkBounds=false;int inkX0=0,inkY0=0,inkX1=-1,inkY1=-1;
 Texture():width(0),height(0),decodedOpaque(false),decodedUniform(false){}
 Texture(uint16_t w,uint16_t h,std::vector<uint16_t> p,std::vector<uint8_t> a):width(w),height(h),pixels(std::move(p)),alpha(std::move(a)),decodedOpaque(false),decodedUniform(false){}
};
// Position-only setup, consumed immediately through private device bridges.
// No depth/target mutations may occur between preparation and consumption.
struct WorldTriangleSetup {Vec3 points[3];int x0,x1,y0,y1;float area,inv,row[3],dx[3],dy[3],z,zx,zy;bool swapped,depthChecked,tl[3];};
struct Triangle { Vec3 v[3]; uint16_t color; };
struct ScreenVertex {float x,y,reciprocalZ,u,v;float r,g,b,a,depthZ;};
struct ShadedVertex {Vec3 position;Vec2 uv;float r,g,b,a;};
struct ScreenState {bool zTest=false,zWrite=false;unsigned blend=0,alphaRef=0,cull=1;uint8_t addressU=3,addressV=3;bool perspectiveColor=false,screenDepth=false,linear=false;};
struct Camera {
    Vec3 position{0, 2.0f, -8};
    float yaw = 0, nearZ = 0.1f, farZ = 80, focal = 180;
};
inline unsigned frustumCode(Vec3 p,const Camera& c){float sx=160/c.focal,sy=120/c.focal;
    return (p.z<c.nearZ?1u:0u)|(p.z>c.farZ?2u:0u)|(p.x < -sx*p.z?4u:0u)|(p.x > sx*p.z?8u:0u)|(p.y < -sy*p.z?16u:0u)|(p.y > sy*p.z?32u:0u);
}
// Core writes and packed-world writes also gate optional live calibration.
// Keep them (and triangle/error/visibility counters) enabled in every build.
struct Stats {
    PixelDiagnosticCounter tileScanPixels=0;
    PixelDiagnosticCounter rainVectorPixels=0;
    PixelDiagnosticCounter rowExpandPixels=0;
    PixelDiagnosticCounter skyVectorPixels=0;
    PixelDiagnosticCounter skyColorPixels=0;
    PixelDiagnosticCounter flatSpanPixels=0;
    PixelDiagnosticCounter filterRowPixels=0;
    PixelDiagnosticCounter alphaSpanPixels=0;
    uint64_t alphaSpanTriangles=0;
    uint64_t lazyQueries=0;
    uint64_t lazyHidden=0;
    uint64_t lazyTiles=0;
    uint64_t lazySamples=0;
    uint64_t smallTileTriangles=0;
    uint64_t hiddenSampleTriangles=0;
    uint64_t hiddenSampleChecks=0;
    uint64_t submitted=0;
    uint64_t rejected=0;
    uint64_t rasterized=0;
    uint64_t pixels=0;
    uint64_t hudFallbacks=0;
    PixelDiagnosticCounter spanSkippedPixels=0;
    PixelDiagnosticCounter perspectiveSaved=0;
    PixelDiagnosticCounter tintPixels=0;
    uint64_t occludedTriangles=0;
    uint64_t occlusionChecks=0;
    uint64_t occlusionBuilds=0;
    PixelDiagnosticCounter filteredAlphaOnly=0;
    PixelDiagnosticCounter filteredFlat=0;
    uint64_t attributeSavedAdds=0;
    PixelDiagnosticCounter preparedWorldPixels=0;
    uint64_t modulationTriangles=0;
    uint64_t packedWorldPixels=0;
    uint64_t hiddenDepthRows=0;
    PixelDiagnosticCounter hiddenDepthSamples=0;
    PixelDiagnosticCounter coverageRunPixels=0;
    PixelDiagnosticCounter texelRunPixels=0;
    uint64_t texelRunBlocks=0;
    PixelDiagnosticCounter packedReciprocals=0;
    PixelDiagnosticCounter packetPixels=0;
    PixelDiagnosticCounter packetFallbacks=0;
    PixelDiagnosticCounter texelOpaquePixels=0;
    uint64_t hiddenDepthBlocks=0;
    PixelDiagnosticCounter hiddenBlockSamples=0;
    uint64_t certifiedScreenTriangles=0;
    PixelDiagnosticCounter blackAddPixels=0;
    PixelDiagnosticCounter additiveCachedPixels=0;
    uint64_t additiveCacheBuilds=0;
    PixelDiagnosticCounter vectorFilteredPixels=0;
    PixelDiagnosticCounter vectorBlendPixels=0;
    PixelDiagnosticCounter whiteTexturePixels=0;
    PixelDiagnosticCounter fastFilterCoordinates=0;
    PixelDiagnosticCounter vertexAlphaRejected=0;
    PixelDiagnosticCounter finalAlphaOnly=0;
    PixelDiagnosticCounter preparedFilterPixels=0;
    PixelDiagnosticCounter packedAlphaPixels=0;
    PixelDiagnosticCounter preparedNearestPixels=0;
    PixelDiagnosticCounter spriteEffectPixels=0;
    PixelDiagnosticCounter additiveWorldPixels=0;
    uint64_t skyPasses=0;
    PixelDiagnosticCounter skyCompositePixels=0;
    PixelDiagnosticCounter blackFadePixels=0;
    PixelDiagnosticCounter particleEffectPixels=0;
    uint64_t rainPasses=0;
    PixelDiagnosticCounter rainCompositePixels=0;
    uint64_t rainFallbacks=0;
    PixelDiagnosticCounter flatAlphaPixels=0;
    uint64_t flatAlphaBuilds=0;
    PixelDiagnosticCounter transparentWorldPixels=0;
    PixelDiagnosticCounter setupSavedAttributes=0;
    PixelDiagnosticCounter verifiedScreenVertices=0;
    PixelDiagnosticCounter nearestCoveragePixels=0;
    PixelDiagnosticCounter nearestOpaquePixels=0;
    PixelDiagnosticCounter constantAlphaPixels=0;
    PixelDiagnosticCounter whiteBlendPixels=0;
    uint64_t earlyDepthTriangles=0;
    PixelDiagnosticCounter additiveBoundsPixels=0;
    uint64_t leanPackedTriangles=0;
    uint64_t alphaRejectedTriangles=0;
    uint64_t hudCompositeSamples=0,hudSparseResolves=0;
    uint64_t rgbRegisterTriangles=0,rgbRegisterSteps=0;
};
// Keep the tiny RGB565 packer visible to pixel loops in every translation unit.
inline uint16_t rgb(unsigned r, unsigned g, unsigned b) {
    return uint16_t(((r&255)>>3)<<11 | ((g&255)>>2)<<5 | ((b&255)>>3));
}
Vec3 view(Vec3 p, const Camera& camera);
class Renderer {
    friend class SoftwareDevice;
public:
    static constexpr int width=320, height=240;
    Renderer();
    // The main thread owns configuration and submission. Texture storage must
    // remain immutable until flushDeferred(); SoftwareDevice supplies barriers.
    // A live queue/worker cannot be duplicated with an implicit renderer copy.
    Renderer(const Renderer&)=delete;
    Renderer& operator=(const Renderer&)=delete;
    void beginDeferred(unsigned mode,unsigned limit=512);
    void endDeferred();
    void flushDeferred();
    size_t deferredBytes()const;
    uint64_t deferredCommands()const{return deferredCommands_;}
    uint64_t deferredBatches()const{return deferredBatches_;}
    uint64_t deferredFallbacks()const{return deferredFallbacks_;}

    void clear(uint16_t color);
    void clearBuffers(uint16_t color,bool image,bool depth);
    void screenTriangle(const ScreenVertex* vertices,const Texture* texture,const ScreenState& state);
    // Perspective-correct color/alpha + UV; clipped on all six frustum planes.
    void shadedTriangle(const ShadedVertex* vertices,const Texture* texture,ScreenState state,const Camera& camera);
    // Camera-space vertices; all six frustum planes are clipped before projection.
    void triangle(const Vec3* v, uint16_t color, const Camera& camera);
    // Perspective-correct nearest texel sampling with binary alpha test.
    void texturedTriangle(const Vec3* v,const Vec2* uv,uint16_t tint,const Texture& texture,
                          uint8_t addressU,uint8_t addressV,const Camera& camera,unsigned alphaRef=128);
    // Logical 320x240 positions, reciprocal depth and raw UV already prepared.
    // Caller classifies all six camera planes; this entry preserves FLAT tint.
    void texturedProjectedTriangle(const ScreenVertex* vertices,uint16_t tint,const Texture& texture,uint8_t addressU,uint8_t addressV,unsigned alphaRef=128);
    void worldTriangle(const Triangle& t, const Camera& camera);
    bool writePPM(const std::string& path) const;
    const std::vector<uint16_t>& pixels() const;
    void renderScale(unsigned scale);
    void resolveFullResolution();
    void resolveNativeUi();
    void beginUi(bool native);
    void beginBackground(unsigned width);
    void beginHud(unsigned width);
    bool beginRain(unsigned width=120);void endRain();void cancelRain();bool rainActive()const{return rainActive_;}
    bool beginSky(unsigned width);
    void endSky();
    void cancelSky();
    bool skyActive()const{return skyActive_;}
    void beginRadar(float left,float top,float right,float bottom);
    void endRadar(){radarClip_=false;}
    void rgbRegisterEnabled(bool enabled){rgbRegister_=enabled;}
    void sparseHudEnabled(bool enabled){sparseHud_=enabled;}
    void alphaRejectEnabled(bool enabled){alphaReject_=enabled;}
    void alphaSpansEnabled(bool enabled){alphaSpans_=enabled;}
    void filterStages(unsigned mask){filterStages_=mask&31;}
    void vectorFilterEnabled(bool enabled){vectorFilter_=enabled;}
    void additiveBoundsEnabled(bool enabled){additiveBounds_=enabled;}
    void smallDepthEnabled(bool enabled){smallDepth_=enabled;}
    void lazyDepthEnabled(bool enabled){if(lazyDepth_!=enabled){lazyDepth_=enabled;invalidateLazyDepth();}}
    void earlyDepthEnabled(bool enabled){earlyDepth_=enabled;}
    void whiteBlendEnabled(bool enabled){whiteBlend_=enabled;}
    void constantAlphaEnabled(bool enabled){constantAlpha_=enabled;}
    void nearestOpacityEnabled(bool enabled){nearestOpacity_=enabled;}
    void nearestCoverageEnabled(bool enabled){nearestCoverage_=enabled;}
    void leanSetupEnabled(bool enabled){leanSetup_=enabled;}
    void transparentWorldEnabled(bool enabled){transparentWorld_=enabled;}
    void flatAlphaEnabled(bool enabled){flatAlpha_=enabled;}
    void zeroMasksEnabled(bool enabled){zeroMasks_=enabled;}
    void particleSpansEnabled(bool enabled){particleSpans_=enabled;}
    void effectSpansEnabled(bool enabled){effectSpans_=enabled;}
    void additiveCacheEnabled(bool enabled){additiveCache_=enabled;}
    void blackAddEnabled(bool enabled){blackAdd_=enabled;}
    void screenCertificatesEnabled(bool enabled){screenCertificates_=enabled;}
    void opaqueOverlayEnabled(bool enabled){opaqueOverlay_=enabled;}
    void packetSamplerEnabled(bool enabled){packetSampler_=enabled;}
    void texelRunsEnabled(bool enabled){texelRuns_=enabled;}
    void edgeRunsEnabled(bool enabled){edgeRuns_=enabled;}
    void opaqueTexturesEnabled(bool enabled){opaqueTextures_=enabled;}
    void depthRowCullingEnabled(bool enabled){depthRows_=enabled;}
    void depthBlocksEnabled(bool enabled){depthBlocks_=enabled;}
    void gradientDepthRowsEnabled(bool enabled){gradientDepthRows_=enabled;}
    void batchGeneralCountersEnabled(bool enabled){batchGeneral_=enabled;}
    void batchCountersEnabled(bool enabled){batchCounters_=enabled;}
    void flatOcclusionEnabled(bool enabled){flatOcclusion_=enabled;invalidateOcclusion();}
    void occlusionEnabled(bool enabled){occlusionEnabled_=enabled;invalidateOcclusion();}
    void tileDepthEnabled(bool enabled){tileDepth_=enabled;}
    void worldChannelsEnabled(bool enabled){worldChannels_=enabled;}
    bool worldChannelsEnabled()const{return worldChannels_;}
    void tileScanEnabled(bool enabled){tileScan_=enabled;}
    bool tileScanEnabled()const{return tileScan_;}
    void rainCompositeEnabled(bool enabled){rainComposite_=enabled;}
    bool rainCompositeEnabled()const{return rainComposite_;}
    void rowExpansionEnabled(bool enabled){rowExpansion_=enabled;}
    bool rowExpansionEnabled()const{return rowExpansion_;}
    void skyCompositeEnabled(bool enabled){skyComposite_=enabled;}
    bool skyCompositeEnabled()const{return skyComposite_;}
    void skyChannelsEnabled(bool enabled){skyChannels_=enabled;}
    bool skyChannelsEnabled()const{return skyChannels_;}
    void flatSpansEnabled(bool enabled){flatSpans_=enabled;}
    bool flatSpansEnabled()const{return flatSpans_;}
    void exactAdvanceEnabled(bool enabled){exactAdvance_=enabled;}
    void packedWorldEnabled(bool enabled){packedWorld_=enabled;}
    void modulationTablesEnabled(bool enabled){modulationTables_=enabled;}
    void preparedWorldEnabled(bool enabled){preparedWorld_=enabled;}
    void clearUiDepth(){flushDeferred();invalidateOcclusion();std::fill(depth_.begin(),depth_.end(),0.f);}
    unsigned outputWidth() const {return rainActive_?120:nativeUi_?640:320;}
    unsigned outputHeight() const {return nativeUi_?480:240;}
    void renderWorldWidth(unsigned width);
    unsigned worldWidth()const{return unsigned(320.f/requestedScale_+.5f);}
    unsigned renderScale() const {return scale_==1?1:scale_==2?2:scale_==4?4:3;}
    const std::vector<float>& depth() const { const_cast<Renderer*>(this)->flushDeferred();return depth_; }
    const Stats& stats() const { return stats_; }
    size_t bufferBytes() const { return color_.size()*2 + depth_.size()*sizeof(float) + expanded_.size()*2 + uiColor_.size()*2 + uiDepth_.size()*sizeof(float) + hudLayer_.size()*sizeof(uint32_t) + skyBase_.size()*2 + skyDeltas_.size()*4; }
private:
    struct Command {ScreenVertex v[3];const Texture* texture;ScreenState state;WorldTriangleSetup setup;bool hasSetup,verified;unsigned kind;float focal;uint16_t tint;uint8_t au,av;unsigned alpha;float low,high;};
    struct DeferredPool;
    std::shared_ptr<DeferredPool> deferredPool_;
    bool deferredActive_=false,deferredReplaying_=false;
    unsigned deferredMode_=0,deferredLimit_=512;
    uint64_t deferredCommands_=0,deferredBatches_=0,deferredFallbacks_=0;
    bool deferCommand(const ScreenVertex* input,const Texture* texture,const ScreenState& state,bool verified,const WorldTriangleSetup* setup,unsigned kind,uint16_t tint=0,uint8_t au=0,uint8_t av=0,unsigned alpha=0,float focal=0);
    void syncRaster(const Renderer& initial);
    void replayCommands(const std::vector<Command>& commands,unsigned lo,unsigned hi);
    void dispatchPipeline();
    void finishPipeline();
    void addRasterStats(const Stats& value);
    std::vector<Command> commands_;
    void resetReplay(const Renderer& initial,unsigned lo,unsigned hi);
    void replayFrom(const Renderer& source,unsigned lo,unsigned hi);
    void mergeRows(const Renderer& source,unsigned lo,unsigned hi);int replayLo_=0,replayHi_=10000;
    void recordCommand(const ScreenVertex* input,const Texture* texture,const ScreenState& state,bool verified,const WorldTriangleSetup* setup,unsigned kind,uint16_t tint=0,uint8_t au=0,uint8_t av=0,unsigned alpha=0,float focal=0);
    void screenTriangleChecked(const ScreenVertex* vertices,const Texture* texture,const ScreenState& state,bool verified,const WorldTriangleSetup* setup=nullptr);
    // Bounded cache for exact RGB565 modulation in the opaque world pass.
    // Small triangles keep arithmetic; larger ones reuse 128 component entries.
    struct TintTable {bool valid=false;uint16_t tint=0;uint16_t red[32],green[64],blue[32];};
    TintTable tintTables_[16];
    // General renderer uses full 8-bit vertex colours, unlike the RGB565 tint
    // in the legacy textured pass. Separate tables preserve its rounding.
    struct ColorTable {bool valid=false;uint32_t key=0;uint16_t red[32],green[64],blue[32];};
    ColorTable colorTables_[16];
    bool occlusionEnabled_=true,occlusionValid_=false,tileDepth_=false;
    unsigned occlusionCalls_=0,occlusionBuilds_=0;
    float occlusionDepth_[40*30]; // conservative minima; monotone lazy refresh is safe
    bool alphaReject_=true;
    bool lazyDepth_=false;uint32_t lazyGeneration_=1;unsigned lazyCalls_=0,lazyFrameProbes_=0,lazyFrameHidden_=0,lazySampleMask_=15,lazyBudgetFrame_=0;bool lazyFull_=false,lazyThisFrame_=false;
    std::array<uint32_t,40*30> lazyStamp_{};
    void finishLazyFrame(){if(lazyFrameProbes_>=4){bool dense=lazyFrameHidden_*8>=lazyFrameProbes_;lazyFull_=lazyFrameHidden_>=4&&dense;lazySampleMask_=dense?15:63;}lazyFrameProbes_=lazyFrameHidden_=0;lazyThisFrame_=lazyDepth_&&(lazyFull_||((++lazyBudgetFrame_&3)==0));}
    void invalidateLazyDepth(){if(++lazyGeneration_==0){lazyStamp_.fill(0);lazyGeneration_=1;}lazyCalls_=0;}
    void invalidateOcclusion(){occlusionValid_=false;occlusionCalls_=0;invalidateLazyDepth();}
    bool sampleLazyDepth(){unsigned calls=++lazyCalls_;if(calls==512||calls==2048){if(++lazyGeneration_==0){lazyStamp_.fill(0);lazyGeneration_=1;}}return lazyFull_||(calls&lazySampleMask_)==0;}
    bool lazyOccluded(int x0,int y0,int x1,int y1,float row,float dx,float dy);
    // Queued 3D commands only increase depth. The last committed depth is
    // immutable during a batch and remains a conservative culling bound.
    bool canQueryWorldVisibility()const{return smallDepth_&&!nativeUi_&&!hudOverlay_&&!rainActive_&&!skyActive_;}
    int queryWorldVisibility(const Vec3* screen,WorldTriangleSetup& setup,bool wide);
    bool occludedSmall(int x0,int y0,int x1,int y1,float row,float dx,float dy);
    bool occluded(int x0,int y0,int x1,int y1,float row,float dx,float dy);
    bool hudOverlay_=false;
    bool radarClip_=false;float radarCx_=0,radarCy_=0,radarRx_=0,radarRy_=0;
    void radarSpan(int y,int x0,int& first,int& last) const;
    bool rgbRegister_=true;
    bool sparseHud_=true;
    std::array<uint16_t,180> hudFirst_{},hudLast_{};
    void clearHudLayer();
    void markHudBounds(int x0,int y0,int x1,int y1);
    std::vector<uint32_t> hudLayer_; // 240x180 premultiplied RGBA, stride 320
    void flushHudOverlay();
    void overlayPixel(size_t index,unsigned r,unsigned g,unsigned b,unsigned alpha,unsigned blend);
    bool nativeUi_=false;
    std::vector<uint16_t> uiColor_;
    std::vector<float> uiDepth_;
    void restoreWorld();
    unsigned stride() const {return rainActive_?120:nativeUi_?640:320;}
    float scale_=1,requestedScale_=1;
    bool rainActive_=false;unsigned rainWorldWidth_=0;float rainScale_=1;
    std::vector<uint16_t> rainColor_;std::vector<float> rainDepth_,rainMinimum_;
    std::vector<std::array<uint16_t,3>> rainRGB_;
    bool skyActive_=false;unsigned skyWidth_=0,skyWorldWidth_=0;float skyScale_=1;
    std::vector<uint32_t> skyDeltas_;
    std::vector<uint16_t> skyBase_; // one reusable 320x240 colour plane, no depth copy
    unsigned renderWidth() const {return rainActive_?120:skyActive_?skyWidth_:nativeUi_?640:unsigned(320.f/scale_+.5f);}
    unsigned renderHeight() const {return renderWidth()*3/4;}
    mutable bool expandedValid_=false;
    mutable std::vector<uint16_t> expanded_;
    std::vector<uint16_t> color_;
    std::vector<float> depth_; // reciprocal camera Z, larger is closer
    // Exact source-channel contributions; 16 bounded keys, no texture pointers.
    struct AddContribution {uint64_t key=0;bool valid=false;uint8_t red[32],green[64],blue[32];};
    AddContribution addContributions_[16];
    unsigned filterStages_=19;bool vectorFilter_=true,alphaSpans_=false;
    bool smallDepth_=false;
    bool whiteBlend_=false,constantAlpha_=true,additiveBounds_=false,earlyDepth_=false,nearestOpacity_=true,nearestCoverage_=true,leanSetup_=true,transparentWorld_=true,flatAlpha_=false;
    struct FlatAlphaTable {bool valid=false;uint32_t key=0;uint8_t red[32*32],green[64*64],blue[32*32];};
    std::vector<FlatAlphaTable> flatAlphaTables_;
    bool effectSpans_=true,particleSpans_=true,zeroMasks_=true;
    bool blackAdd_=false,additiveCache_=false;
    bool screenCertificates_=true,opaqueOverlay_=true;
    bool worldChannels_=true;
    bool flatSpans_=true;
    bool skyChannels_=true;
    bool skyComposite_=true;
    bool rowExpansion_=true;
    bool rainComposite_=true;
    bool tileScan_=true;
    bool exactAdvance_=false,preparedWorld_=true,modulationTables_=true,packedWorld_=true,flatOcclusion_=false,batchCounters_=true,batchGeneral_=false,depthRows_=true,depthBlocks_=false,gradientDepthRows_=false,opaqueTextures_=true,edgeRuns_=true,texelRuns_=false,packetSampler_=false;
    Stats stats_;
    void texturedProjectedPrepared(const ScreenVertex* vertices,uint16_t tint,const Texture& texture,uint8_t addressU,uint8_t addressV,unsigned alphaRef,const WorldTriangleSetup* setup=nullptr);
    void rasterTextureProjected(const Vec3* projected,const Vec2* uvOverZ,uint16_t tint,const Texture& texture,uint8_t addressU,uint8_t addressV,unsigned alphaRef,const WorldTriangleSetup* setup=nullptr);
    void rasterTexture(const Vec3* v,const Vec2* uv,uint16_t tint,const Texture& texture,
                       uint8_t addressU,uint8_t addressV,float focal,unsigned alphaRef);
    void raster(Vec3 a, Vec3 b, Vec3 c, uint16_t color, float focal);
};
std::vector<Triangle> makeScene();
void renderScene(Renderer& r, const std::vector<Triangle>& scene, const Camera& camera);
}
