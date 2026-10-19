#pragma once
#include <vector>
#include <cstdint>
// Process host keeps display/input responsive while the game kernel loads.
uint32_t cpuHostButtons();
void cpuHostTestButtons(uint32_t bits);
void cpuHostStage(unsigned stage,const char* label);
void cpuHostPresent(const std::vector<uint16_t>& pixels);
int cpuKernel(int argc,char** argv);
int cpuConsole(int argc,char** argv);

uint32_t cpuHostSettings();
void cpuHostTestSettings(uint32_t bits);
bool cpuHostPaused();
float cpuFarClip();
void cpuDynamicFarClip(float distance);
bool cpuShadowsEnabled();

void cpuFullResolutionHud();

void cpuProfileGameProcess();
void cpuProfileWorldStart();
void cpuProfileWorldEnd();

// Timers are per phase, never per triangle/pixel.
void cpuPhaseStart(unsigned phase);
void cpuPhaseEnd(unsigned phase);
struct CpuProfileScope {unsigned phase;explicit CpuProfileScope(unsigned p):phase(p){cpuPhaseStart(p);}~CpuProfileScope(){cpuPhaseEnd(phase);}};

void cpuFullResolutionMenu();
void cpuMenuBackground();

bool cpuAutomaticAim();

void cpuBeginRadar(float left,float top,float right,float bottom);
void cpuEndRadar();

bool cpuRainStreakVisible(unsigned index);

// One immutable LOD factor per rendered frame / frozen diagnostic pass.
float cpuVehicleLodScale();

float cpuPedLodScale();
void cpuRecordPedLod();

class CParticle;
bool cpuBeginParticles();
bool cpuParticleVisible(const CParticle*);
void cpuParticleReductionOverride(int);
bool cpuParticleReducedActive();
unsigned cpuParticleCandidates();
unsigned cpuParticleSkipped();

const char* cpuControlHint(unsigned action);
void cpuMiyooControlsTest();
