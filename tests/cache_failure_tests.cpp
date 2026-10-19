#include "software_device.hpp"
#include "model_clusters.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <new>
// Fail only inside the operation under test, after all game objects exist.
static int failAfter = -1;
void* operator new(std::size_t size) {
    if (failAfter == 0) { failAfter = -1; throw std::bad_alloc(); }
    if (failAfter > 0) --failAfter;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
static rw::Geometry* mesh() {
    auto g = rw::Geometry::create(3, 1, rw::Geometry::POSITIONS);
    auto m = rw::Material::create(); g->matList.appendMaterial(m); m->destroy();
    g->morphTargets[0].vertices[0] = {-1, -1, 5};
    g->morphTargets[0].vertices[1] = {1, -1, 5};
    g->morphTargets[0].vertices[2] = {0, 1, 5};
    g->triangles[0] = {{0, 1, 2}, 0}; return g;
}
int main() {
    soft::Renderer r; soft::SoftwareDevice d(r);
    for (int failure = 0; failure < 2; ++failure) {
        auto g = mesh(); assert(soft::SoftwareDevice::opaqueGeometry(g));
        g->lock(rw::Geometry::LOCKVERTICES);
        g->morphTargets[0].vertices[0].x = -3; g->unlock();
        failAfter = failure;
        assert(!soft::SoftwareDevice::opaqueGeometry(g));
        assert(failAfter == -1 && d.clusterCacheBytes() == 0);
        assert(soft::SoftwareDevice::opaqueGeometry(g));
        soft::ModelBounds b;
        assert(soft::SoftwareDevice::geometryBounds(g, b) && b.minimum.x == -3);
        g->destroy(); assert(d.clusterCacheBytes() == 0);
    }
    auto g = mesh(); soft::ModelBounds b;
    assert(soft::SoftwareDevice::geometryBounds(g, b) && b.minimum.x == -1);
    g->lock(rw::Geometry::LOCKVERTICES);
    g->morphTargets[0].vertices[0].x = -4; g->unlock();
    assert(soft::SoftwareDevice::geometryBounds(g, b) && b.minimum.x == -4);
    g->destroy(); assert(d.clusterCacheBytes() == 0);
    assert(!soft::SoftwareDevice::geometryBounds(nullptr, b));
    // Both allocations of the cross-frame colour cache must fall back safely.
    for (int failure=0;failure<2;++failure) {
        auto colored=rw::Geometry::create(3,1,rw::Geometry::POSITIONS|rw::Geometry::PRELIT|rw::Geometry::LIGHT|rw::Geometry::MODULATE);
        auto material=rw::Material::create();colored->matList.appendMaterial(material);material->destroy();
        rw::V3d positions[]={{-1,-1,5},{1,-1,5},{0,1,5}};
        for(unsigned i=0;i<3;++i){colored->morphTargets[0].vertices[i]=positions[i];colored->colors[i]={120,150,190,255};}
        colored->triangles[0]={{0,1,2},0};auto atomic=rw::Atomic::create();atomic->setGeometry(colored,0);
        soft::Camera camera;d.begin(camera);rw::SetRenderState(rw::CULLMODE,rw::CULLNONE);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);
        d.vertexColorCache(true);r.clear(12345);atomic->render(); // observe recipe, warm scratch buffers
        failAfter=failure;r.clear(12345);atomic->render();
        assert(failAfter==-1&&d.vertexColorCacheBytes()==0);
        r.clear(12345);atomic->render();r.clear(12345);atomic->render();
        assert(d.vertexColorCacheBytes()>0);d.end();atomic->destroy();colored->destroy();
        assert(d.vertexColorCacheBytes()==0&&d.clusterCacheBytes()==0);
    }
    puts("PASS cache allocation/resize failures recover, dirty bounds refresh, ownership released");
}
