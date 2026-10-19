#pragma once
void cpuAudioSetRoot();
bool cpuAudioInit();
void cpuAudioTerminate();
void cpuAudioTrack(unsigned id,const char* path);
bool cpuAudioStart(unsigned track,unsigned position,unsigned stream);
void cpuAudioStop(unsigned stream);
void cpuAudioPause(bool pause,unsigned stream);
void cpuAudioMenuPause(bool pause);
void cpuAudioVolume(unsigned volume,unsigned stream);
void cpuAudioMaster(unsigned volume,bool fade);
unsigned cpuAudioPosition(unsigned stream);
unsigned cpuAudioLength(unsigned track);
bool cpuAudioPlaying(unsigned stream);

unsigned cpuSfxInfo(unsigned sample,unsigned field);
bool cpuSfxAvailable();
bool cpuSfxInitChannel(unsigned channel,unsigned sample);
void cpuSfxChannelVolume(unsigned channel,unsigned volume);
void cpuSfxChannelRate(unsigned channel,unsigned rate);
void cpuSfxLoop(unsigned channel,unsigned start,int end);
void cpuSfxCount(unsigned channel,unsigned count);
void cpuSfxStart(unsigned channel);
void cpuSfxStop(unsigned channel);
bool cpuSfxPlaying(unsigned channel);
void cpuSfxMaster(unsigned volume,bool fade);
