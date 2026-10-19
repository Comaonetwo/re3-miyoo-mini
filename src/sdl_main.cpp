#include "soft_renderer.hpp"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
// CPU rasterization, SDL texture only for the final 2D display blit.
int main(int argc,char** argv) {
    const unsigned limit=argc>1?unsigned(std::strtoul(argv[1],nullptr,10)):0;
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER)!=0) { std::fprintf(stderr,"SDL_Init: %s\n",SDL_GetError()); return 1; }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
#ifdef SOFT_MIYOO
    const int windowWidth=320,windowHeight=240;
    const char* presenter="Miyoo Mini";
    const unsigned flags=0;
#else
    const int windowWidth=640,windowHeight=480;
    const char* presenter="software";
    const unsigned flags=SDL_RENDERER_SOFTWARE;
#endif
    int driver=-1;
    for(int i=0;i<SDL_GetNumRenderDrivers();++i) {
        SDL_RendererInfo info{};
        if(SDL_GetRenderDriverInfo(i,&info)==0 && std::strcmp(info.name,presenter)==0) driver=i;
    }
    if(driver<0) { std::fprintf(stderr,"Required SDL presenter '%s' missing\n",presenter);SDL_Quit();return 1; }
    SDL_Window* w=SDL_CreateWindow("GTA3 software experiment (procedural scene)",SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,windowWidth,windowHeight,SDL_WINDOW_SHOWN);
    SDL_Renderer* display=w?SDL_CreateRenderer(w,driver,flags):nullptr;
    SDL_Texture* texture=display?SDL_CreateTexture(display,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,320,240):nullptr;
    soft::Renderer r; soft::Camera c; auto scene=soft::makeScene();
    if(!texture) { std::fprintf(stderr,"SDL display: %s\n",SDL_GetError());SDL_DestroyRenderer(display);SDL_DestroyWindow(w);SDL_Quit();return 1; }
    SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_NONE);
    SDL_RendererInfo info{};SDL_GetRendererInfo(display,&info);
    std::printf("video=%s presenter=%s internal=320x240 window=%dx%d\n",SDL_GetCurrentVideoDriver(),info.name,windowWidth,windowHeight);
    bool running=true,paused=false,keys[SDL_NUM_SCANCODES]={}; unsigned frames=0; uint32_t prev=SDL_GetTicks(),start=prev;
    int status=0;
    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN||e.type==SDL_KEYUP) {
                const int scan=int(e.key.keysym.scancode);
                if(scan>=0&&scan<SDL_NUM_SCANCODES) keys[scan]=e.type==SDL_KEYDOWN;
                if(e.type==SDL_KEYDOWN&&!e.key.repeat) {
                    if(e.key.keysym.sym==SDLK_ESCAPE) running=false;
                    if(e.key.keysym.sym==SDLK_RETURN) paused=!paused;
                    if(e.key.keysym.sym==SDLK_SPACE) c=soft::Camera{};
                }
            }
            if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST) std::fill(keys,keys+SDL_NUM_SCANCODES,false);
        }
        if(!running) break;
        uint32_t now=SDL_GetTicks(); float dt=std::min((now-prev)*.001f,.05f);prev=now;
        if(!paused) {
            c.yaw+=(int(keys[SDL_SCANCODE_RIGHT])-int(keys[SDL_SCANCODE_LEFT]))*dt*1.5f;
            float d=(int(keys[SDL_SCANCODE_UP])-int(keys[SDL_SCANCODE_DOWN]))*dt*6.f;
            c.position.x+=std::sin(c.yaw)*d;c.position.z+=std::cos(c.yaw)*d;
        }
        soft::renderScene(r,scene,c);
        if(SDL_UpdateTexture(texture,nullptr,r.pixels().data(),640)!=0||SDL_RenderCopy(display,texture,nullptr,nullptr)!=0) { std::fprintf(stderr,"present: %s\n",SDL_GetError());status=1;break; }
        SDL_RenderPresent(display);
        ++frames;
        if(frames%300==0) std::printf("frames=%u elapsed_ms=%u tris=%llu writes=%llu\n",frames,SDL_GetTicks()-start,
            (unsigned long long)r.stats().submitted,(unsigned long long)r.stats().pixels);
        if(limit&&frames>=limit) break;
        uint32_t spent=SDL_GetTicks()-now; if(spent<33) SDL_Delay(33-spent);
    }
    std::printf("exit frames=%u elapsed_ms=%u status=%d\n",frames,SDL_GetTicks()-start,status);
    SDL_DestroyTexture(texture);SDL_DestroyRenderer(display);SDL_DestroyWindow(w);SDL_Quit();return status;
}
