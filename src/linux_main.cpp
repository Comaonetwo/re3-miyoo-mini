// Linux/ARM standalone Miyoo demo. No SDL or shared-library dependencies.
#include "soft_renderer.hpp"
#include "fb_blit.hpp"
#include "asset_scene.hpp"
#include "exit_gesture.hpp"
#include "menu_actions.hpp"
#include "driving_clock.hpp"
#include "driving.hpp"
#include "checkpoint_run.hpp"
#ifdef SOFT_RW_DEVICE
#include "demo_settings.hpp"
#endif
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <memory>

namespace {
volatile sig_atomic_t stopping=0;
void stop(int) { stopping=1; }
soft::Channel field(fb_bitfield f) {return {f.offset,f.length,f.msb_right};}
class Display {
    int fd_=-1;uint8_t* mapped_=nullptr;size_t length_=0;
    fb_var_screeninfo original_{};fb_fix_screeninfo fixed_{};
    size_t visible_=0;std::vector<uint8_t> saved_;
    std::unique_ptr<soft::FrameBlitter> blitter_;
public:
    Display() {
        fd_=open("/dev/fb0",O_RDWR|O_CLOEXEC);
        if(fd_<0) throw std::runtime_error(std::string("open /dev/fb0: ")+strerror(errno));
        try {
            if(ioctl(fd_,FBIOGET_FSCREENINFO,&fixed_)<0||ioctl(fd_,FBIOGET_VSCREENINFO,&original_)<0)
                throw std::runtime_error(std::string("framebuffer info: ")+strerror(errno));
            if(fixed_.type!=FB_TYPE_PACKED_PIXELS||fixed_.visual!=FB_VISUAL_TRUECOLOR)
                throw std::runtime_error("framebuffer is not packed truecolor");
            if(original_.nonstd||original_.grayscale) throw std::runtime_error("unsupported framebuffer mode");
            soft::DisplayFormat f{original_.xres,original_.yres,fixed_.line_length,original_.bits_per_pixel,
                field(original_.red),field(original_.green),field(original_.blue),field(original_.transp)};
            blitter_.reset(new soft::FrameBlitter(f));
            length_=fixed_.smem_len;
            visible_=size_t(original_.yoffset)*fixed_.line_length+size_t(original_.xoffset)*(original_.bits_per_pixel/8);
            size_t required=blitter_->requiredBytes();
            if(length_==0||length_>32*1024*1024||visible_>length_||required>length_-visible_||
               uint64_t(original_.xoffset+original_.xres)*(original_.bits_per_pixel/8)>fixed_.line_length)
                throw std::runtime_error("framebuffer page is outside mapped memory");
            void* p=mmap(nullptr,length_,PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
            if(p==MAP_FAILED) throw std::runtime_error(std::string("mmap framebuffer: ")+strerror(errno));
            mapped_=static_cast<uint8_t*>(p);
            saved_.assign(mapped_+visible_,mapped_+visible_+required);
            std::printf("framebuffer=%ux%u bpp=%u pitch=%u yoffset=%u smem=%zu rotate=180\n",
                original_.xres,original_.yres,original_.bits_per_pixel,fixed_.line_length,original_.yoffset,length_);
        } catch(...) { if(mapped_)munmap(mapped_,length_);close(fd_);fd_=-1;throw; }
    }
    ~Display() {
        if(mapped_) {
            std::copy(saved_.begin(),saved_.end(),mapped_+visible_);
            munmap(mapped_,length_);
        }
        if(fd_>=0)close(fd_);
    }
    void present(const uint16_t* pixels,bool rotated) {blitter_->present(pixels,mapped_+visible_,length_-visible_,rotated);}
};
// musl uses 64-bit time_t, while Miyoo's 32-bit kernel evdev ABI uses
// two 32-bit timestamp words. Decode that ABI explicitly on ARM32.
#if defined(__arm__) && !defined(__aarch64__)
struct KernelInputEvent { uint32_t seconds,microseconds;uint16_t type,code;int32_t value; };
static_assert(sizeof(KernelInputEvent)==16,"ARM32 evdev event layout");
#else
using KernelInputEvent=input_event;
#endif
class Buttons {
    int fd_=-1;
public:
    bool held[KEY_MAX+1]={};soft::MenuActions menu;
    bool lightingRequested=false,paintRequested=false,qualityRequested=false;
    bool mainMenuRequested=false,resetRequested=false,toggleModeRequested=false;
    Buttons() {
        fd_=open("/dev/input/event0",O_RDONLY|O_NONBLOCK|O_CLOEXEC);
        if(fd_<0)throw std::runtime_error(std::string("open buttons: ")+strerror(errno));
        unsigned long state[(KEY_MAX+sizeof(unsigned long)*8)/(sizeof(unsigned long)*8)]={};
        if(ioctl(fd_,EVIOCGKEY(sizeof(state)),state)>=0)for(unsigned i=0;i<=KEY_MAX;++i)
            held[i]=(state[i/(sizeof(unsigned long)*8)]>>(i%(sizeof(unsigned long)*8)))&1;
        if(held[KEY_ESC])menu.reset();
    }
    ~Buttons(){if(fd_>=0)close(fd_);}
    void poll(bool& running,bool& paused,bool& rotated,bool& textured,soft::Camera& camera,const soft::Camera& spawn) {
        KernelInputEvent e;
        while(read(fd_,&e,sizeof(e))==sizeof(e)) {
            if(e.type==EV_SYN&&e.code==SYN_DROPPED) {std::fill(held,held+KEY_MAX+1,false);menu.reset();continue;}
            if(e.type!=EV_KEY||e.code>KEY_MAX||e.value<0||e.value>2)continue;
            bool was=held[e.code];held[e.code]=e.value!=0;
            auto exit=soft::exitGesture(e.code,e.value,held[KEY_ESC],held[KEY_RIGHTCTRL]);
            if(exit!=soft::ExitRequest::None){
                mainMenuRequested=exit==soft::ExitRequest::MainMenu;running=false;
                std::printf("exit_request=%s\n",mainMenuRequested?"main-menu":"port-list");return;
            }
            auto action=menu.event(e.code,e.value,held[KEY_ESC],held[KEY_RIGHTCTRL]);
            if(action==soft::MenuAction::Lighting)lightingRequested=true;
            if(action==soft::MenuAction::Paint){paintRequested=true;continue;}
            if(action==soft::MenuAction::Quality){qualityRequested=true;continue;}
            if(e.value==1&&!was) {
                // Confirmed Miyoo evdev mapping: Select=97, Start=28,
                // A=57, X=42. Menu+Select is handled above before other actions.
                if(e.code==KEY_ENTER)paused=!paused;
                if(e.code==KEY_SPACE){camera=spawn;resetRequested=true;}
                if(e.code==KEY_T)toggleModeRequested=true;
                if(e.code==KEY_LEFTCTRL)camera.farZ=camera.farZ<100?160:80;
                if(e.code==KEY_LEFTSHIFT)rotated=!rotated;
                if(e.code==KEY_LEFTALT)textured=!textured;
                std::printf("key=%u paused=%d rotated=%d textures=%d\n",e.code,paused,rotated,textured);
            }
        }
    }
};

}
int main(int argc,char** argv) {
    setvbuf(stdout,nullptr,_IOLBF,0);setvbuf(stderr,nullptr,_IOLBF,0);
    std::printf("GTA3Soft SD demo v0.10; owner assets; real librw CPU Device, not full re3\n");
    const char* scenePath=nullptr;const char* carPath=nullptr;const char* collisionPath=nullptr;unsigned limit=0;
    if(argc==5&&std::strcmp(argv[1],"--drive")==0){scenePath=argv[2];carPath=argv[3];collisionPath=argv[4];}
    else if(argc==3&&std::strcmp(argv[1],"--scene")==0)scenePath=argv[2];
    else if(argc==2)limit=unsigned(std::strtoul(argv[1],nullptr,10));
    else if(argc!=1){std::fprintf(stderr,"usage: demo [frames] | --scene file.m3s | --drive city.m3s car.m3s city.m3c\n");return 2;}
    struct sigaction action{};action.sa_handler=stop;sigemptyset(&action.sa_mask);
    sigaction(SIGTERM,&action,nullptr);sigaction(SIGINT,&action,nullptr);sigaction(SIGHUP,&action,nullptr);
    try {
        soft::AssetScene assets;soft::Camera spawn;
        auto scene=scenePath?std::vector<soft::Triangle>{}:soft::makeScene();
        if(scenePath){assets=soft::AssetScene::load(scenePath);spawn=assets.spawn;spawn.farZ=80;
            std::printf("asset_scene=%s objects=%zu triangles=%zu mesh_memory=%zu distance=80\n",
                scenePath,assets.groups.size(),assets.triangles.size(),assets.triangles.size()*sizeof(soft::Triangle));}
        soft::AssetScene car;std::unique_ptr<soft::CollisionWorld> world;std::unique_ptr<soft::Driving> driver;
        if(carPath){car=soft::AssetScene::load(carPath);world.reset(new soft::CollisionWorld(soft::CollisionWorld::load(collisionPath)));
            auto carSpawn=spawn.position;carSpawn.z+=10;driver.reset(new soft::Driving(*world,carSpawn,spawn.yaw));
            std::printf("car_triangles=%zu collision_faces=%zu\n",car.triangles.size(),world->faceCount());}
        std::unique_ptr<soft::CheckpointRun> run;std::unique_ptr<soft::MiniMap> map;
        if(driver){std::printf("Preparing checkpoint route and minimap...\n");
            run.reset(new soft::CheckpointRun(soft::CheckpointRun::street(*world,driver->position,driver->yaw)));
            map.reset(new soft::MiniMap(*world,driver->position,driver->yaw));}
        bool driveMode=bool(driver);
        const bool assetTextured=!assets.textures.empty();
        Display display;Buttons buttons;soft::Renderer renderer;soft::Camera camera=spawn;
#ifdef SOFT_RW_DEVICE
        soft::SoftwareDevice device(renderer);
        std::unique_ptr<soft::RwAssetScene> rwCity,rwCar;
        if(scenePath)rwCity.reset(new soft::RwAssetScene(assets,device.world()));
        if(driver)rwCar.reset(new soft::RwAssetScene(car,device.world()));
        assets.discardConvertedData();car.discardConvertedData();
        soft::DemoSettings settings;settings.apply(device,rwCar.get());
        std::vector<soft::TransparentAtomic> transparent;
        std::printf("rw_scene_atomics=%zu rw_car_atomics=%zu\n",rwCity?rwCity->atomics():0,rwCar?rwCar->atomics():0);
        std::printf("software_device=librw CPU; camera/raster/Im2D/Im3D callbacks active\n");
#endif
        std::vector<uint16_t> frame(320*240);
        bool running=true,paused=false,rotated=true,textured=true;
        soft::DrivingClock physics;
        unsigned frames=0,sampleFrames=0;double fps=0;
        auto previous=std::chrono::steady_clock::now(),sample=previous;
        while(running&&!stopping) {
            auto begin=std::chrono::steady_clock::now();
            buttons.poll(running,paused,rotated,textured,camera,spawn);if(!running||stopping)break;
            if(buttons.resetRequested){if(driver){driver->reset(*world);run->reset(driver->position);physics.reset();}buttons.resetRequested=false;}
            if(buttons.toggleModeRequested){if(driver){driveMode=!driveMode;physics.reset();}buttons.toggleModeRequested=false;}
            double elapsedFrame=std::chrono::duration<double>(begin-previous).count();
            float dt=std::min(float(elapsedFrame),.05f);previous=begin;
            physics.advance(elapsedFrame,driveMode&&!paused,[&](float tick){driver->update(*world,tick,
                int(buttons.held[KEY_UP])-int(buttons.held[KEY_DOWN]),
                int(buttons.held[KEY_RIGHT])-int(buttons.held[KEY_LEFT]),buttons.held[KEY_E]);});
            if(!paused){
                if(!driveMode){
                    camera.yaw+=(int(buttons.held[KEY_RIGHT])-int(buttons.held[KEY_LEFT]))*dt*1.5f;
                    float d=(int(buttons.held[KEY_UP])-int(buttons.held[KEY_DOWN]))*dt*6.f;
                    camera.position.x+=std::sin(camera.yaw)*d;camera.position.z+=std::cos(camera.yaw)*d;
                    if(scenePath)camera.position.y=std::max(2.f,std::min(100.f,camera.position.y+
                        (driver?int(buttons.held[KEY_E]):int(buttons.held[KEY_T])-int(buttons.held[KEY_E]))*dt*6.f));
                }
            }
            if(run)run->update(driver->position,driver->speed,elapsedFrame,driveMode&&!paused);
            if(driveMode)camera=driver->chaseCamera(camera.farZ,world.get());
#ifdef SOFT_RW_DEVICE
            bool settingsChanged=buttons.lightingRequested||buttons.paintRequested||buttons.qualityRequested;
            if(buttons.lightingRequested){settings.night=!settings.night;buttons.lightingRequested=false;}
            if(buttons.paintRequested){++settings.paintIndex;buttons.paintRequested=false;}
            if(buttons.qualityRequested){settings.quality=!settings.quality;buttons.qualityRequested=false;}
            if(settingsChanged)settings.apply(device,rwCar.get());device.begin(camera);transparent.clear();
#endif
#ifdef SOFT_RW_DEVICE
            device.worldTextures(textured);
            if(rwCity){renderer.clear(settings.sky());rwCity->gather(camera,transparent);}else soft::renderScene(renderer,scene,camera);
            if(rwCar)rwCar->gather(camera,transparent,driver.get());
#else
            if(scenePath)assets.render(renderer,camera,textured);else soft::renderScene(renderer,scene,camera);
            if(driver)driver->draw(renderer,car,camera,textured);
#endif
            if(run){
#ifdef SOFT_RW_DEVICE
                device.checkpoint(*run,camera);
#else
                run->draw(renderer,camera);
#endif
            }
#ifdef SOFT_RW_DEVICE
            soft::renderTransparent(transparent);if(run)device.checkpointGlow(*run);
#endif
            std::copy(renderer.pixels().begin(),renderer.pixels().end(),frame.begin());
            std::fill(frame.begin(),frame.begin()+320*12,soft::rgb(15,20,28));
            std::fill(frame.end()-320*23,frame.end(),soft::rgb(15,20,28));
            soft::drawText(frame,4,3,driver?"GTA3 DRIVE":scenePath?"GTA3 CHINATOWN":"GTA3 SOFTWARE DEMO");char status[48];
            std::snprintf(status,sizeof(status),"FPS %.1f D%.0f %s%s",fps,camera.farZ,textured&&assetTextured?"T":"F",paused?" P":"");
            if(driver)soft::drawDrivingHud(frame,*map,*run,*driver,fps,camera.farZ,textured,paused,driveMode);
            else{soft::drawText(frame,155,3,status);soft::drawText(frame,4,220,scenePath?"D-PAD MOVE L-R HEIGHT B DIST":"D-PAD MOVE START PAUSE");
                soft::drawText(frame,4,230,scenePath?"A RESET Y TEX X FLIP MENU-SELECT EXIT":"A RESET X FLIP MENU-SELECT EXIT");}
#ifdef SOFT_RW_DEVICE
            settings.hud(frame,driveMode);device.hud(frame);device.end();
            std::copy(renderer.pixels().begin(),renderer.pixels().end(),frame.begin());
#endif
            display.present(frame.data(),rotated);++frames;++sampleFrames;
            double elapsed=std::chrono::duration<double>(begin-sample).count();
            if(elapsed>=2) {fps=sampleFrames/elapsed;sampleFrames=0;sample=begin;
                std::printf("frames=%u fps=%.2f textures=%d triangles=%llu writes=%llu\n",frames,fps,textured,
                    (unsigned long long)renderer.stats().submitted,(unsigned long long)renderer.stats().pixels);}
            if(driver&&elapsed>=2)std::printf("driving=%d speed=%.2f position=%.2f %.2f %.2f blocked=%d\n",
                driveMode,driver->speed,driver->position.x,driver->position.y,driver->position.z,driver->hit);
            if(run&&elapsed>=2)std::printf("gates=%zu/%zu run_seconds=%.2f finished=%d wheel_angle=%.2f steer_angle=%.2f\n",
                run->completed(),run->route().size(),run->seconds(),run->finished(),driver->wheelAngle,driver->steeringAngle);
#ifdef SOFT_RW_DEVICE
            if(elapsed>=2)std::printf("night=%d quality=%d paint=%u rw_shaded=%llu rw_atomics=%llu rw_im2d_triangles=%llu rw_im3d_triangles=%llu rw_unsupported=%llu\n",
                settings.night,settings.quality,settings.paintIndex%5,(unsigned long long)device.shadedTriangles(),(unsigned long long)device.atomicCalls(),(unsigned long long)device.im2dTriangles(),(unsigned long long)device.im3dTriangles(),(unsigned long long)device.unsupported());
#endif
            if(elapsed>=2){rusage usage{};if(getrusage(RUSAGE_SELF,&usage)==0)std::printf("peak_rss_kib=%ld\n",usage.ru_maxrss);
                auto memory=std::fopen("/proc/self/status","r");if(memory){char line[256];long current;while(std::fgets(line,sizeof(line),memory))if(std::sscanf(line,"VmRSS: %ld",&current)==1){std::printf("current_rss_kib=%ld\n",current);break;}std::fclose(memory);}}
            if(limit&&frames>=limit)break;
            std::this_thread::sleep_until(begin+std::chrono::milliseconds(33));
        }
        std::printf("exit frames=%u signal=%d\n",frames,int(stopping));
        return buttons.mainMenuRequested?soft::mainMenuExitCode:0;
    } catch(const std::exception& e) {std::fprintf(stderr,"ERROR: %s\n",e.what());return 1;}
}
