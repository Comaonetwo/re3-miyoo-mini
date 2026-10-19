#include "common.h"
#include "platform.h"
#include "crossplatform.h"
#include "ControllerConfig.h"
#include "Pad.h"
#include "Lists.h"
#include "PlayerInfo.h"
#include "PlayerPed.h"
#include "Camera.h"
#include "Frontend.h"
#include "miyoo_aim.hpp"
#include "miyoo_extra_actions.hpp"
#include "Messages.h"
#include "Script.h"
#include <cstring>
#include "core_menu.hpp"
#include <chrono>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <stdexcept>
#include "re3_cpu_host.hpp"
size_t _dwMemAvailPhys=48*1024*1024;
RwUInt32 gGameState=GS_START_UP;
long _dwOperatingSystemVersion=OS_WINXP;
#ifdef DETECT_JOYSTICK_MENU
char gSelectedJoystickName[128]="Miyoo CPU input";
#endif
RwBool IsForegroundApp(){return TRUE;}
void _psCreateFolder(const char* path){mkdir(path,0755);}
const char* _psGetUserFilesFolder(){static std::string folder;if(folder.empty()){char root[256];if(!getcwd(root,sizeof(root)))throw std::runtime_error("Cannot resolve game folder for saves");folder=std::string(root)+"/userfiles";if(folder.size()>235)throw std::runtime_error("Save folder path too long");_psCreateFolder(folder.c_str());}return folder.c_str();}
static bool automaticAim=false;
static soft::MiyooViewLatch viewLatch;
bool cpuAutomaticAim(){return automaticAim;}
void CapturePad(RwInt32 padID){if(padID!=0)return;auto& state=CPad::GetPad(0)->PCTempJoyState;state.Clear();uint32_t b=cpuHostButtons();
state.DPadUp=(b&1)?255:0;state.DPadDown=(b&2)?255:0;state.DPadLeft=(b&4)?255:0;state.DPadRight=(b&8)?255:0;
state.LeftStickX=(b&4)?-128:(b&8)?128:0;state.LeftStickY=(b&1)?-128:(b&2)?128:0;
state.Cross=(b&16)?255:0;state.Circle=(b&32)?255:0;state.Square=!(b&2048)&&(b&64)?255:0;state.Triangle=!(b&2048)&&(b&128)?255:0;
state.LeftShoulder2=(b&2048)&&(b&64)?255:0;state.RightShoulder2=(b&2048)&&(b&128)?255:0;
auto player=FindPlayerPed();const bool car=player&&player->bInVehicle;auto extra=soft::miyooExtraActions(b,car,!FrontEndMenuManager.m_bMenuActive);extra.view=viewLatch.update(b,!FrontEndMenuManager.m_bMenuActive);if(extra.view)state.Cross=0;bool firearm=false;if(player){switch(player->GetWeapon()->m_eWeaponType){case WEAPONTYPE_COLT45:case WEAPONTYPE_UZI:case WEAPONTYPE_SHOTGUN:case WEAPONTYPE_AK47:case WEAPONTYPE_M16:firearm=true;break;default:break;}}
bool aim=soft::miyooAutoAim(b,firearm,player&&!player->bInVehicle&&!FindPlayerVehicle()&&player->GetPedState()!=PED_SNIPER_MODE,!FrontEndMenuManager.m_bMenuActive,soft::CoreSettings{cpuHostSettings()}.autoAim());
automaticAim=aim;
// GTA's console lock-on is gated by its mouse-camera flag.
if(!FrontEndMenuManager.m_bMenuActive)CCamera::m_bUseMouse3rdPerson=false;
bool camera=soft::CoreSettings{cpuHostSettings()}.camera()&&!(b&2048)&&player&&!car;const bool targeting=aim||extra.manualAim;
state.RightStickX=camera&&!targeting?((b&256)?-110:(b&512)?110:0):0;
state.LeftShoulder1=!camera&&(b&256)?255:0;state.RightShoulder1=!camera&&(b&512)?255:0;if(targeting){state.RightShoulder1=255;state.LeftShoulder2=(b&256)?255:0;state.RightShoulder2=(b&512)?255:0;}
// R3 toggles taxi/ambulance/fire/vigilante submissions in native scripts.
// Contextual physical shoulders keep that action distinct from the horn.
state.LeftShock=extra.horn?255:0;state.RightShock=(extra.mission||extra.lookBehind)?255:0;
if(extra.horn&&CPad::GetPad(0)->Mode==1)state.LeftShoulder1=255;
if(extra.horn&&CPad::GetPad(0)->Mode==2)state.RightShoulder1=255;
state.Select=extra.view?255:0;
if(extra.turret){state.RightStickX=(b&4)?-128:(b&8)?128:0;state.RightStickY=(b&1)?-128:(b&2)?128:0;state.LeftStickX=state.LeftStickY=0;state.DPadUp=state.DPadDown=state.DPadLeft=state.DPadRight=0;}
state.Start=(b&1024)?255:0;
ControlsManager.m_OldState=ControlsManager.m_NewState;for(int i=0;i<MAX_BUTTONS;++i)ControlsManager.m_NewState.buttons[i]=false;
ControlsManager.m_NewState.buttons[0]=state.Circle;ControlsManager.m_NewState.buttons[1]=state.Cross;ControlsManager.m_NewState.buttons[2]=state.Square;ControlsManager.m_NewState.buttons[3]=state.Triangle;
ControlsManager.m_NewState.buttons[6]=state.LeftShoulder1;ControlsManager.m_NewState.buttons[7]=state.RightShoulder1;ControlsManager.m_NewState.buttons[11]=state.Start;ControlsManager.m_NewState.buttons[10]=state.RightShock;ControlsManager.m_NewState.buttons[8]=state.Select;
ControlsManager.m_NewState.buttons[12]=state.DPadUp;ControlsManager.m_NewState.buttons[13]=state.DPadRight;ControlsManager.m_NewState.buttons[14]=state.DPadDown;ControlsManager.m_NewState.buttons[15]=state.DPadLeft;
}
const char* cpuControlHint(unsigned action){switch(action){
#define MIYOO_HINT(action,label) case action:return label;
#include "miyoo_hint_actions.inc"
#undef MIYOO_HINT
 default:return "N/A";
}}
// Native fixture-only probe: no save or persistent game state is written.
void cpuMiyooControlsTest(){
 auto player=FindPlayerPed();if(!player)throw std::runtime_error("controls probe needs player");
 auto pad=CPad::GetPad(0);const auto old=pad->OldState,now=pad->NewState,temp=pad->PCTempJoyState;const auto buttons=cpuHostButtons();const auto savedView=viewLatch;const auto controlOld=ControlsManager.m_OldState,controlNow=ControlsManager.m_NewState;const auto mode=pad->Mode;const auto disabled=pad->DisablePlayerControls;bool car=player->bInVehicle,menu=FrontEndMenuManager.m_bMenuActive,mouse=CCamera::m_bUseMouse3rdPerson,autoAim=automaticAim;
 auto restore=[&](){pad->OldState=old;pad->NewState=now;pad->PCTempJoyState=temp;pad->Mode=mode;pad->DisablePlayerControls=disabled;player->bInVehicle=car;FrontEndMenuManager.m_bMenuActive=menu;CCamera::m_bUseMouse3rdPerson=mouse;automaticAim=autoAim;viewLatch=savedView;ControlsManager.m_OldState=controlOld;ControlsManager.m_NewState=controlNow;cpuHostTestButtons(buttons);};
 unsigned probe=0;auto require=[&](bool ok){++probe;if(!ok)throw std::runtime_error(std::string("Miyoo controls/hints mismatch at ")+std::to_string(probe));};
 auto input=[&](uint32_t b,bool vehicle){viewLatch={};player->bInVehicle=vehicle;FrontEndMenuManager.m_bMenuActive=false;pad->Mode=0;pad->DisablePlayerControls=0;pad->OldState.Clear();cpuHostTestButtons(b);CapturePad(0);pad->NewState=pad->PCTempJoyState;};
 try{CRunningScript script;
  input(4096,true);require(script.GetPadState(0,19)&&pad->GetRightShockJustDown()&&!pad->GetHorn());pad->OldState=pad->NewState;require(!pad->GetRightShockJustDown());
  input(8192,true);require(pad->GetHorn()&&!script.GetPadState(0,19));
  input(4096,false);require(pad->GetTarget()&&!pad->GetLookBehindForPed());
  input(8192,false);require(pad->GetLookBehindForPed()&&!pad->GetTarget());
  input(2048|16,true);require(pad->CycleCameraModeUpJustDown()&&!pad->GetAccelerate());
  input(16,true);cpuHostTestButtons(2048|16);CapturePad(0);pad->NewState=pad->PCTempJoyState;require(pad->GetAccelerate()&&!pad->CycleCameraModeUpJustDown());
  cpuHostTestButtons(2048|16|64);CapturePad(0);pad->NewState=pad->PCTempJoyState;require(pad->GetAccelerate()&&pad->GetLookLeft()&&!pad->CycleCameraModeUpJustDown());
  input(2048,true);cpuHostTestButtons(2048|16);CapturePad(0);pad->NewState=pad->PCTempJoyState;require(pad->CycleCameraModeUpJustDown()&&!pad->GetAccelerate());
  input(2048|4,true);require(pad->GetCarGunLeftRight()==-128&&!pad->GetSteeringLeftRight());
  input(2048|64,true);require(pad->GetLookLeft()&&!pad->GetBrake());input(2048|128,true);require(pad->GetLookRight()&&!pad->GetExitVehicle());input(2048|64|128,true);require(pad->GetLookBehindForCar());
  wchar text[256];const char* token="~k~~TOGGLE_SUBMISSIONS~";for(unsigned i=0;i<=std::strlen(token);++i)text[i]=token[i];CMessages::InsertPlayerControlKeysInString(text);require(text[0]=='L'&&text[1]=='2'&&!text[2]);
  text[0]=123;ControlsManager.GetWideStringOfCommandKeys(VEHICLE_HORN,text,1);require(!text[0]);text[0]=123;ControlsManager.GetWideStringOfCommandKeys(VEHICLE_HORN,text,0);require(text[0]==123);
  restore();puts("PASS Miyoo native controls: mission R3/script19, horn, camera, aim/look, turret, drive-by, hint expansion and bounded strings");
 }catch(...){restore();throw;}
}
extern "C" {
double psTimer(){using namespace std::chrono;return duration<double,std::milli>(steady_clock::now().time_since_epoch()).count();}
RwBool psInitialize(){RsGlobal.maximumWidth=320;RsGlobal.maximumHeight=240;return TRUE;}
void psTerminate(){}
RwBool psCameraBeginUpdate(RwCamera* c){return RwCameraBeginUpdate(c)!=nil;}
void psCameraShowRaster(RwCamera* c){RwCameraShowRaster(c,nil,0);}
RwImage* psGrabScreen(RwCamera* c){return RwCameraGetRaster(c)->toImage();}
void psMouseSetPos(RwV2d*){}
RwBool psSelectDevice(){return TRUE;}
RwMemoryFunctions* psGetMemoryFunctions(){return nil;}
RwBool psInstallFileSystem(){return TRUE;}
RwBool psNativeTextureSupport(){return TRUE;}
void _InputTranslateShiftKeyUpDown(RsKeyCodes*){}
long _InputInitialiseMouse(){return 0;}
void _InputInitialiseJoys(){ControlsManager.InitDefaultControlConfigJoyPad(16);}
void HandleExit(){RsGlobal.quit=TRUE;}
void _psSelectScreenVM(RwInt32){}
void InitialiseLanguage(){}
RwBool _psSetVideoMode(RwInt32,RwInt32){return TRUE;}
RwChar** _psGetVideoModeList(){static char mode[]="320x240 CPU";static char* modes[]={mode};return modes;}
RwInt32 _psGetNumVideModes(){return 1;}
}
