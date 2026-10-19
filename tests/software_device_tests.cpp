#include "software_device.hpp"
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <limits>
using namespace soft;
static void check(bool result,const char* message){if(!result)throw std::runtime_error(message);}
static size_t at(int x,int y){return size_t(y)*320+x;}
static void quad(float x,float y,float width,float height,uint8_t red,uint8_t green,uint8_t blue,uint8_t alpha){
    rw::cpu::Im2DVertex v[4];for(int i=0;i<4;++i){v[i].x=x+(i&1)*width;v[i].y=y+(i>>1)*height;v[i].u=float(i&1);v[i].v=float(i>>1);v[i].setColor(red,green,blue,alpha);}
    uint16_t indices[6]={0,1,2,2,1,3};rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,v,4,indices,6);
}
int main(){try{
    Renderer renderer;{
        SoftwareDevice device(renderer);Camera camera;camera.position={0,0,0};device.begin(camera);
        check(rw::Engine::getNumVideoModes()==1&&rw::Engine::getNumSubSystems()==1,"device enumeration");
        rw::VideoMode mode;check(rw::Engine::getVideoModeInfo(&mode,0)&&mode.width==320&&mode.height==240,"video mode");
        rw::RGBA blue{0,0,255,255};device.camera()->clear(&blue,rw::Camera::CLEARIMAGE|rw::Camera::CLEARZ);
        rw::SetRenderState(rw::ZTESTENABLE,0);rw::SetRenderState(rw::ZWRITEENABLE,0);rw::SetRenderState(rw::VERTEXALPHA,1);
        rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAALWAYS);quad(10,10,20,20,255,0,0,128);
        check(renderer.pixels()[at(15,15)]==rgb(128,0,127),"source alpha blend");
        check(renderer.pixels()[at(20,19)]==renderer.pixels()[at(19,20)],"shared edge is blended once");
        check(renderer.pixels()[at(0,0)]==rgb(0,0,255),"outside triangle preserved");
        rw::SetRenderState(rw::DESTBLEND,rw::BLENDONE);quad(40,10,10,10,255,0,0,128);check(renderer.pixels()[at(45,15)]==rgb(128,0,255),"additive blend");
        rw::SetRenderState(rw::VERTEXALPHA,0);rw::SetRenderState(rw::DESTBLEND,rw::BLENDINVSRCALPHA);
        auto image=rw::Image::create(2,2,32);image->allocate();uint8_t rgba[16]={255,255,255,255,255,0,0,255,0,255,0,255,0,0,0,0};
        for(int y=0;y<2;++y)std::memcpy(image->pixels+y*image->stride,rgba+y*8,8);
        auto raster=rw::Raster::createFromImage(image);check(raster!=nullptr,"raster from image");auto exported=raster->toImage();
        check(exported&&std::memcmp(exported->pixels,rgba,16)==0,"raster/image roundtrip");exported->destroy();image->destroy();
        rw::SetRenderStatePtr(rw::TEXTURERASTER,raster);rw::SetRenderState(rw::ALPHATESTFUNC,rw::ALPHAGREATEREQUAL);rw::SetRenderState(rw::ALPHATESTREF,128);
        quad(60,10,20,20,255,255,255,255);
        check(renderer.pixels()[at(62,12)]==rgb(255,255,255)&&renderer.pixels()[at(77,12)]==rgb(255,0,0),"2D texture coordinates");
        check(renderer.pixels()[at(77,27)]==rgb(0,0,255),"texture alpha test");
        auto pixels=raster->lock(0,rw::Raster::LOCKWRITE);check(pixels!=nullptr,"raster lock");pixels[0]=0;pixels[1]=0;pixels[2]=0;raster->unlock(0);
        quad(85,10,20,20,255,255,255,255);check(renderer.pixels()[at(87,12)]==0,"texture cache invalidated on unlock");

        // Certify uniform/opaque, then mutate RGB and alpha through the real
        // raster API. Stale certificates would incorrectly hide these changes.
        for(unsigned pass=0;pass<3;++pass){auto bytes=raster->lock(0,rw::Raster::LOCKWRITE);for(unsigned i=0;i<4;++i){bytes[i*4]=255;bytes[i*4+1]=255;bytes[i*4+2]=255;bytes[i*4+3]=255;}if(pass==1){bytes[0]=bytes[1]=0;bytes[2]=255;}if(pass==2)bytes[3]=0;raster->unlock(0);
          renderer.clear(rgb(0,0,255));rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::LINEAR);quad(60,10,20,20,255,255,255,255);
          renderer.screenCertificatesEnabled(false);auto reference=renderer.pixels();auto depth=renderer.depth();renderer.clear(rgb(0,0,255));quad(60,10,20,20,255,255,255,255);check(reference==renderer.pixels()&&depth==renderer.depth(),"real raster certificate dirty lifecycle");renderer.screenCertificatesEnabled(true);
        }
        rw::SetRenderState(rw::TEXTUREFILTER,rw::Texture::NEAREST);
        rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);raster->destroy();
        rw::SetRenderState(rw::ZTESTENABLE,1);rw::SetRenderState(rw::ZWRITEENABLE,1);
        rw::cpu::Im3DVertex tri[3];tri[0].position={-1,-1,5};tri[1].position={1,-1,5};tri[2].position={0,1,5};
        for(auto& v:tri)v.setColor(255,255,0,255);
        rw::im3d::Transform(tri,3,nullptr,rw::im3d::ALLOPAQUE);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();
        check(renderer.pixels()[at(160,120)]==rgb(255,255,0),"Im3D camera projection");
        for(auto& v:tri){v.position.z=7;v.setColor(255,0,0,255);}rw::im3d::Transform(tri,3,nullptr,rw::im3d::ALLOPAQUE);rw::im3d::RenderPrimitive(rw::PRIMTYPETRILIST);rw::im3d::End();
        check(renderer.pixels()[at(160,120)]==rgb(255,255,0),"3D depth test");
        auto geometry=rw::Geometry::create(3,1,rw::Geometry::POSITIONS);auto material=rw::Material::create();material->color={0,255,0,255};geometry->triangles[0].matId=uint16_t(geometry->matList.appendMaterial(material));material->destroy();
        for(int i=0;i<3;++i){geometry->triangles[0].v[i]=uint16_t(i);geometry->morphTargets[0].vertices[i]=tri[i].position;geometry->morphTargets[0].vertices[i].z=3;}
        geometry->calculateBoundingSphere();auto atomic=rw::Atomic::create();atomic->setGeometry(geometry,0);geometry->destroy();atomic->render();
        check(renderer.pixels()[at(160,120)]==rgb(0,255,0),"default librw Atomic pipeline");atomic->destroy();
        device.end();auto cameraPixels=device.camera()->frameBuffer->lock(0,rw::Raster::LOCKREAD);uint16_t color;std::memcpy(&color,cameraPixels+at(160,120)*2,2);device.camera()->frameBuffer->unlock(0);
        check(color==rgb(0,255,0),"endUpdate writes camera raster");check(device.unsupported()==0,"supported callbacks never hit NULL or reject");
        // Compare complete shifted/yawed marker frame against the direct path.
        Camera transformed;transformed.position={1,2,-8};transformed.yaw=.2f;
        CheckpointRun route({{0,0,15}},{0,.825f,0});Renderer direct;direct.clear(0);route.draw(direct,transformed);renderer.clear(0);device.begin(transformed);device.checkpoint(route,transformed);device.end();
        check(direct.pixels()==renderer.pixels(),"Im3D world/camera matrix exact frame match");
        // Real raster texture + near-plane clipping, compared against the independent direct API.
        renderer.clear(0);direct.clear(0);device.begin(camera);
        auto textured=rw::Raster::create(2,2,16,rw::Raster::TEXTURE|rw::Raster::C565);
        uint16_t colors[4]={rgb(255,0,0),rgb(0,255,0),rgb(0,0,255),rgb(255,255,255)};
        std::memcpy(textured->lock(0,rw::Raster::LOCKWRITE),colors,8);textured->unlock(0);
        Vec3 positions[3]={{-.03f,.01f,.05f},{1,-1,5},{-1,-1,5}};Vec2 uv[3]={{.5f,0},{1,1},{0,1}};
        for(int i=0;i<3;++i){tri[i].position={positions[i].x,positions[i].y,positions[i].z};tri[i].u=uv[i].u;tri[i].v=uv[i].v;tri[i].setColor(255,255,255,255);}
        rw::SetRenderStatePtr(rw::TEXTURERASTER,textured);rw::im3d::Transform(tri,3,nullptr,rw::im3d::EVERYTHING);rw::im3d::RenderTriangle(0,1,2);rw::im3d::End();
        Texture t;t.width=t.height=2;t.pixels.assign(colors,colors+4);t.alpha.assign(4,255);direct.texturedTriangle(positions,uv,rgb(255,255,255),t,3,3,camera);
        check(renderer.pixels()==direct.pixels(),"Im3D textured near-plane clipping exact");
        rw::SetRenderStatePtr(rw::TEXTURERASTER,nullptr);textured->destroy();device.end();
        // Exact RGB565 transfer through three real Im2D textured HUD primitives.
        std::vector<uint16_t> overlay(320*240);for(size_t i=0;i<overlay.size();++i)overlay[i]=uint16_t(i*73);
        renderer.clear(0);device.begin(camera);device.hud(overlay);device.end();
        for(auto z:renderer.depth())check(z==0,"HUD does not change 3D depth");
        for(int y=0;y<240;++y)for(int x=0;x<320;++x){bool inside=y<23||y>=217||(x>=243&&x<317&&y>=139&&y<213);
            check(renderer.pixels()[at(x,y)]==(inside?overlay[at(x,y)]:0),"RGB565 HUD transfer exact");}
        check(device.unsupported()==0,"zero unsupported after demo paths");
        // Unsupported and invalid operations are explicit, not silent no-ops.
        device.begin(camera);auto before=device.unsupported();uint16_t bad[3]={0,1,100};rw::cpu::Im2DVertex v[3];rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,v,3,bad,3);check(device.unsupported()==before+1,"bad index rejected");
        rw::im2d::RenderLine(v,3,0,1);check(device.unsupported()==before+1,"supported line accepted");
        rw::im2d::RenderLine(v,3,0,4);check(device.unsupported()==before+2,"invalid line index rejected");
        auto invalid=rw::Raster::create(2,2,8,rw::Raster::TEXTURE|rw::Raster::PAL8);check(!invalid&&device.unsupported()==before+3,"unsupported raster rejected without leak");
        device.end();
    }
    check(rw::Engine::state==rw::Engine::Dead&&!rw::engine,"engine cleanup");
    check(!rw::Raster::numAllocated&&!rw::Camera::numAllocated&&!rw::Frame::numAllocated&&!rw::Geometry::numAllocated&&!rw::Material::numAllocated&&!rw::Atomic::numAllocated&&!rw::Image::numAllocated,"object cleanup");
    std::puts("real librw Device/Camera/Raster/Im2D alpha+texture/Im3D/default Atomic/cleanup PASS");return 0;
}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
