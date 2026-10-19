#include "port_version.h"
#include "core_menu.hpp"
#include "checkpoint_run.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <string>
namespace soft {
namespace {
// Graphics only. Camera, auto aim and FPS display are independent preferences.
constexpr uint32_t presetGraphics[]={
 (1u<<24)|(1u<<22)|3u|8u|32u|1024u|2048u|(2u<<15)|(1u<<17),
 (1u<<24)|(1u<<22)|3u|12u|32u|2048u|(2u<<15)|(1u<<17),
 (1u<<24)|(1u<<22)|3u|12u|16u|32u|64u|4096u|(2u<<15)|(1u<<17),
 (1u<<24)|(1u<<22)|1u|12u|16u|32u|64u|512u|(2u<<15)|(1u<<17)
};
}
const char* CoreSettings::presetName(unsigned id){const char* names[]={"LOW","MEDIUM","HIGH","ULTRA","CUSTOM"};return names[id<5?id:4];}
unsigned CoreSettings::preset()const{if(customSelected)return 4;for(unsigned i=0;i<4;++i)if((bits&graphicsMask)==presetGraphics[i])return i;return 4;}
void CoreSettings::applyPreset(unsigned id){if(id>4)return;if(preset()==4)customGraphics=bits&graphicsMask;
 bits=(bits&~graphicsMask)|(id==4?customGraphics:presetGraphics[id]);customSelected=id==4;}
CoreSettings CoreSettings::load(const char* path){CoreSettings s;FILE* f=std::fopen(path,"rb");if(!f)return s;
 char tag[32]={};unsigned version=0,bits=0;int n=std::fscanf(f,"%31s %u %u",tag,&version,&bits);
 if(n==3&&!std::strcmp(tag,"GTA3_CPU_SETTINGS")&&version==1&&valid(bits)){s=CoreSettings(bits&~((1u<<18)|(1u<<21)|(1u<<23)));
  char extra[32]={};unsigned custom=0,selected=0;
  if(std::fscanf(f,"%31s %u %u",extra,&custom,&selected)==3&&!std::strcmp(extra,"GTA3_CUSTOM")&&valid(custom)&&!(custom&~graphicsMask)&&selected<=1){s.customGraphics=custom&~((1u<<18)|(1u<<21)|(3u<<25));s.customSelected=selected;}
  char recipeTag[32]={};unsigned recipe=0;
  if(std::fscanf(f,"%31s %u",recipeTag,&recipe)!=2||std::strcmp(recipeTag,"GTA3_WORLD_LOD")||recipe!=2){s.bits&=~((1u<<21)|(3u<<25));s.customGraphics&=~((1u<<21)|(3u<<25));}
  char threadsTag[32]={};unsigned threads=1;if(std::fscanf(f,"%31s %u",threadsTag,&threads)==2&&!std::strcmp(threadsTag,"GTA3_RENDER_THREADS")&&threads==2){} // Retired UI option: always migrate to one core.
 }
 // Named presets adopt new defaults; explicit CUSTOM choices retain AUTO OFF.
 if(!s.customSelected&&(s.bits&graphicsMask)==(presetGraphics[1]^6144u))s.bits=(s.bits&~graphicsMask)|presetGraphics[1];
 const uint32_t newDefaults=(1u<<22)|(1u<<24);
 if(!s.customSelected&&((s.bits&graphicsMask)&~newDefaults)==(presetGraphics[3]&~(512u|newDefaults)))s.bits|=512u;
 if(!s.customSelected)for(unsigned i=0;i<4;++i)if(((s.bits&graphicsMask)&~newDefaults)==(presetGraphics[i]&~newDefaults))s.bits|=newDefaults;
 s.bits&=~((1u<<21)|(3u<<25));s.customGraphics&=~((1u<<21)|(3u<<25));
 std::fclose(f);return s;
}
bool CoreSettings::save(const char* path) const {if(!valid(bits)||!valid(customGraphics)||(customGraphics&~graphicsMask))return false;std::string temp=std::string(path)+".tmp";
 FILE* f=std::fopen(temp.c_str(),"wb");if(!f)return false;bool ok=std::fprintf(f,"GTA3_CPU_SETTINGS 1\n%u\nGTA3_CUSTOM %u %u\nGTA3_WORLD_LOD 2\nGTA3_RENDER_THREADS %u\n",bits&~(1u<<23),customSelected?(bits&graphicsMask):customGraphics,unsigned(customSelected),1u)>0;
 if(std::fclose(f)!=0)ok=false;if(ok&&std::rename(temp.c_str(),path)==0)return true;std::remove(temp.c_str());return false;
}
void CoreMenu::key(CoreKey k){if(!open)return;
    if(k==CoreKey::Back||k==CoreKey::Start){if(help)help=false;else if(about)about=false;else open=false;return;}if(help||about)return;
    if(k==CoreKey::Up){row=(row+rowCount-1)%rowCount;return;}if(k==CoreKey::Down){row=(row+1)%rowCount;return;}
    if(k==CoreKey::Accept&&row==16){help=true;return;}if(k==CoreKey::Accept&&row==17){about=true;return;}
    if(k==CoreKey::Accept&&row==18){open=false;return;}
    if(k==CoreKey::Accept&&row==19){exitRequested=true;return;}
    if(k!=CoreKey::Accept&&k!=CoreKey::Left&&k!=CoreKey::Right)return;
    if(row==0){unsigned id=settings.preset();const unsigned visible[]={1,2,4};unsigned index=id==1?0:id==2?1:2;settings.applyPreset(visible[(index+(k==CoreKey::Left?2:1))%3]);return;}
    const unsigned actionRow=row==2?15:(row>=3&&row<=15?row-1:row);
    const unsigned item=actionRow-1;const uint32_t before=settings.bits;
    if(item==1){settings.bits&=~(1u<<24);settings.distanceIndex((settings.distanceIndex()+(k==CoreKey::Left?6:1))%7);
    }else if(item==0){settings.bits&=~(1u<<24);unsigned shift=item*2,value=(settings.bits>>shift)&3;if(item==0){const unsigned order[]={2,0,3,1};unsigned index=0;while(order[index]!=value)++index;value=order[(index+(k==CoreKey::Left?3:1))%4];}else value=(value+(k==CoreKey::Left?3:1))%4;
        settings.bits=(settings.bits&~(3u<<shift))|(value<<shift);
    }else if(item==2){settings.bits=(settings.bits&~(1u<<18))^(1u<<4);
    }else if(item==7){const unsigned order[]={1024,0,512};unsigned value=settings.bits&1536,index=0;while(order[index]!=value)++index;settings.bits=(settings.bits&~1536u)|order[(index+(k==CoreKey::Left?2:1))%3];
    }else if(item==8){unsigned value=(settings.bits>>11)&3;value=(value+(k==CoreKey::Left?2:1))%3;settings.bits=(settings.bits&~6144u)|(value<<11);
    }else if(item==9){settings.bits^=1u<<13;
    }else if(item==10){settings.bits^=1u<<14;
    }else if(item==11){unsigned value=(settings.bits>>15)&3;const unsigned order[]={0,2,1};unsigned index=0;while(order[index]!=value)++index;value=order[(index+(k==CoreKey::Left?2:1))%3];settings.bits=(settings.bits&~98304u)|(value<<15);
    }else if(item==12){settings.bits&=~(1u<<24);unsigned value=settings.lowRain()?0:settings.reducedRain()?1:2;value=(value+(k==CoreKey::Left?2:1))%3;settings.bits&=~((1u<<17)|(1u<<27));if(value<2)settings.bits|=1u<<17;if(value==0)settings.bits|=1u<<27;
    }else if(item==13){settings.bits^=1u<<22;
    }else if(item==14){settings.bits^=1u<<24;

    }else if(item<7){unsigned flag=item==2?4:item==3?5:item==4?6:item==5?7:item==6?8:9;settings.bits^=1u<<flag;}
    if((before^settings.bits)&CoreSettings::graphicsMask)settings.graphicsChanged();
}
void CoreMenu::draw(std::vector<uint16_t>& f) const {if(!open||f.size()!=320*240)return;
    std::fill(f.begin(),f.end(),rgb(13,22,32));drawText(f,12,12,"GTA3 V" GTA3_PORT_VERSION " - MIYOO MENU");
    if(about){
        drawText(f,12,40,"ABOUT");
        drawText(f,12,65,"A PORT OF THE RE3 ENGINE");
        drawText(f,12,79,"FOR MIYOO MINI.");
        drawText(f,12,105,"VERSION " GTA3_PORT_VERSION);
        drawText(f,12,132,"WITH SUPPORT FROM: SNEZHA.");
        drawText(f,12,230,"B BACK");return;
    }
    if(help){const char* lines[]={"D-PAD MOVE/STEER/SCOPE","A SPRINT/GAS/ZOOM OUT","X JUMP/BRAKE/ZOOM IN","B FIRE AND AUTO AIM","Y ENTER OR LEAVE CAR",settings.camera()?"L-R FOOT CAMERA":"L-R ORIGINAL L1-R1","CAR L RADIO / R HANDBRAKE","FOOT L2 AIM / R2 LOOK BACK","CAR L2 MISSION / R2 HORN","L2-L/R CHANGE AIM TARGET","SELECT-X/Y WEAPON/CAR LOOK","SELECT-X-Y CAR LOOK BACK","SELECT-A CAMERA VIEW","SELECT-L CENTER CAMERA","SELECT-DPAD CAR TURRET","START GAME PAUSE","MENU SETTINGS","MENU-X FLIP SCREEN","MENU-SELECT EXIT TO ONION"};
        for(unsigned i=0;i<sizeof(lines)/sizeof(*lines);++i)drawText(f,12,31+i*10,lines[i]);drawText(f,12,230,"B BACK");return;}
    const char* labels[]={"GRAPHICS PRESET","WORLD RESOLUTION","DYNAMIC WORLD","DRAW DISTANCE","LIGHTING","WORLD TEXTURES","SHADOWS","FPS OVERLAY","L-R CAMERA","HUD RESOLUTION","MENU BACKGROUND","AUTO AIM","RENDER MODE","SKY RESOLUTION","RAIN QUALITY","PARTICLE DENSITY","HOTKEYS","ABOUT","RESUME GAME","EXIT TO ONION"};
    char distance[16];if(settings.dynamicWorld())std::snprintf(distance,sizeof(distance),"AUTO");else std::snprintf(distance,sizeof(distance),"%u M",settings.distance());
    const char* values[]={(settings.preset()==0||settings.preset()==3)?"CUSTOM":CoreSettings::presetName(settings.preset()),settings.dynamicWorld()?"AUTO":settings.scale()==1?"320X240":settings.scale()==2?"160X120":settings.scale()==3?"240X180":"80X60",settings.dynamicWorld()?"AUTO":"OFF",distance,settings.smooth()?"SMOOTH":settings.bakedLighting()?"BAKED":"FLAT",settings.textures()?"ON":"OFF",settings.shadows()?"ON":"OFF",settings.stats()?"ON":"OFF",settings.camera()?"ON":"OFF",settings.hudWidth()==240?"240X180":settings.hudWidth()==640?"640X480":"320X240",settings.menuWidth()==240?"240X180":settings.menuWidth()==320?"320X240":"640X480",settings.autoAim()?"ON":"OFF",settings.fastEffects()?"FAST":"STANDARD",settings.skyWidth()==160?"160X120":settings.skyWidth()==120?"REDUCED":"FULL",settings.dynamicWorld()?"AUTO":settings.lowRain()?"LOW":settings.reducedRain()?"MEDIUM":"FULL",settings.reducedParticles()?"REDUCED":"FULL","","","",""};
    const bool tip=row==2;
    unsigned first=row>=18?row-17:0;for(unsigned i=first;i<std::min(rowCount,first+18);++i){int y=32+9*(i-first);if(i==row)std::fill(f.begin()+y*320,f.begin()+(y+9)*320,rgb(35,67,84));drawText(f,12,y+2,labels[i]);drawText(f,116,y+2,values[i]);}
    if(tip){auto rounded=[&](int x,int y,int w,int h,uint16_t color){for(int dy=0;dy<h;++dy){int edge=std::min(dy,h-1-dy),inset=edge==0?5:edge==1?3:edge==2?2:edge<5?1:0;std::fill(f.begin()+(y+dy)*320+x+inset,f.begin()+(y+dy)*320+x+w-inset,color);}};
     rounded(182,32,130,170,rgb(54,91,112));rounded(183,33,128,168,rgb(18,36,49));
     const char* lines[]={"DYNAMIC WORLD","(recommended)","","AUTO ADJUSTS","- RAIN QUALITY","- WORLD RESOLUTION","- DRAW DISTANCE","","UP 21FPS","DOWN 18FPS"};
     for(unsigned i=0;i<sizeof(lines)/sizeof(*lines);++i)drawText(f,191,43+i*12,lines[i]);
    }
    drawText(f,12,218,"LEFT-RIGHT CHANGE  A SELECT  B BACK");drawText(f,12,230,"SETTINGS SAVED WHEN MENU CLOSES");
}
}
