#pragma once
#include <stdint.h>
/* Fixed 32-bit ABI, one game writer and one audio writer per field. */
#define CPU_AUDIO_RATE 22050
#define CPU_AUDIO_STREAMS 2
#define CPU_AUDIO_VOICES 32
#define CPU_AUDIO_ABI 0x1401u
typedef struct {uint32_t serial,sample,rate,volume,loops,start,end,playing,ack;} CpuAudioVoice;
typedef struct {uint32_t serial,track,seek,paused,volume,position,playing,ack;} CpuAudioStream;
typedef struct {uint32_t ready,quit,menuPaused,master,fade;CpuAudioStream stream[CPU_AUDIO_STREAMS];uint32_t abi,effects,effectsFade;CpuAudioVoice voice[CPU_AUDIO_VOICES];} CpuAudioShared;
