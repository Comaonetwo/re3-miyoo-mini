#pragma once
#ifndef RWCORE_H
#include <rw.h>
#endif
#include "rw_soft_vertices.hpp"
#include "soft_renderer.hpp"
#include "checkpoint_run.hpp"
#include <memory>
namespace soft {
// One real librw Engine/Camera/Device per process. Installs CPU callbacks before start.
class SoftwareDevice {
    struct Impl;std::unique_ptr<Impl> impl_;
    static void drawPreparedScreen(Renderer& renderer,const ScreenVertex* vertices,const Texture* texture,const ScreenState& state,const WorldTriangleSetup* setup=nullptr);
    static void drawPreparedFlat(Renderer& renderer,const ScreenVertex* vertices,uint16_t tint,const Texture& texture,uint8_t addressU,uint8_t addressV,unsigned alphaRef,const WorldTriangleSetup* setup=nullptr);
static bool canQueryWorldVisibility(Renderer& renderer);
    static int queryWorldVisibility(Renderer& renderer,const Vec3* screen,WorldTriangleSetup& setup,bool wide);
public:
    void earlyWideVisibility(bool enabled);void earlyVisibility(bool enabled);uint64_t earlyVisibilityQueries()const;uint64_t earlyVisibilityHidden()const;
    explicit SoftwareDevice(Renderer& renderer,bool (*attachPlugins)()=nullptr);
    ~SoftwareDevice();
    SoftwareDevice(const SoftwareDevice&)=delete;
    SoftwareDevice& operator=(const SoftwareDevice&)=delete;
    void begin(const Camera& camera);
    void end();
    void checkpoint(const CheckpointRun& run,const Camera& camera,bool glow=false);
    void checkpointGlow(const CheckpointRun& run);
    void hud(const std::vector<uint16_t>& frame);
    uint64_t unsupported() const;
    uint64_t im2dTriangles() const;
    uint64_t im3dTriangles() const;
    rw::Camera* camera() const;
    void worldTextures(bool enabled);
    rw::World* world() const;
    void farSurfaceLighting(bool);bool farSurfaceLighting()const;
    void farNormalLighting(bool);bool farNormalLighting()const;uint64_t farNormalVertices()const;
    void lighting(Vec3 ambient,Vec3 sun,Vec3 direction);
    void highQuality(bool enabled);
    void flatFading(bool enabled);uint64_t flatFadingTriangles()const;
    void distanceShading(bool enabled);uint64_t distanceShadingTriangles()const;
    void ambientShortcut(bool enabled);uint64_t ambientVerticesSkipped()const;
    void whitePrelight(bool enabled);uint64_t whitePrelightVertices()const;
    void vertexColorCache(bool enabled);
    uint64_t cachedVertexColors()const;uint64_t preparedVertexColors()const;
    size_t vertexColorCacheBytes()const;
    uint64_t atomicCalls() const;
    uint64_t matFxCalls() const;
    uint64_t skinCalls() const;
    uint64_t skinnedVertices() const;
    uint64_t shadedTriangles() const;
    uint64_t culledTriangles() const;
    uint64_t backfaceTriangles() const;
    uint64_t reusedColors() const;uint64_t reusedGroupedColors()const;
    void groupedColors(bool enabled);
    void preparedScreenValidation(bool enabled);
    void preparedAtomics(bool enabled);
    uint64_t preparedMaterials() const;
    uint64_t reusedMaterials() const;
    uint64_t reusedFlatProjections() const;
    void traceTextures(bool enabled);void normalCache(bool enabled);uint64_t cachedNormalsReused()const;uint64_t cachedNormalsPrepared()const;size_t normalCacheBytes()const;
    void directAtomic(bool enabled);uint64_t directAtomicVertices()const;
    void atomicTransfer(bool enabled);void textureAlpha(bool enabled);
    uint64_t transferredAtomicVertices()const;uint64_t textureAlphaTriangles()const;
    void effectFiltering(bool enabled);void bakedLighting(bool enabled);
    static void printTextureInfo(rw::Raster*,const char* label);
    void hierarchicalBounds(bool enabled);
    uint64_t modelBoundsTests()const;uint64_t modelBoundsCulls()const;uint64_t fullyInsideModels()const;uint64_t savedPlaneTests()const;
    static bool geometryBounds(rw::Geometry*,struct ModelBounds&);
    static bool opaqueGeometry(rw::Geometry*,bool ignoreMaterialAlpha=false);
    static bool worldGeometrySuitable(rw::Geometry*);
    static rw::Geometry* simplifiedWorld(rw::Geometry*,const Vec3* protectedCamera=nullptr,float protectedRadius=0,bool intermediate=false);
    static bool simplifiedWorldRejected(rw::Geometry*,bool intermediate);
    static float simplifiedError(rw::Geometry*);
    static bool simplifiedSafeAt(rw::Geometry*,Vec3 localCamera,float minimumDistance);
    void simplificationBudget(unsigned);uint64_t simplifiedWorldBuilds()const;size_t simplifiedWorldBytes()const;double simplificationPeakMs()const;
    void worldLodSelector(rw::Geometry* (*selector)(rw::Atomic*));
    uint64_t worldLodAtomics()const;uint64_t worldLodInputSaved()const;
    void modelClusters(bool enabled);
    void fastClusterBounds(bool enabled);
    uint64_t culledClusters() const;
    uint64_t clusterTests() const;
    uint64_t clusterTriangles() const;
    uint64_t skippedVertices() const;
    uint64_t clusterRebuilds() const;
    size_t clusterCacheBytes() const;
    uint64_t dynamicGeometryFallbacks() const;
};
}
