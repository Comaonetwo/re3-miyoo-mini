#pragma once
#include "soft_renderer.hpp"
namespace soft {
struct SceneGroup {uint32_t begin,count;Vec3 center;float radius;};
struct TriangleTexture {Vec2 uv[3];uint16_t texture;uint8_t addressU,addressV;};
struct TriangleSurface {uint8_t rgba[3][4],material[4];Vec3 normals[3];float ambient,diffuse;uint8_t flags;};
struct AssetScene {
    Camera spawn;
    std::vector<Triangle> triangles;
    std::vector<SceneGroup> groups;
    std::vector<Texture> textures;
    std::vector<TriangleTexture> materials;
    std::vector<TriangleSurface> surfaces;
    void discardConvertedData();
    static AssetScene load(const std::string& path);
    void draw(Renderer& renderer,const Camera& camera,bool textured=true) const;
    void render(Renderer& renderer,const Camera& camera,bool textured=true) const;
};
}
