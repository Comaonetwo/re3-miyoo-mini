#!/usr/bin/env python3
"""Create an isolated, reproducibly patched CPU-platform tree; never alter upstream."""
import argparse,pathlib,shutil,subprocess,hashlib,json
p=argparse.ArgumentParser();p.add_argument('source',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args()
source=a.source.resolve();out=a.output.resolve()
revision=subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip()
if revision!='3233ffe1c4b99e8efb4c41c6794b4fce880cf503':raise ValueError('wrong pinned re3')
if out==source or source in out.parents:raise ValueError('output must be separate from upstream')
if out.exists():
 if not (out/'cpu-patch-manifest.json').exists():raise ValueError('refusing non-managed output')
 shutil.rmtree(out)
shutil.copytree(source/'src',out/'src');changed=[]
def edit(name,fn):
 f=out/'src'/name;s=f.read_text();t=fn(s)
 if t==s:raise ValueError('patch did not change '+name)
 f.write_text(t);changed.append(dict(file=name,original_sha256=hashlib.sha256(s.encode()).hexdigest(),patched_sha256=hashlib.sha256(t.encode()).hexdigest()))
def replace(s,old,new):
 if s.count(old)!=1:raise ValueError('patch anchor count '+str(s.count(old))+': '+old[:90])
 return s.replace(old,new)
def function_guard(s,signature,code):
 start=s.index(signature);brace=s.index('{',start);depth=1;i=brace+1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[:brace+1]+'\n#ifdef MIYOO_CPU\n'+code+'\n#else\n'+s[brace+1:i-1]+'\n#endif\n'+s[i-1:]
edit('core/ControllerConfig.h',lambda s: replace(replace(s,'#if defined RW_GL3\n\tGlfwJoyState','#if defined MIYOO_CPU\n\tstruct CpuJoyState { bool buttons[MAX_BUTTONS]; };\n\tCpuJoyState m_OldState{}, m_NewState{};\n#elif defined RW_GL3\n\tGlfwJoyState'),'#ifndef RW_GL3\nVALIDATE_SIZE','#if !defined RW_GL3 && !defined MIYOO_CPU\nVALIDATE_SIZE'))
def controller(s):
 s='#include "re3_cpu_host.hpp"\n'+s
 s=function_guard(s,'void CControllerConfigManager::GetWideStringOfCommandKeys(uint16 action, wchar *text, uint16 leight)', 'if(!text||!leight)return;\nconst char* hint=cpuControlHint(action);unsigned i=0;for(;hint[i]&&i+1<leight;++i)text[i]=static_cast<unsigned char>(hint[i]);text[i]=0;')
 s=function_guard(s,'int32 CControllerConfigManager::GetJoyButtonJustDown()', 'for(int i=0;i<MAX_BUTTONS;++i) if(m_NewState.buttons[i]&&!m_OldState.buttons[i]) return i+1;\nreturn 0;')
 return function_guard(s,'void CControllerConfigManager::UpdateJoyButtonState(int32 padnumber)','for(int i=0;i<MAX_BUTTONS;++i) m_aButtonStates[i]=m_NewState.buttons[i];')
edit('core/ControllerConfig.cpp',controller)
def pad(s):
 s=function_guard(s,'CMouseControllerState CMousePointerStateHelper::GetMouseSetUp()','return CMouseControllerState();')
 return function_guard(s,'void CPad::UpdateMouse()','PCTempMouseControllerState.Clear();\nPCTempMouseControllerState.x=PCTempMouseControllerState.y=0;\nOldMouseControllerState=NewMouseControllerState;\nNewMouseControllerState=PCTempMouseControllerState;')
edit('core/Pad.cpp',pad)
def player_aim(s):
 s='#include \"re3_cpu_host.hpp\"\n'+s
 s=replace(s,'CPlayerPed::ProcessPlayerWeapon(CPad *padUsed)\n{','CPlayerPed::ProcessPlayerWeapon(CPad *padUsed)\n{\n\t// Acquire before SetAttack, so the first shot uses the native target too.\n\tstatic uint32 cpuNextAimSearch = 0;\n\tbool cpuSearch = padUsed->TargetJustDown() || (padUsed->GetWeapon() && int32(CTimer::GetTimeInMilliseconds() - cpuNextAimSearch) >= 0);\n\tif (!m_pPointGunAt && cpuSearch && padUsed->GetTarget() && !CCamera::m_bUseMouse3rdPerson && !bInVehicle && m_nSelectedWepSlot == m_currentWeapon && m_nMoveState != PEDMOVE_SPRINT) {\n\t\tauto type = GetWeapon()->m_eWeaponType;\n\t\tif (type == WEAPONTYPE_COLT45 || type == WEAPONTYPE_UZI || type == WEAPONTYPE_SHOTGUN || type == WEAPONTYPE_AK47 || type == WEAPONTYPE_M16) { cpuNextAimSearch = CTimer::GetTimeInMilliseconds() + 250; FindWeaponLockOnTarget(); }\n\t}')
 s=replace(s,'GetWeapon()->m_eWeaponType == WEAPONTYPE_SNIPERRIFLE || GetWeapon()->m_eWeaponType == WEAPONTYPE_M16) {','GetWeapon()->m_eWeaponType == WEAPONTYPE_SNIPERRIFLE || (GetWeapon()->m_eWeaponType == WEAPONTYPE_M16 && !cpuAutomaticAim())) {')
 return s
edit('peds/PlayerPed.cpp',player_aim)
def frontend(s):
 s='#include \"re3_cpu_host.hpp\"\n'+replace(s,'CMenuManager::Draw()\n{','CMenuManager::Draw()\n{\n\tCpuProfileScope cpuPhase(3);\n\tcpuFullResolutionMenu();')
 normal='CMenuManager::DrawFrontEndNormal()\n{'
 if s.count(normal)!=2:raise ValueError('normal menu entry anchors')
 s=s.replace(normal,normal+'\n\tCpuProfileScope cpuPhase(3);\n\tcpuMenuBackground();')
 s=replace(s,'\t// GTA LOGO','\tcpuFullResolutionMenu();\n\t// GTA LOGO')
 s=function_guard(s,'CMenuManager::CentreMousePointer()','// No mouse exists on the handheld CPU platform.')
 return replace(s,'#if !defined RW_GL3\n\t\t\t\t\t\tif (AllValidWinJoys','#if defined MIYOO_CPU\n                        ControlsManager.InitDefaultControlConfigJoyPad(16);\n#elif !defined RW_GL3\n\t\t\t\t\t\tif (AllValidWinJoys')
edit('core/Frontend.cpp',frontend)
edit('skel/crossplatform.h',lambda s:s+'\n#ifdef MIYOO_CPU\nvoid CapturePad(RwInt32 padID);\n#endif\n')
edit('core/config.h',lambda s:s+'''\n// The software platform deliberately omits optional GPU/debug effects.\n#ifdef MIYOO_CPU\n#undef EXTENDED_PIPELINES\n#undef NEW_RENDERER\n#undef USE_TXD_CDIMAGE\n#undef NO_ISLAND_LOADING\n#undef EXTENDED_COLOURFILTER\n#undef SCREEN_DROPLETS\n#if defined(__linux__)\n#define USE_UNNAMED_SEM\n#endif\n#endif\n''')
edit('fakerw/fake.cpp',lambda s: replace(function_guard(s,'RwBool rwNativeTextureHackRead(RwStream *stream, RwTexture **tex, RwInt32 size)','return cpuNativeTextureRead(stream,tex);'),'// hack for reading native textures','extern bool cpuNativeTextureRead(rw::Stream*,rw::Texture**);\n// hack for reading native textures'))
def music(s):
 s=replace(s,'\tint pos;','\tuint32 pos;')
 return replace(s,'pos = pTm->tm_yday','pos = uint32(pTm->tm_yday)')
edit('audio/MusicManager.cpp',music)
def audio(s):
 s='#include "re3_audio_host.hpp"\n'+s
 entries=[
 ('cSampleManager::SetEffectsMasterVolume(uint8 nVolume)','cpuSfxMaster(nVolume,false);'),
 ('cSampleManager::SetEffectsFadeVolume(uint8 nVolume)','cpuSfxMaster(nVolume,true);'),
 ('cSampleManager::InitialiseSampleBanks(void)','return cpuSfxAvailable();'),
 ('cSampleManager::LoadSampleBank(uint8 nBank)','return nBank<MAX_SFX_BANKS&&cpuSfxAvailable();'),
 ('cSampleManager::IsSampleBankLoaded(uint8 nBank)','return nBank<MAX_SFX_BANKS&&cpuSfxAvailable()?LOADING_STATUS_LOADED:LOADING_STATUS_NOT_LOADED;'),
 ('cSampleManager::IsPedCommentLoaded(uint32 nComment)','return cpuSfxInfo(nComment,4)==1?LOADING_STATUS_LOADED:LOADING_STATUS_NOT_LOADED;'),
 ('cSampleManager::_GetPedCommentSlot(uint32 nComment)','return cpuSfxInfo(nComment,4)==1?0:-1;'),
 ('cSampleManager::LoadPedComment(uint32 nComment)','return cpuSfxInfo(nComment,4)==1;'),
 ('cSampleManager::GetBankContainingSound(uint32 offset)','return offset>=BankStartOffset[SFX_BANK_PED_COMMENTS]?SFX_BANK_PED_COMMENTS:offset<TOTAL_AUDIO_SAMPLES?SFX_BANK_0:INVALID_SFX_BANK;'),
 ('cSampleManager::GetSampleBaseFrequency(uint32 nSample)','return cpuSfxInfo(nSample,0);'),
 ('cSampleManager::GetSampleLoopStartOffset(uint32 nSample)','return cpuSfxInfo(nSample,1);'),
 ('cSampleManager::GetSampleLoopEndOffset(uint32 nSample)','return int32(cpuSfxInfo(nSample,2));'),
 ('cSampleManager::GetSampleLength(uint32 nSample)','return cpuSfxInfo(nSample,3);'),
 ('cSampleManager::InitialiseChannel(uint32 nChannel, uint32 nSfx, uint8 nBank)','return cpuSfxInitChannel(nChannel,nSfx);'),
 ('cSampleManager::SetChannelVolume(uint32 nChannel, uint32 nVolume)','cpuSfxChannelVolume(nChannel,nVolume);'),
 ('cSampleManager::SetChannelEmittingVolume(uint32 nChannel, uint32 nVolume)','cpuSfxChannelVolume(nChannel,nVolume);'),
 ('cSampleManager::SetChannelFrequency(uint32 nChannel, uint32 nFreq)','cpuSfxChannelRate(nChannel,nFreq);'),
 ('cSampleManager::SetChannelLoopPoints(uint32 nChannel, uint32 nLoopStart, int32 nLoopEnd)','cpuSfxLoop(nChannel,nLoopStart,nLoopEnd);'),
 ('cSampleManager::SetChannelLoopCount(uint32 nChannel, uint32 nLoopCount)','cpuSfxCount(nChannel,nLoopCount);'),
 ('cSampleManager::GetChannelUsedFlag(uint32 nChannel)','return cpuSfxPlaying(nChannel);'),
 ('cSampleManager::StartChannel(uint32 nChannel)','cpuSfxStart(nChannel);'),
 ('cSampleManager::StopChannel(uint32 nChannel)','cpuSfxStop(nChannel);'),
 ('cSampleManager::Initialise(void)','for(unsigned i=0;i<TOTAL_STREAMED_SOUNDS;++i) cpuAudioTrack(i,StreamedNameTable[i]); cpuAudioInit(); return TRUE;'),
 ('cSampleManager::Terminate(void)','cpuAudioTerminate();'),
 ('cSampleManager::SetMusicMasterVolume(uint8 nVolume)','cpuAudioMaster(nVolume,false);'),
 ('cSampleManager::SetMusicFadeVolume(uint8 nVolume)','cpuAudioMaster(nVolume,true);'),
 ('cSampleManager::PauseStream(bool8 nPauseFlag, uint8 nStream)','cpuAudioPause(nPauseFlag,nStream);'),
 ('cSampleManager::StartStreamedFile(uint8 nFile, uint32 nPos, uint8 nStream)','return cpuAudioStart(nFile,nPos,nStream);'),
 ('cSampleManager::StopStreamedFile(uint8 nStream)','cpuAudioStop(nStream);'),
 ('cSampleManager::GetStreamedFilePosition(uint8 nStream)','return cpuAudioPosition(nStream);'),
 ('cSampleManager::SetStreamedVolumeAndPan(uint8 nVolume, uint8 nPan, uint8 nEffectFlag, uint8 nStream)','cpuAudioVolume(nVolume,nStream);'),
 ('cSampleManager::GetStreamedFileLength(uint8 nStream)','unsigned length=cpuAudioLength(nStream); return length?length:1;'),
 ('cSampleManager::IsStreamPlaying(uint8 nStream)','return cpuAudioPlaying(nStream);'),
 ('cSampleManager::PreloadStreamedFile(uint8 nFile, uint8 nStream)','if(nStream<2) cpuPreloaded[nStream]=nFile;'),
 ('cSampleManager::StartPreloadedStreamedFile(uint8 nStream)','if(nStream<2) cpuAudioStart(cpuPreloaded[nStream],0,nStream);')]
 s=s.replace('cSampleManager SampleManager;', 'static unsigned cpuPreloaded[2]={};\ncSampleManager SampleManager;')
 for signature,code in entries:s=function_guard(s,signature,code)
 return s
edit('audio/sampman_null.cpp',audio)

def streaming(s):
 start=s.index('\tms_memoryAvailable = (_dwMemAvailPhys - 10*MB)/2;');end=s.index('\tdebug("Memory allocated to Streaming',start)
 return s[:start]+'#ifdef MIYOO_CPU\n\tms_memoryAvailable = 24*MB;\n\tdesiredNumVehiclesLoaded = 2;\n#else\n'+s[start:end]+'#endif\n'+s[end:]
def garage_streaming(s):
 s=streaming(s)
 s='#include "re3_cpu_garage.hpp"\n'+s
 s=replace(s,'((ms_aInfoForModel[id].m_flags & STREAMFLAGS_CANT_REMOVE) == 0 || GetAvailableVehicleSlot() == -1)', '(((ms_aInfoForModel[id].m_flags & STREAMFLAGS_CANT_REMOVE) == 0 && !cpuGarageModelNeeded(id)) || GetAvailableVehicleSlot() == -1)')
 s=replace(s,'CModelInfo::GetModelInfo(id)->GetNumRefs() == 0 &&\n\t\t   ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED)', 'CModelInfo::GetModelInfo(id)->GetNumRefs() == 0 && !cpuGarageModelNeeded(id) &&\n\t\t   ms_aInfoForModel[id].m_loadState == STREAMSTATE_LOADED)')
 s=replace(s,'(ms_aInfoForModel[id].m_flags & STREAMFLAGS_CANT_REMOVE) == 0 && CModelInfo::GetModelInfo(id)->GetNumRefs() == 0)', '(ms_aInfoForModel[id].m_flags & STREAMFLAGS_CANT_REMOVE) == 0 && CModelInfo::GetModelInfo(id)->GetNumRefs() == 0 && !cpuGarageModelNeeded(id))')
 s=replace(s,'if (CModelInfo::GetModelInfo(streamId)->GetNumRefs() == 0) {','if (CModelInfo::GetModelInfo(streamId)->GetNumRefs() == 0 && !cpuGarageModelNeeded(streamId)) {')
 return s
edit('core/Streaming.cpp',garage_streaming)
def garage_car(s):
 s='#include "re3_cpu_garage.hpp"\n'+s
 s=replace(s,'CStoredCar::RestoreCar()\n{','CStoredCar::RestoreCar()\n{\n\tcpuGarageRequestModel(m_nModelIndex);')
 start=s.index('CStoredCar::RestoreCar()');end=s.index('void CGarage::StoreAndRemoveCarsForThisHideout',start)
 body=s[start:end];body=replace(body,'return pVehicle;', 'cpuGarageModelRestored(m_nModelIndex);\n\treturn pVehicle;')
 return s[:start]+body+s[end:]
edit('control/Garages.cpp',garage_car)
edit('core/Game.cpp',lambda s:replace(s,'\t\t\t\tHandleExit();','\t\t\t\t// CPU platform: a failed load restarts rather than quitting Onion.'))
def storage(s):
 s='#include "save_header.hpp"\n'+s
 s=replace(s,'eLevelName level = LEVEL_GENERIC;','uint32 level = LEVEL_GENERIC;')
 s=replace(s,'\t\tif (blocksize > align4bytes(sizeof(work_buff)))\n\t\t\tblocksize = sizeof(work_buff) - sizeof(uint32);','\t\tif (blocksize < 0 || blocksize > align4bytes(sizeof(work_buff))) { CloseFile(file); return false; }')
 s=replace(s,'\t\tif (blocknum == 0)\n\t\t\tmemcpy(&level, work_buff+4, sizeof(level));','\t\tif (blocknum == 0 && !soft::saveLevel(work_buff, blocksize, level)) { CloseFile(file); return false; }')
 return replace(s,'m_LevelToLoad = level;','m_LevelToLoad = static_cast<eLevelName>(level);')
edit('save/GenericGameStorage.cpp',storage)

def weather_cpu(s):
 s='#include "re3_cpu_host.hpp"\n'+s
 call='RenderOneRainStreak(pos, dir, intensity, false, (pos - TheCamera.GetPosition()).Magnitude());'
 return replace(s,call,'if(cpuRainStreakVisible(unsigned(i))) '+call+' else if(!CTimer::GetIsPaused()){myrand();myrand();myrand();}')
edit('renderer/Weather.cpp',weather_cpu)

def particle_cpu(s):
 s='#include "re3_cpu_host.hpp"\n'+s
 s=replace(s,'void CParticle::Render()\n{','void CParticle::Render()\n{\n\tconst bool cpuReduced = cpuBeginParticles();')
 return replace(s,'bool canDraw = true;','bool canDraw = !cpuReduced || cpuParticleVisible(particle);')
edit('renderer/Particle.cpp',particle_cpu)


edit('renderer/Timecycle.h',lambda s:replace(replace(s,'class CTimeCycle','float cpuFarClip();\nclass CTimeCycle'),'return m_fCurrentFarClip;','return Min(m_fCurrentFarClip,cpuFarClip());'))
def renderer_order(s):
 s=replace(replace(s,'&aBlockedRanges[i-1]','i>0 ? &aBlockedRanges[i-1] : nil'),'&aBlockedRanges[i+1]','i<15 ? &aBlockedRanges[i+1] : nil')
 s='#include "re3_cpu_draw_order.hpp"\n#include "re3_cpu_world_fade.hpp"\n'+s
 s='#include "re3_cpu_detail.hpp"\n'+s
 s=replace(s,'CRenderer::RenderOneNonRoad(CEntity *e)\n{','CRenderer::RenderOneNonRoad(CEntity *e)\n{\n CpuDetailScope cpuDetail(e);if(cpuDetail.skip)return;')
 if s.count('\tmi->IncreaseAlpha();')!=2:raise ValueError('world alpha anchors')
 s=s.replace('\tmi->IncreaseAlpha();','\tcpuIncreaseWorldAlpha(ent,mi);')
 start=s.index('CRenderer::RenderEverythingBarRoads(void)');end=s.index('\n}\n',start);part=s[start:end]
 part=replace(part,'\tgSortedVehiclesAndPeds.Clear();','\tgSortedVehiclesAndPeds.Clear();\n\tCpuOpaqueOrder cpuOrder(ms_aVisibleEntityPtrs,ms_nNoOfVisibleEntities);')
 return s[:start]+part+s[end:]
edit('renderer/Renderer.cpp',renderer_order)
def main_cpu(s):
 s='#include "re3_cpu_host.hpp"\n'+s.replace('Render2dStuff(void)\n{','Render2dStuff(void)\n{\n\tcpuFullResolutionHud();').replace('CShadows::RenderStaticShadows();','if(cpuShadowsEnabled()) CShadows::RenderStaticShadows();').replace('CShadows::RenderStoredShadows();','if(cpuShadowsEnabled()) CShadows::RenderStoredShadows();')
 if s.count('CGame::Process();')!=3:raise ValueError('game profiling anchors')
 s=s.replace('CGame::Process();','cpuProfileGameProcess();')
 for call,phase in [('CRenderer::ConstructRenderList();',0),('RenderEffects();',1),('TheCamera.RenderMotionBlur();',2),('Render2dStuff();',3),('Render2dStuffAfterFade();',3)]:
  if call not in s:raise ValueError('phase profiling anchor '+call)
  s=s.replace(call,'{CpuProfileScope cpuPhase('+str(phase)+');'+call+'}')

 # Coarse detail scopes read the clock only on selected diagnostic frames.
 def detail_scopes(source,signature,calls):
  begin=source.index('\n'+signature+'(void)\n{');brace=source.index('{',begin);depth=1;end=brace+1
  while depth:
   if source[end]=='{':depth+=1
   elif source[end]=='}':depth-=1
   end+=1
  body=source[brace:end]
  for phase,call in calls:
   if body.count(call)!=1:raise ValueError('detail phase anchor '+call)
   body=body.replace(call,'{CpuProfileScope detail('+str(phase)+');'+call+'}')
  return source[:brace]+body+source[end:]
 s=detail_scopes(s,"RenderScene",[(4, 'CClouds::Render();'), (5, 'DoRWRenderHorizon();'), (6, 'CRenderer::RenderRoads();'), (7, 'CCoronas::RenderReflections();'), (8, 'CRenderer::RenderEverythingBarRoads();'), (9, 'CRenderer::RenderBoats();'), (10, 'CWaterLevel::RenderWater();'), (11, 'CRenderer::RenderFadingInEntities();'), (12, 'CRenderer::RenderVehiclesButNotBoats();'), (13, 'CWeather::RenderRainStreaks();')])
 s=detail_scopes(s,"RenderEffects",[(14, 'CGlass::Render();'), (15, 'CWaterCannons::Render();'), (16, 'CSpecialFX::Render();'), (17, 'CShadows::RenderStaticShadows();'), (18, 'CShadows::RenderStoredShadows();'), (19, 'CSkidmarks::Render();'), (20, 'CAntennas::Render();'), (21, 'CRubbish::Render();'), (22, 'CCoronas::Render();'), (23, 'CParticle::Render();'), (24, 'CPacManPickups::Render();'), (25, 'CWeaponEffects::Render();'), (26, 'CPointLights::RenderFogEffect();'), (27, 'CMovingThings::Render();'), (28, 'CRenderer::RenderFirstPersonVehicle();')])
 s=replace(s,"CClouds::RenderBackground(TopRed, TopGreen, TopBlue, BottomRed, BottomGreen, BottomBlue, Alpha);","{CpuProfileScope detail(29);CClouds::RenderBackground(TopRed, TopGreen, TopBlue, BottomRed, BottomGreen, BottomBlue, Alpha);}")
 start=s.index('\nRenderScene(void)\n{');brace=s.index('{',start);depth=1;i=brace+1
 while depth:
  if s[i]=='{':depth+=1
  elif s[i]=='}':depth-=1
  i+=1
 return s[:brace+1]+'\n\tcpuProfileWorldStart();'+s[brace+1:i-1]+'\n\tcpuProfileWorldEnd();\n'+s[i-1:]
edit('core/main.cpp',main_cpu)
edit('control/Replay.cpp',lambda s:function_guard(replace(s,'pp->heading = 128.0f / PI * ped->m_fRotationCur;','const uint8 heading = uint8(int(128.0f / PI * ped->m_fRotationCur));\n\tmemcpy(&pp->heading, &heading, sizeof(heading));'),'void CReplay::Update(void)','// Replay recording is disabled on the first CPU target.\nreturn;'))
from cdstream_patch import patch_cdstream
def cdstream(s):return patch_cdstream(s,replace)
edit('core/CdStreamPosix.cpp',cdstream)
def audio_fade(s):
 s='#include "audio_fade.hpp"\n'+s
 old='sample.m_nVolumeChange = sample.m_nVolume / sample.m_nFramesToPlay;'
 if s.count(old)!=2:raise ValueError('audio fade anchors')
 s=s.replace(old,'sample.m_nVolumeChange = soft::audioFadeStep(sample.m_nVolume, sample.m_nFramesToPlay);')
 return replace(s,'sample.m_nEmittingVolumeChange = sample.m_nEmittingVolume / sample.m_nFramesToPlay;','sample.m_nEmittingVolumeChange = soft::audioFadeStep(sample.m_nEmittingVolume, sample.m_nFramesToPlay);')
edit('audio/AudioManager.cpp',audio_fade)
def radar(s):
 s='#include "re3_cpu_host.hpp"\n'+s
 s=replace(s,'\tDrawRadarMask();','\tDrawRadarMask();\n\tCVector2D cpuCornerA, cpuCornerB;\n\tTransformRadarPointToScreenSpace(cpuCornerA, CVector2D(-1.0f, 1.0f));\n\tTransformRadarPointToScreenSpace(cpuCornerB, CVector2D(1.0f, -1.0f));\n\tcpuBeginRadar(cpuCornerA.x, cpuCornerA.y, cpuCornerB.x, cpuCornerB.y);')
 return replace(s,'\tDrawRadarSection(x + 1, y + 1);','\tDrawRadarSection(x + 1, y + 1);\n\tcpuEndRadar();')
edit('core/Radar.cpp',radar)
def vehicle_lod_header(s):
 s=replace(s,'\t\tint alpha;\n\t};','\t\tint alpha;\n#ifdef MIYOO_CPU\n\t\tbool cpuVehicleLod;\n\t\tbool cpuPedLod;\n#endif\n\t};')
 return replace(s,'\tstatic void SetClumpModelInfo(', '\tstatic void CpuSetPedLod(RpClump*);\n\tstatic float CpuPedLodDistance(RpClump*);\n\tstatic void CpuSetVehicleLod(RpClump*);\n\tstatic float CpuVehicleLodDistance(RpClump*);\n\tstatic void SetClumpModelInfo(')
edit('rw/VisibilityPlugins.h',vehicle_lod_header)
def vehicle_lod_source(s):
 s='#include "re3_cpu_world_fade.hpp"\n'+s
 s=replace(s,'\t\tmi = (CSimpleModelInfo *)CModelInfo::GetModelInfo(e->GetModelIndex());','\t\tCpuProfileScope cpuFadeKind(e->IsBuilding()?30:31);\n\t\tmi = (CSimpleModelInfo *)CModelInfo::GetModelInfo(e->GetModelIndex());')
 s=replace(s,'\t\t\tRenderFadingAtomic((RpAtomic*)e->m_rwObject, node->item.sort);','\t\t\tCpuWorldFadeScope cpuFadeScope(e->IsBuilding() && !e->bIsBIGBuilding);\n\t\t\tRenderFadingAtomic((RpAtomic*)e->m_rwObject, node->item.sort);')
 s=replace(s,'\t\talpha = mi->m_alpha * fadefactor;','\t\talpha = mi->m_alpha * fadefactor;\n\t\talpha = cpuWorldFadeAlpha(atomic,alpha);')
 s='#include "re3_cpu_host.hpp"\n#include "vehicle_lod.hpp"\n'+s
 s=replace(s,'#include "common.h"','#include "common.h"\n#include "CutsceneMgr.h"')
 for signature in ['CVisibilityPlugins::RenderVehicleHiDetailCB(RpAtomic *atomic)','CVisibilityPlugins::RenderVehicleHiDetailAlphaCB(RpAtomic *atomic)','CVisibilityPlugins::RenderVehicleReallyLowDetailCB(RpAtomic *atomic)']:
  start=s.index(signature);end=s.index('\n}\n',start);part=s[start:end];part=replace(part,'ms_vehicleLod0Dist','CpuVehicleLodDistance(RpAtomicGetClump(atomic))');s=s[:start]+part+s[end:]
 for signature in ['CVisibilityPlugins::RenderPedHiDetailCB(RpAtomic *atomic)','CVisibilityPlugins::RenderPedLowDetailCB(RpAtomic *atomic)']:
  start=s.index(signature);end=s.index('\n}\n',start);part=s[start:end];part=replace(part,'ms_pedLod0Dist','CpuPedLodDistance(clump)');s=s[:start]+part+s[end:]
 start=s.index('CVisibilityPlugins::RenderPedLowDetailCB(RpAtomic *atomic)');end=s.index('\n}\n',start);part=s[start:end];part=replace(part,'if(dist >= CpuPedLodDistance(clump)){','if(dist >= CpuPedLodDistance(clump)){\n\t\tif(dist < ms_pedLod0Dist) cpuRecordPedLod();');s=s[:start]+part+s[end:]
 s=replace(s,'\text->alpha = 0xFF;','\text->alpha = 0xFF;\n#ifdef MIYOO_CPU\n\text->cpuVehicleLod = false;\n\text->cpuPedLod = false;\n#endif')
 s=replace(s,'\tCLUMPEXT(dst)->visibilityCB = CLUMPEXT(src)->visibilityCB;','\tCLUMPEXT(dst)->visibilityCB = CLUMPEXT(src)->visibilityCB;\n#ifdef MIYOO_CPU\n\tCLUMPEXT(dst)->cpuVehicleLod = CLUMPEXT(src)->cpuVehicleLod;\n\tCLUMPEXT(dst)->cpuPedLod = CLUMPEXT(src)->cpuPedLod;\n#endif')
 s+='\nvoid CVisibilityPlugins::CpuSetVehicleLod(RpClump* clump){ CLUMPEXT(clump)->cpuVehicleLod = true; }\nfloat CVisibilityPlugins::CpuVehicleLodDistance(RpClump* clump){\n static const bool allowed = !getenv("GTA3_VEHICLE_LOD") || strcmp(getenv("GTA3_VEHICLE_LOD"),"0");\n return allowed && CLUMPEXT(clump)->cpuVehicleLod ? ms_vehicleLod0Dist*cpuVehicleLodScale() : ms_vehicleLod0Dist;\n}\n'
 s+='\nvoid CVisibilityPlugins::CpuSetPedLod(RpClump* clump){CLUMPEXT(clump)->cpuPedLod=true;}\nfloat CVisibilityPlugins::CpuPedLodDistance(RpClump* clump){\n static const bool allowed=!getenv("GTA3_PED_LOD")||strcmp(getenv("GTA3_PED_LOD"),"0");\n return allowed&&CLUMPEXT(clump)->cpuPedLod&&!CCutsceneMgr::IsRunning()&&!CCutsceneMgr::IsCutsceneProcessing()?ms_pedLod0Dist*cpuPedLodScale():ms_pedLod0Dist;\n}\n'
 # CoreSettings declaration is lightweight, independent of renderer implementation.
 # Frame-snapshotted factor comes from re3_cpu_host; no shared settings read here.
 return s
edit('rw/VisibilityPlugins.cpp',vehicle_lod_source)
def vehicle_lod_model(s):
 return replace(s,'\t}else if(strstr(name, "_vlo"))\n\t\tCVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB);','\t}else if(strstr(name, "_vlo")){\n\t\tCVisibilityPlugins::SetAtomicRenderCallback(atomic, CVisibilityPlugins::RenderVehicleReallyLowDetailCB);\n\t\tauto geometry=RpAtomicGetGeometry(atomic);\n\t\tif(geometry && geometry->numVertices>0 && geometry->numTriangles>0) CVisibilityPlugins::CpuSetVehicleLod(clump);\n\t}')
edit('modelinfo/VehicleModelInfo.cpp',vehicle_lod_model)

def ped_lod_model(s):
 # Eligibility requires every original LOD limb to be attached and nonempty.
 start=s.index('CPedModelInfo::SetLowDetailClump');end=s.index('\n}\n',start);part=s[start:end]
 part=replace(part,'\tnumAtm = 0;', '\tunsigned cpuAttached=0;\n\tnumAtm = 0;')
 part=replace(part,'\t\t\tRpClumpAddAtomic(m_clump, atomics[i]);','\t\t\tRpClumpAddAtomic(m_clump, atomics[i]);\n\t\t\tauto geometry=RpAtomicGetGeometry(atomics[i]);\n\t\t\tif(geometry&&geometry->numVertices>0&&geometry->numTriangles>0)++cpuAttached;')
 part+='\n\tif(numLodAtm>0&&cpuAttached==unsigned(numLodAtm)&&strcmp(GetModelName(),"player")\n#ifdef PED_SKIN\n\t   && !IsClumpSkinned(m_clump)\n#endif\n\t  ) CVisibilityPlugins::CpuSetPedLod(m_clump);'
 return s[:start]+part+s[end:]
edit('modelinfo/PedModelInfo.cpp',ped_lod_model)

charset=(source/'vendor/librw/src/charset.cpp').read_text()
charset=replace(charset,'#ifndef RW_NULL','#if !defined(RW_NULL) || defined(MIYOO_CPU)')
charset=replace(charset,'#include \"gl/rwgl3.h\"','#include \"gl/rwgl3.h\"\n#include \"rw_soft_vertices.hpp\"')
(out/'cpu_charset.cpp').write_text(charset)
(out/'cpu-patch-manifest.json').write_text(json.dumps(dict(re3=revision,changes=changed),indent=2)+'\n')
print('Prepared isolated CPU tree:',out,'patched',len(changed),'files')
