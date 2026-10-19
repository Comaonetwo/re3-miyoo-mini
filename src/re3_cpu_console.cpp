#include "port_version.h"
#include "loading_watch.hpp"
#include "re3_cpu_host.hpp"
#include "checkpoint_run.hpp"
#include "exit_gesture.hpp"
#include "core_menu.hpp"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <thread>
#include <chrono>
#include <sys/resource.h>
static uint32_t testButtons=0,testSettings=soft::CoreSettings::defaultBits;
void cpuHostTestSettings(uint32_t bits){testSettings=soft::CoreSettings(bits).bits;}
// Child-process override; the display/input parent keeps manual settings intact.
static float dynamicFarClip=-1;
void cpuDynamicFarClip(float distance){dynamicFarClip=std::isfinite(distance)&&distance>=80&&distance<=120?distance:-1;}
float cpuFarClip(){soft::CoreSettings s(cpuHostSettings());return s.dynamicWorld()&&dynamicFarClip>=0?dynamicFarClip:float(s.distance());}
bool cpuShadowsEnabled(){return soft::CoreSettings(cpuHostSettings()).shadows();}
void cpuHostTestButtons(uint32_t bits){testButtons=bits;}
#ifdef __linux__
#include "miyoo_display.hpp"
#include <linux/input.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <new>
namespace {
struct Shared {
 pthread_mutex_t frameMutex;
 std::atomic<uint32_t> buttons{0},stage{0},frames{0},settings{932},paused{0};
 unsigned imageWidth=320;
 uint16_t pixels[640*480];
};
Shared* shared=nullptr;
volatile sig_atomic_t stopped=0;
void stop(int){stopped=1;}
#if defined(__arm__) && !defined(__aarch64__)
struct InputEvent{uint32_t sec,usec;uint16_t type,code;int32_t value;};
static_assert(sizeof(InputEvent)==16,"Miyoo kernel evdev ABI");
#else
using InputEvent=input_event;
#endif
const char* labels[]={"STARTING","FILES AND STREAM","CPU RENDERER","CAMERA FONT HUD","GAME TABLES","CORE READY","LOADING WORLD","WORLD READY","RENDERING WORLD"};
}
uint32_t cpuHostButtons(){return shared?shared->buttons.load():testButtons;}
uint32_t cpuHostSettings(){return shared?shared->settings.load():testSettings;}
bool cpuHostPaused(){return shared&&shared->paused.load();}
void cpuHostStage(unsigned n,const char* label){struct rusage usage{};getrusage(RUSAGE_SELF,&usage);std::printf("STAGE %u %s peak_rss_kib=%ld\n",n,label,usage.ru_maxrss);if(shared)shared->stage.store(n);}
void cpuHostPresent(const std::vector<uint16_t>& frame){if(!shared)return;pthread_mutex_lock(&shared->frameMutex);if(frame.size()!=320*240&&frame.size()!=640*480){pthread_mutex_unlock(&shared->frameMutex);return;}shared->imageWidth=frame.size()==640*480?640:320;std::memcpy(shared->pixels,frame.data(),frame.size()*2);shared->frames.fetch_add(1);pthread_mutex_unlock(&shared->frameMutex);}
int cpuConsole(int argc,char**argv){
 try{softHost::MiyooDisplay display;int input=open("/dev/input/event0",O_RDONLY|O_NONBLOCK|O_CLOEXEC);if(input<0)throw std::runtime_error("Cannot open Miyoo buttons");
 void* memory=mmap(nullptr,sizeof(Shared),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);if(memory==MAP_FAILED){close(input);throw std::runtime_error("Cannot allocate shared frame");}shared=new(memory)Shared;
 pthread_mutexattr_t attr;pthread_mutexattr_init(&attr);pthread_mutexattr_setpshared(&attr,PTHREAD_PROCESS_SHARED);pthread_mutex_init(&shared->frameMutex,&attr);pthread_mutexattr_destroy(&attr);
 soft::CoreMenu menu;menu.settings=soft::CoreSettings::load("settings.cfg");shared->settings.store(menu.settings.bits);
 signal(SIGTERM,stop);signal(SIGINT,stop);signal(SIGHUP,stop);
 pid_t child=fork();if(child<0){close(input);munmap(memory,sizeof(Shared));shared=nullptr;throw std::runtime_error("Cannot start kernel process");}
 if(!child){close(input);signal(SIGTERM,SIG_DFL);signal(SIGINT,SIG_DFL);signal(SIGHUP,SIG_DFL);try{_exit(cpuKernel(argc,argv));}catch(const std::exception& e){std::fprintf(stderr,"Kernel failed: %s\n",e.what());_exit(5);}}
 bool held[KEY_MAX+1]={};bool rotated=true,mainMenu=false,menuPending=false,wasMenuOpen=false,drainInput=false;uint32_t savedSettings=menu.settings.bits,savedCustom=menu.settings.customGraphics;bool savedCustomSelected=menu.settings.customSelected;unsigned long state[(KEY_MAX+sizeof(unsigned long)*8)/(sizeof(unsigned long)*8)]={};
 if(ioctl(input,EVIOCGKEY(sizeof(state)),state)>=0)for(unsigned i=0;i<=KEY_MAX;++i)held[i]=(state[i/(sizeof(unsigned long)*8)]>>(i%(sizeof(unsigned long)*8)))&1;
 auto started=std::chrono::steady_clock::now();auto fpsTime=started;unsigned fpsFrames=0;float fps=0;std::vector<uint16_t> frame(320*240);int status=0;bool finished=false;unsigned lastStage=99,lastFrame=0;
 bool firstPresent=true;uint64_t presented=0;double presentMs=0;
 soft::LoadingWatch loadingWatch;
 while(!stopped){bool dirty=firstPresent;firstPresent=false;InputEvent e;while(read(input,&e,sizeof(e))==sizeof(e)){
  if(e.type==EV_SYN&&e.code==SYN_DROPPED){std::fill(held,held+KEY_MAX+1,false);continue;}
  if(e.type!=EV_KEY||e.code>KEY_MAX||e.value<0||e.value>2)continue;held[e.code]=e.value!=0;if(e.value==1)dirty=true;
  if(soft::exitGesture(e.code,e.value,held[KEY_ESC],held[KEY_RIGHTCTRL])!=soft::ExitRequest::None){mainMenu=true;stopped=1;break;}
  if(e.code==KEY_ESC&&e.value==1)menuPending=true;
  if(held[KEY_ESC]&&(e.code==KEY_LEFTSHIFT||e.code==KEY_RIGHTCTRL))menuPending=false;
  if(e.code==KEY_LEFTSHIFT&&e.value==1&&held[KEY_ESC]){rotated=!rotated;drainInput=true;}
  if(e.code==KEY_ESC&&e.value==0&&menuPending){menu.toggle();dirty=true;menuPending=false;}
  if(menu.open&&e.value==1&&!held[KEY_ESC]){
   if(e.code==KEY_UP)menu.key(soft::CoreKey::Up);if(e.code==KEY_DOWN)menu.key(soft::CoreKey::Down);
   if(e.code==KEY_LEFT)menu.key(soft::CoreKey::Left);if(e.code==KEY_RIGHT)menu.key(soft::CoreKey::Right);
   if(e.code==KEY_SPACE)menu.key(soft::CoreKey::Accept);if(e.code==KEY_LEFTCTRL)menu.key(soft::CoreKey::Back);
   if(e.code==KEY_ENTER)menu.key(soft::CoreKey::Start);
  }
 }
 unsigned codes[]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_SPACE,KEY_LEFTCTRL,KEY_LEFTSHIFT,KEY_LEFTALT,KEY_E,KEY_T,KEY_ENTER,KEY_RIGHTCTRL,KEY_TAB,KEY_BACKSPACE};uint32_t bits=0;for(unsigned i=0;i<14;++i)if(held[codes[i]]&&!(codes[i]==KEY_LEFTSHIFT&&held[KEY_ESC]))bits|=1u<<i;if(wasMenuOpen&&!menu.open)drainInput=true;if(drainInput&&!bits&&!held[KEY_ESC])drainInput=false;shared->buttons.store(menu.open||held[KEY_ESC]||drainInput?0:bits);shared->paused.store(menu.open);shared->settings.store(menu.settings.bits);
 if(wasMenuOpen&&!menu.open&&(savedSettings!=menu.settings.bits||savedCustom!=menu.settings.customGraphics||savedCustomSelected!=menu.settings.customSelected)){if(menu.settings.save("settings.cfg")){savedSettings=menu.settings.bits;savedCustom=menu.settings.customGraphics;savedCustomSelected=menu.settings.customSelected;std::printf("HOST settings saved=%u\n",savedSettings);}else std::fprintf(stderr,"Cannot save settings.cfg\n");}wasMenuOpen=menu.open;
 if(menu.exitRequested){mainMenu=true;stopped=1;break;}
 if(wasMenuOpen!=menu.open)dirty=true;unsigned stage=shared->stage.load(),frames=shared->frames.load();if(stage!=lastStage){lastStage=stage;dirty=true;std::printf("HOST stage=%u\n",stage);}
 if(frames!=lastFrame&&pthread_mutex_trylock(&shared->frameMutex)==0){frame.resize(shared->imageWidth*(shared->imageWidth*3/4));std::memcpy(frame.data(),shared->pixels,frame.size()*2);lastFrame=shared->frames.load();dirty=true;pthread_mutex_unlock(&shared->frameMutex);}
 if(!finished&&waitpid(child,&status,WNOHANG)==child){finished=true;dirty=true;if(WIFEXITED(status)&&WEXITSTATUS(status)==42){mainMenu=true;stopped=1;break;}}
 auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();loadingWatch.observe(stage,uint64_t(elapsed));
 if(loadingWatch.expired(uint64_t(elapsed))&&!finished){std::printf("HOST loading timeout 120s\n");kill(child,SIGTERM);waitpid(child,&status,0);finished=true;dirty=true;}
 if(!frames){std::fill(frame.begin(),frame.end(),soft::rgb(16,24,34));soft::drawText(frame,12,32,"GTA3 V" GTA3_PORT_VERSION " REAL RE3 CORE");soft::drawText(frame,12,56,labels[std::min(stage,8u)]);soft::drawText(frame,12,80,finished?(WIFEXITED(status)&&WEXITSTATUS(status)==0?"CORE TEST COMPLETE":"CORE STOPPED - SEE CORE.LOG"):"LOADING PLEASE WAIT");}
 if(frames){auto now=std::chrono::steady_clock::now();double dt=std::chrono::duration<double>(now-fpsTime).count();if(dt>=1){fps=(frames-fpsFrames)/dt;fpsFrames=frames;fpsTime=now;if(menu.settings.stats()&&!menu.open)dirty=true;}if(menu.settings.stats()&&stage!=9){std::fill(frame.begin(),frame.begin()+(frame.size()==640*480?640*20:320*10),soft::rgb(8,12,18));char line[48];std::snprintf(line,sizeof(line),"RE3 FPS %.1f FRAME %u",fps,frames);soft::drawText(frame,6,1,line);}if(finished)soft::drawText(frame,6,14,"CORE STOPPED - SEE CORE.LOG");}
 if(!menu.open&&!frames)soft::drawText(frame,6,230,"MENU SETTINGS  MENU-SELECT EXIT");if(menu.open&&frame.size()==640*480){std::vector<uint16_t> m(320*240);menu.draw(m);for(unsigned y=0;y<480;++y)for(unsigned x=0;x<640;++x)frame[y*640+x]=m[(y/2)*320+x/2];}else menu.draw(frame);if(dirty){auto presentStart=std::chrono::steady_clock::now();display.present(frame.data(),rotated,frame.size()==640*480?640:320);++presented;presentMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-presentStart).count();if(presented%120==0)std::printf("HOST DISPLAY presented=%llu mean_ms=%.3f\n",(unsigned long long)presented,presentMs/presented);}std::this_thread::sleep_for(std::chrono::milliseconds(33));
 }
 if((savedSettings!=menu.settings.bits||savedCustom!=menu.settings.customGraphics||savedCustomSelected!=menu.settings.customSelected)&&!menu.settings.save("settings.cfg"))std::fprintf(stderr,"Cannot save settings on exit\n");
 if(!finished){kill(child,SIGTERM);waitpid(child,&status,0);}std::printf("HOST child_status=%d main_menu=%d presented=%llu\n",status,mainMenu,(unsigned long long)presented);close(input);munmap(memory,sizeof(Shared));shared=nullptr;return mainMenu?42:0;
 }catch(const std::exception& e){std::fprintf(stderr,"Console host: %s\n",e.what());return 2;}
}
#else
uint32_t cpuHostButtons(){return testButtons;}
uint32_t cpuHostSettings(){return testSettings;}
bool cpuHostPaused(){return false;}
void cpuHostStage(unsigned n,const char* label){struct rusage usage{};getrusage(RUSAGE_SELF,&usage);std::printf("STAGE %u %s native_peak_rss_kib=%ld\n",n,label,usage.ru_maxrss/1024);}
void cpuHostPresent(const std::vector<uint16_t>&){}
int cpuConsole(int,char**){return 2;}
#endif
