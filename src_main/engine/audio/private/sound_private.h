//=============================================================================
//
//=============================================================================

#include "basetypes.h"
#include "snd_fixedint.h"

#ifndef SOUND_PRIVATE_H
#define SOUND_PRIVATE_H
#pragma once

// Forward declarations
struct portable_samplepair_t;
struct channel_t;
typedef int SoundSource;
class CAudioSource;
struct channel_t;
class CSfxTable;
class IAudioDevice;

// ====================================================================

#define		SAMPLE_16BIT_SHIFT		1

void S_Startup (void);
void S_FlushSoundData(int rate);

CAudioSource *S_LoadSound( CSfxTable *s, channel_t *ch );
void S_TouchSound (char *sample);
CSfxTable *S_FindName (const char *name, int *pfInCache);

// spatializes a channel
void SND_Spatialize(channel_t *ch);

// shutdown the DMA xfer.
void SNDDMA_Shutdown(void);

// ====================================================================
// User-setable variables
// ====================================================================

extern int paintedtime;

extern bool	snd_initialized;

void S_LocalSound (char *s);

void SND_InitScaletable (void);
void SNDDMA_Submit(void);

void S_AmbientOff (void);
void S_AmbientOn (void);
void S_FreeChannel(channel_t *ch);

//=============================================================================

// UNDONE: Move this global?
extern IAudioDevice *g_AudioDevice;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void S_TransferStereo16 (int end);
void S_TransferPaintBuffer(int endtime);
void S_SpatializeChannel( int volume[4], int master_vol, float gain, float dotRight );
void S_MixBufferUpsample2x( int count, portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem, int cfltmem, int filtertype );

extern void Mix8MonoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount );
extern void Mix8StereoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount );
extern void Mix16MonoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount );
extern void Mix16StereoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount );

extern void SND_MoveMouth8(channel_t *ch, CAudioSource *pSource, int count);
extern void SND_CloseMouth(channel_t *ch);
extern void SND_InitMouth(int entnum, int entchannel);
extern void SND_UpdateMouth( channel_t *pChannel );
extern void SND_ClearMouth( channel_t *pChannel );
extern bool SND_IsMouth( channel_t *pChannel );

void MIX_PaintChannels(int endtime);
// Play a big of zeroed out sound
void MIX_PaintNullChannels(int endtime);

bool AllocDsps( void );
void FreeDsps( void );
void CheckNewDspPresets( void );

void DSP_Process( int idsp, portable_samplepair_t *pbfront, portable_samplepair_t *pbrear, int sampleCount );
void DSP_ClearState();

extern int idsp_room;
extern int idsp_water;
extern int idsp_player;
extern int idsp_facingaway;

extern float g_DuckScale;

// Legacy DSP Routines

void SX_Init (void);
void SX_Free (void);
void SX_ReloadRoomFX();
void SX_RoomFX(int endtime, int fFilter, int fTimefx);

// DSP Routines

void DSP_InitAll(void);
void DSP_FreeAll(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif			// SOUND_PRIVATE_H
