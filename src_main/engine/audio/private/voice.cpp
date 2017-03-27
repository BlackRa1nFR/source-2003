/*==============================================================================
   Copyright (c) Valve, LLC. 1999
   All Rights Reserved.  Proprietary.
--------------------------------------------------------------------------------
   Author : DSpeyrer
   Purpose: 
--------------------------------------------------------------------------------
 * $Log:                      $
 * 
 * $NoKeywords:
==============================================================================*/

#include <windows.h>
#include <math.h>
#include <mmsystem.h>
#include <stdio.h>
#include "tier0/dbg.h"
#include "convar.h"
#include "CircularBuffer.h"
#include "voice.h"
#include "voice_wavefile.h"
#include "voice_sound_engine_interface.h"
#include "sound.h"
#include "r_efx.h"
#include "cdll_int.h"
#include "voice_gain.h"
#include "voice_mixer_controls.h"

#include "SoundService.h"
#include "ivoicerecord.h"
#include "ivoicecodec.h"

#include "filesystem.h"
#include "../../FileSystem_Engine.h"


void Voice_EndChannel( int iChannel );


// Special entity index used for tweak mode.
#define TWEAKMODE_ENTITYINDEX				-500

// Special channel index passed to Voice_AddIncomingData when in tweak mode.
#define TWEAKMODE_CHANNELINDEX				-100


// How long does the sign stay above someone's head when they talk?
#define SPARK_TIME		0.2

// How long a voice channel has to be inactive before we free it.
#define DIE_COUNTDOWN	0.5

#define VOICE_RECEIVE_BUFFER_SIZE			(VOICE_OUTPUT_SAMPLE_RATE * BYTES_PER_SAMPLE)

#define TIME_PADDING						0.2f	// Time between receiving the first voice packet and actually starting
													// to play the sound. This accounts for frametime differences on the clients
													// and the server.

#define LOCALPLAYERTALKING_TIMEOUT			0.2f	// How long it takes for the client to decide the server isn't sending acks
													// of voice data back.


// The format we sample voice in.
WAVEFORMATEX g_VoiceSampleFormat =
{
	WAVE_FORMAT_PCM,		// wFormatTag
	1,						// nChannels
	8000,					// nSamplesPerSec
	16000,					// nAvgBytesPerSec
	2,						// nBlockAlign
	16,						// wBitsPerSample
	sizeof(WAVEFORMATEX)	// cbSize
};


ConVar voice_dsound( "voice_dsound", "0" ); 
ConVar voice_avggain( "voice_avggain", "0.5" );
ConVar voice_maxgain( "voice_maxgain", "5" );
ConVar voice_scale( "voice_scale", "1", FCVAR_ARCHIVE );

ConVar voice_loopback( "voice_loopback", "0" );
ConVar voice_fadeouttime( "voice_fadeouttime", "0.1" );	// It fades to no sound at the tail end of your voice data when you release the key.

// Debugging cvars.
ConVar voice_profile( "voice_profile", "0" );
ConVar voice_showchannels( "voice_showchannels", "0" );	// 1 = list channels
															// 2 = show timing info, etc
ConVar voice_showincoming( "voice_showincoming", "0" );	// show incoming voice data

ConVar voice_enable( "voice_enable", "1", FCVAR_ARCHIVE );		// Globally enable or disable voice.

// Have it force your mixer control settings so waveIn comes from the microphone. 
// CD rippers change your waveIn to come from the CD drive
ConVar voice_forcemicrecord( "voice_forcemicrecord", "1", FCVAR_ARCHIVE );

int g_nVoiceFadeSamples		= 1;							// Calculated each frame from the cvar.
float g_VoiceFadeMul		= 1;							// 1 / (g_nVoiceFadeSamples - 1).

// While in tweak mode, you can't hear anything anyone else is saying, and your own voice data
// goes directly to the speakers.
bool g_bInTweakMode = false;

bool g_bVoiceAtLeastPartiallyInitted = false;


IMixerControls *g_pMixerControls = NULL;


// Timing info for each frame.
static double	g_CompressTime = 0;
static double	g_DecompressTime = 0;
static double	g_GainTime = 0;
static double	g_UpsampleTime = 0;

class CVoiceTimer
{
public:
	inline void		Start()
	{
		if( voice_profile.GetInt() )
			m_StartTime = g_pSoundServices->GetRealTime();
	}
	
	inline void		End(double *out)
	{
		if( voice_profile.GetInt() )
			*out += g_pSoundServices->GetRealTime() - m_StartTime;
	}

	double			m_StartTime;
};


static bool		g_bLocalPlayerTalkingAck = false;
static float	g_LocalPlayerTalkingTimeout = 0;


CSysModule *g_hVoiceCodecDLL = 0;

// Voice recorder. Can be waveIn, DSound, or whatever.
static IVoiceRecord *g_pVoiceRecord = NULL;
static IVoiceCodec  *g_pEncodeCodec = NULL;

static bool			g_bVoiceRecording = false;	// Are we recording at the moment?

// Hacked functions to create the inputs and codecs..
extern IVoiceRecord*	CreateVoiceRecord_DSound(int nSamplesPerSec);

//
// Used for storing incoming voice data from an entity.
//
class CVoiceChannel
{
public:
									CVoiceChannel();

	// Called when someone speaks and a new voice channel is setup to hold the data.
	void							Init(int nEntity);

public:
	int								m_iEntity;		// Number of the entity speaking on this channel (index into cl_entities).
													// This is -1 when the channel is unused.

	
	CSizedCircularBuffer
		<VOICE_RECEIVE_BUFFER_SIZE>	m_Buffer;		// Circular buffer containing the voice data.

	// Used for upsampling..
	double							m_LastFraction;	// Fraction between m_LastSample and the next sample.
	short							m_LastSample;
	
	bool							m_bStarved;		// Set to true when the channel runs out of data for the mixer.
													// The channel is killed at that point.

	float							m_TimePad;		// Set to TIME_PADDING when the first voice packet comes in from a sender.
													// We add time padding (for frametime differences)
													// by waiting a certain amount of time before starting to output the sound.

	IVoiceCodec						*m_pVoiceCodec;	// Each channel gets is own IVoiceCodec instance so the codec can maintain state.

	CAutoGain						m_AutoGain;		// Gain we're applying to this channel

	CVoiceChannel					*m_pNext;
};


CVoiceChannel::CVoiceChannel()
{
	m_iEntity = -1;
	m_pVoiceCodec = NULL;
}

void CVoiceChannel::Init(int nEntity)
{
	m_iEntity = nEntity;
	m_bStarved = false;
	m_Buffer.Flush();
	m_TimePad = TIME_PADDING;
	m_LastSample = 0;
	m_LastFraction = 0.999;
	
	m_AutoGain.Reset( 128, voice_maxgain.GetFloat(), voice_avggain.GetFloat(), voice_scale.GetFloat() );
}



// Incoming voice channels.
CVoiceChannel g_VoiceChannels[VOICE_NUM_CHANNELS];


// These are used for recording the wave data into files for debugging.
#define MAX_WAVEFILEDATA_LEN	1024*1024
char *g_pUncompressedFileData = NULL;
int g_nUncompressedDataBytes = 0;
const char *g_pUncompressedDataFilename = NULL;

char *g_pDecompressedFileData = NULL;
int g_nDecompressedDataBytes = 0;
const char *g_pDecompressedDataFilename = NULL;

char *g_pMicInputFileData = NULL;
int g_nMicInputFileBytes = 0;
int g_CurMicInputFileByte = 0;
double g_MicStartTime;


inline void ApplyFadeToSamples(short *pSamples, int nSamples, int fadeOffset, float fadeMul)
{
	for(int i=0; i < nSamples; i++)
	{
		float percent = (i+fadeOffset) * fadeMul;
		pSamples[i] = (short)(pSamples[i] * (1 - percent));
	}
}


int Voice_GetOutputData(
	const int iChannel,			//! The voice channel it wants samples from.
	char *copyBufBytes,			//! The buffer to copy the samples into.
	const int copyBufSize,		//! Maximum size of copyBuf.
	const int samplePosition,	//! Which sample to start at.
	const int sampleCount		//! How many samples to get.
)
{
	CVoiceChannel *pChannel = &g_VoiceChannels[iChannel];	
	short *pCopyBuf = (short*)copyBufBytes;


	int maxOutSamples = copyBufSize / BYTES_PER_SAMPLE;

	// Find out how much we want and get it from the received data channel.	
	CCircularBuffer *pBuffer = &pChannel->m_Buffer;
	int nBytesToRead = pBuffer->GetReadAvailable();
	nBytesToRead = min(min(nBytesToRead, (int)maxOutSamples), sampleCount * BYTES_PER_SAMPLE);
	int nSamplesGotten = pBuffer->Read(pCopyBuf, nBytesToRead) / BYTES_PER_SAMPLE;

	// Are we at the end of the buffer's data? If so, fade data to silence so it doesn't clip.
	int readSamplesAvail = pBuffer->GetReadAvailable() / BYTES_PER_SAMPLE;
	if(readSamplesAvail < g_nVoiceFadeSamples)
	{
		int bufferFadeOffset = max((readSamplesAvail + nSamplesGotten) - g_nVoiceFadeSamples, 0);
		int globalFadeOffset = max(g_nVoiceFadeSamples - (readSamplesAvail + nSamplesGotten), 0);
		
		ApplyFadeToSamples(
			&pCopyBuf[bufferFadeOffset], 
			nSamplesGotten - bufferFadeOffset,
			globalFadeOffset,
			g_VoiceFadeMul);
	}
	
	// If there weren't enough bytes in the received data channel, pad it with zeros.
	if(nSamplesGotten < sampleCount)
	{
		int wantedSampleCount = min(sampleCount, maxOutSamples);
		memset(&pCopyBuf[nSamplesGotten], 0, (wantedSampleCount - nSamplesGotten) * BYTES_PER_SAMPLE);
		nSamplesGotten = wantedSampleCount;
	}

	// If the buffer is out of data, mark this channel to go away.
	if(pBuffer->GetReadAvailable() == 0)
	{
		pChannel->m_bStarved = true;
	}

	if(voice_showchannels.GetInt() >= 2)
	{
		Msg("Voice - mixed %d samples from channel %d\n", nSamplesGotten, iChannel);
	}

	VoiceSE_MoveMouth(pChannel->m_iEntity, (short*)copyBufBytes, nSamplesGotten);
	return nSamplesGotten;
}


void Voice_OnAudioSourceShutdown( int iChannel )
{
	Voice_EndChannel( iChannel );
}


// ------------------------------------------------------------------------ //
// Internal stuff.
// ------------------------------------------------------------------------ //

CVoiceChannel* GetVoiceChannel(int iChannel, bool bAssert=true)
{
	if(iChannel < 0 || iChannel >= VOICE_NUM_CHANNELS)
	{
		if(bAssert)
		{
			assert(false);
		}
		return NULL;
	}
	else
	{
		return &g_VoiceChannels[iChannel];
	}
}


bool Voice_Init(const char *pCodecName)
{
	if ( voice_enable.GetInt() == 0 )
	{
		return false;
	}

	Voice_Deinit();

	g_bVoiceAtLeastPartiallyInitted = true;

	if(!VoiceSE_Init())
		return false;

	// Get the voice input device.
	int rate = g_VoiceSampleFormat.nSamplesPerSec;
	g_pVoiceRecord = CreateVoiceRecord_DSound( rate );
	if( !g_pVoiceRecord )
	{
		Msg( "Unable to initialize DirectSoundCapture. You won't be able to speak to other players." );
	}

	// Get the codec.
	char dllName[256];
	Q_snprintf( dllName, sizeof( dllName ), "%s.dll", pCodecName );
	CreateInterfaceFn createCodecFn;
	g_hVoiceCodecDLL = FileSystem_LoadModule(dllName);

	if ( !g_hVoiceCodecDLL || !(createCodecFn = Sys_GetFactory(g_hVoiceCodecDLL)) ||
		 !(g_pEncodeCodec = (IVoiceCodec*)createCodecFn(pCodecName, NULL)) || !g_pEncodeCodec->Init() )
	{
		Msg("Unable to load voice codec '%s'. Voice disabled.\n", pCodecName);
		Voice_Deinit();
		return false;
	}

	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
	{
		CVoiceChannel *pChannel = &g_VoiceChannels[i];

		if(!(pChannel->m_pVoiceCodec = (IVoiceCodec*)createCodecFn(pCodecName, NULL)) || !pChannel->m_pVoiceCodec->Init())
		{
			Voice_Deinit();
			return false;
		}
	}

	g_pMixerControls = GetMixerControls();
	
	if( voice_forcemicrecord.GetInt() )
	{
		if( g_pMixerControls )
			g_pMixerControls->SelectMicrophoneForWaveInput();
	}

	return true;
}


void Voice_EndChannel(int iChannel)
{
	assert(iChannel >= 0 && iChannel < VOICE_NUM_CHANNELS);

	CVoiceChannel *pChannel = &g_VoiceChannels[iChannel];
	
	if ( pChannel->m_iEntity != -1 )
	{
		int iEnt = pChannel->m_iEntity;
		pChannel->m_iEntity = -1;

		VoiceSE_EndChannel( iChannel );
		g_pSoundServices->OnChangeVoiceStatus( iEnt, false );
		VoiceSE_CloseMouth( iEnt );
	}
}


void Voice_EndAllChannels()
{
	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
	{
		Voice_EndChannel(i);
	}
}


void Voice_Deinit()
{
	// This call tends to be expensive and when voice is not enabled it will continually
	// call in here, so avoid the work if possible.
	if( !g_bVoiceAtLeastPartiallyInitted )
		return;

	Voice_EndAllChannels();

	Voice_RecordStop();

	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
	{
		CVoiceChannel *pChannel = &g_VoiceChannels[i];
		
		if(pChannel->m_pVoiceCodec)
		{
			pChannel->m_pVoiceCodec->Release();
			pChannel->m_pVoiceCodec = NULL;
		}
	}

	if(g_pEncodeCodec)
	{
		g_pEncodeCodec->Release();
		g_pEncodeCodec = NULL;
	}

	if(g_hVoiceCodecDLL)
	{
		FileSystem_UnloadModule(g_hVoiceCodecDLL);
		g_hVoiceCodecDLL = NULL;
	}

	if(g_pVoiceRecord)
	{
		g_pVoiceRecord->Release();
		g_pVoiceRecord = NULL;
	}

	VoiceSE_Term();

	if( g_pMixerControls )
	{
		g_pMixerControls->Release();
		g_pMixerControls = NULL;
	}

	g_bVoiceAtLeastPartiallyInitted = false;
}


bool Voice_GetLoopback()
{
	return !!voice_loopback.GetInt();
}


void Voice_LocalPlayerTalkingAck()
{
	if(!g_bLocalPlayerTalkingAck)
	{
		// Tell the client DLL when this changes.
		g_pSoundServices->OnChangeVoiceStatus(-2, TRUE);
	}

	g_bLocalPlayerTalkingAck = true;
	g_LocalPlayerTalkingTimeout = 0;
}


void Voice_UpdateVoiceTweakMode()
{
	if(!g_bInTweakMode || !g_pVoiceRecord)
		return;

	char uchVoiceData[4096];
	bool bFinal = false;
	int nDataLength = Voice_GetCompressedData(uchVoiceData, sizeof(uchVoiceData), bFinal);

	Voice_AddIncomingData(TWEAKMODE_CHANNELINDEX, uchVoiceData, nDataLength, 0);
}


void Voice_Idle(float frametime)
{
	if( voice_enable.GetInt() == 0 )
	{
		Voice_Deinit();
		return;
	}

	if(g_bLocalPlayerTalkingAck)
	{
		g_LocalPlayerTalkingTimeout += frametime;
		if(g_LocalPlayerTalkingTimeout > LOCALPLAYERTALKING_TIMEOUT)
		{
			g_bLocalPlayerTalkingAck = false;
			
			// Tell the client DLL.
			g_pSoundServices->OnChangeVoiceStatus(-2, FALSE);
		}
	}

	// Precalculate these to speedup the voice fadeout.
	g_nVoiceFadeSamples = max((int)(voice_fadeouttime.GetFloat() * VOICE_OUTPUT_SAMPLE_RATE), 2);
	g_VoiceFadeMul = 1.0f / (g_nVoiceFadeSamples - 1);

	if(g_pVoiceRecord)
		g_pVoiceRecord->Idle();

	// If we're in voice tweak mode, feed our own data back to us.
	Voice_UpdateVoiceTweakMode();

	// Age the channels.
	int nActive = 0;
	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
	{
		CVoiceChannel *pChannel = &g_VoiceChannels[i];
		
		if(pChannel->m_iEntity != -1)
		{
			if(pChannel->m_bStarved)
			{
				// Kill the channel. It's done playing.
				Voice_EndChannel(i);
			}
			else
			{
				float oldpad = pChannel->m_TimePad;
				pChannel->m_TimePad -= frametime;
				if(oldpad > 0 && pChannel->m_TimePad <= 0)
				{
					// Start its audio.
					VoiceSE_StartChannel(i);
					g_pSoundServices->OnChangeVoiceStatus(pChannel->m_iEntity, TRUE);
					
					VoiceSE_InitMouth(pChannel->m_iEntity);
				}

				++nActive;
			}
		}
	}

	if(nActive == 0)
		VoiceSE_EndOverdrive();

	VoiceSE_Idle(frametime);

	// voice_showchannels.
	if( voice_showchannels.GetInt() >= 1 )
	{
		for(int i=0; i < VOICE_NUM_CHANNELS; i++)
		{
			CVoiceChannel *pChannel = &g_VoiceChannels[i];
			
			if(pChannel->m_iEntity == -1)
				continue;

			Msg("Voice - chan %d, ent %d, bufsize: %d\n", i, pChannel->m_iEntity, pChannel->m_Buffer.GetReadAvailable());
		}
	}

	// Show profiling data?
	if( voice_profile.GetInt() )
	{
		Msg("Voice - compress: %7.2fu, decompress: %7.2fu, gain: %7.2fu, upsample: %7.2fu, total: %7.2fu\n", 
			g_CompressTime*1000000.0, 
			g_DecompressTime*1000000.0, 
			g_GainTime*1000000.0, 
			g_UpsampleTime*1000000.0,
			(g_CompressTime+g_DecompressTime+g_GainTime+g_UpsampleTime)*1000000.0
			);

		g_CompressTime = g_DecompressTime = g_GainTime = g_UpsampleTime = 0;
	}
}


bool Voice_IsRecording()
{
	return g_bVoiceRecording && !g_bInTweakMode;
}


bool Voice_RecordStart(
	const char *pUncompressedFile, 
	const char *pDecompressedFile,
	const char *pMicInputFile)
{
	if(!g_pEncodeCodec)
		return false;

	Voice_RecordStop();

	g_pEncodeCodec->ResetState();

	if(pMicInputFile)
	{
		int a, b, c;
		ReadWaveFile(pMicInputFile, g_pMicInputFileData, g_nMicInputFileBytes, a, b, c);
		g_CurMicInputFileByte = 0;
		g_MicStartTime = g_pSoundServices->GetRealTime();
	}

	if(pUncompressedFile)
	{
		g_pUncompressedFileData = new char[MAX_WAVEFILEDATA_LEN];
		g_nUncompressedDataBytes = 0;
		g_pUncompressedDataFilename = pUncompressedFile;
	}

	if(pDecompressedFile)
	{
		g_pDecompressedFileData = new char[MAX_WAVEFILEDATA_LEN];
		g_nDecompressedDataBytes = 0;
		g_pDecompressedDataFilename = pDecompressedFile;
	}

	g_bVoiceRecording = false;
	if(g_pVoiceRecord)
	{
		g_bVoiceRecording = g_pVoiceRecord->RecordStart();
		if(g_bVoiceRecording)
			g_pSoundServices->OnChangeVoiceStatus(-1, TRUE);		// Tell the client DLL.
	}

	return g_bVoiceRecording;
}


bool Voice_RecordStop()
{
	// Write the files out for debugging.
	if(g_pMicInputFileData)
	{
		delete [] g_pMicInputFileData;
		g_pMicInputFileData = NULL;
	}

	if(g_pUncompressedFileData)
	{
		WriteWaveFile(g_pUncompressedDataFilename, g_pUncompressedFileData, g_nUncompressedDataBytes, g_VoiceSampleFormat.wBitsPerSample, g_VoiceSampleFormat.nChannels, g_VoiceSampleFormat.nSamplesPerSec);
		delete [] g_pUncompressedFileData;
		g_pUncompressedFileData = NULL;
	}

	if(g_pDecompressedFileData)
	{
		WriteWaveFile(g_pDecompressedDataFilename, g_pDecompressedFileData, g_nDecompressedDataBytes, g_VoiceSampleFormat.wBitsPerSample, g_VoiceSampleFormat.nChannels, g_VoiceSampleFormat.nSamplesPerSec);
		delete [] g_pDecompressedFileData;
		g_pDecompressedFileData = NULL;
	}
	
	if(g_pVoiceRecord)
		g_pVoiceRecord->RecordStop();

	if(g_bVoiceRecording)
		g_pSoundServices->OnChangeVoiceStatus(-1, FALSE);		// Tell the client DLL.

	g_bVoiceRecording = false;
	return(true);
}


int Voice_GetCompressedData(char *pchDest, int nCount, bool bFinal)
{
	IVoiceCodec *pCodec = g_pEncodeCodec;
	if(g_pVoiceRecord && pCodec)
	{
		short tempData[8192];
		int gotten = g_pVoiceRecord->GetRecordedData(tempData, min(nCount/BYTES_PER_SAMPLE, (int)sizeof(tempData)/BYTES_PER_SAMPLE));
		
		// If they want to get the data from a file instead of the mic, use that.
		if(g_pMicInputFileData)
		{
			double curtime = g_pSoundServices->GetRealTime();
			int nShouldGet = (curtime - g_MicStartTime) * g_VoiceSampleFormat.nSamplesPerSec;
			gotten = min(sizeof(tempData)/BYTES_PER_SAMPLE, min(nShouldGet, (g_nMicInputFileBytes - g_CurMicInputFileByte) / BYTES_PER_SAMPLE));
			memcpy(tempData, &g_pMicInputFileData[g_CurMicInputFileByte], gotten*BYTES_PER_SAMPLE);
			g_CurMicInputFileByte += gotten * BYTES_PER_SAMPLE;
			g_MicStartTime = curtime;
		}

		int nCompressedBytes = pCodec->Compress((char*)tempData, gotten, pchDest, nCount, !!bFinal);

		// Write to our file buffers..
		if(g_pUncompressedFileData)
		{
			int nToWrite = min(gotten*BYTES_PER_SAMPLE, MAX_WAVEFILEDATA_LEN - g_nUncompressedDataBytes);
			memcpy(&g_pUncompressedFileData[g_nUncompressedDataBytes], tempData, nToWrite);
			g_nUncompressedDataBytes += nToWrite;
		}

		return nCompressedBytes;
	}
	else
	{
		return 0;
	}
}


//------------------ Copyright (c) 1999 Valve, LLC. ----------------------------
// Purpose: Assigns a channel to an entity by searching for either a channel
//			already assigned to that entity or picking the least recently used
//			channel. If the LRU channel is picked, it is flushed and all other
//			channels are aged.
// Input  : nEntity - entity number to assign to a channel.
// Output : A channel index to which the entity has been assigned.
//------------------------------------------------------------------------------
int Voice_AssignChannel(int nEntity)
{
	if(g_bInTweakMode)
		return VOICE_CHANNEL_IN_TWEAK_MODE;

	// See if a channel already exists for this entity and if so, just return it.
	int iFree = -1;
	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
	{
		CVoiceChannel *pChannel = &g_VoiceChannels[i];

		if(pChannel->m_iEntity == nEntity)
		{
			return i;
		}
		else if(pChannel->m_iEntity == -1 && pChannel->m_pVoiceCodec)
		{
			pChannel->m_pVoiceCodec->ResetState();
			iFree = i;
			break;
		}
	}

	// If they're all used, then don't allow them to make a new channel.
	if(iFree == -1)
	{
		return VOICE_CHANNEL_ERROR;
	}

	CVoiceChannel *pChannel = &g_VoiceChannels[iFree];
	pChannel->Init(nEntity);
	VoiceSE_StartOverdrive();

	return iFree;
}


//------------------ Copyright (c) 1999 Valve, LLC. ----------------------------
// Purpose: Determines which channel has been assigened to a given entity.
// Input  : nEntity - entity number.
// Output : The index of the channel assigned to the entity, VOICE_CHANNEL_ERROR
//			if no channel is currently assigned to the given entity.
//------------------------------------------------------------------------------
int Voice_GetChannel(int nEntity)
{
	for(int i=0; i < VOICE_NUM_CHANNELS; i++)
		if(g_VoiceChannels[i].m_iEntity == nEntity)
			return i;

	return VOICE_CHANNEL_ERROR;
}


double UpsampleIntoBuffer(
	const short *pSrc,
	int nSrcSamples,
	CCircularBuffer *pBuffer,
	double startFraction,
	double rate)
{
	double maxFraction = nSrcSamples - 1;

	while(1)
	{
		if(startFraction >= maxFraction)
			break;

		int iSample = (int)startFraction;
		double frac = startFraction - floor(startFraction);

		double val1 = pSrc[iSample];
		double val2 = pSrc[iSample+1];
		short newSample = (short)(val1 + (val2 - val1) * frac);
		pBuffer->Write(&newSample, sizeof(newSample));

		startFraction += rate;
	}

	return startFraction - floor(startFraction);
}


//------------------ Copyright (c) 1999 Valve, LLC. ----------------------------
// Purpose: Adds received voice data to 
// Input  : 
// Output : 
//------------------------------------------------------------------------------
int Voice_AddIncomingData(int nChannel, const char *pchData, int nCount, int iSequenceNumber)
{
	CVoiceChannel *pChannel;

	// If in tweak mode, we call this during Idle with -1 as the channel, so any channel data from the network
	// gets rejected.
	if(g_bInTweakMode)
	{
		if(nChannel == TWEAKMODE_CHANNELINDEX)
			nChannel = 0;
		else
			return 0;
	}

	if(!(pChannel = GetVoiceChannel(nChannel)) || !pChannel->m_pVoiceCodec)
	{
		return(0);
	}

	pChannel->m_bStarved = false;	// This only really matters if you call Voice_AddIncomingData between the time the mixer
									// asks for data and Voice_Idle is called.

	// Decompress.
	char decompressed[8192];
	int nDecompressed = pChannel->m_pVoiceCodec->Decompress(pchData, nCount, decompressed, sizeof(decompressed));

	pChannel->m_AutoGain.ProcessSamples((short*)decompressed, nDecompressed);
	
	// Upsample into the dest buffer. We could do this in a mixer but it complicates the mixer.
	pChannel->m_LastFraction = UpsampleIntoBuffer(
		(short*)decompressed, 
		nDecompressed, 
		&pChannel->m_Buffer, 
		pChannel->m_LastFraction,
		(double)g_VoiceSampleFormat.nSamplesPerSec/VOICE_OUTPUT_SAMPLE_RATE);
	pChannel->m_LastSample = decompressed[nDecompressed];

	// Write to our file buffer..
	if(g_pDecompressedFileData)
	{											  
		int nToWrite = min(nDecompressed*2, MAX_WAVEFILEDATA_LEN - g_nDecompressedDataBytes);
		memcpy(&g_pDecompressedFileData[g_nDecompressedDataBytes], decompressed, nToWrite);
		g_nDecompressedDataBytes += nToWrite;
	}

	if( voice_showincoming.GetInt() != 0 )
	{
		Msg("Voice - %d incoming samples added to channel %d\n", nDecompressed, nChannel);
	}

	return(nChannel);
}


#if DEAD
//------------------ Copyright (c) 1999 Valve, LLC. ----------------------------
// Purpose: Flushes a given receive channel.
// Input  : nChannel - index of channel to flush.
//------------------------------------------------------------------------------
void Voice_FlushChannel(int nChannel)
{
	if ((nChannel < 0) || (nChannel >= VOICE_NUM_CHANNELS))
	{
		assert(false);
		return;
	}

	g_VoiceChannels[nChannel].m_Buffer.Flush();
}
#endif


//------------------------------------------------------------------------------
// IVoiceTweak implementation.
//------------------------------------------------------------------------------

int VoiceTweak_StartVoiceTweakMode()
{
	// If we're already in voice tweak mode, return an error.
	if(g_bInTweakMode)
	{
		assert(!"VoiceTweak_StartVoiceTweakMode called while already in tweak mode.");
		return 0;
	}

	if( !g_pMixerControls )
		return 0;

	Voice_EndAllChannels();
	Voice_RecordStart(NULL, NULL, NULL);
	Voice_AssignChannel(TWEAKMODE_ENTITYINDEX);
	g_bInTweakMode = true;
	g_pMixerControls = GetMixerControls();

	return 1;
}

void VoiceTweak_EndVoiceTweakMode()
{
	if(!g_bInTweakMode)
	{
		assert(!"VoiceTweak_EndVoiceTweakMode called when not in tweak mode.");
		return;
	}

	g_bInTweakMode = false;
	Voice_RecordStop();
}

void VoiceTweak_SetControlFloat(VoiceTweakControl iControl, float value)
{
	if(!g_pMixerControls)
		return;

	if(iControl == MicrophoneVolume)
	{
		g_pMixerControls->SetValue_Float(IMixerControls::Control::MicVolume, value);
	}
	else if(iControl == OtherSpeakerScale)
	{
		voice_scale.SetValue( value );
	}
}

float VoiceTweak_GetControlFloat(VoiceTweakControl iControl)
{
	if(!g_pMixerControls)
		return 0;

	if(iControl == MicrophoneVolume)
	{
		float value = 1;
		g_pMixerControls->GetValue_Float(IMixerControls::Control::MicVolume, value);
		return value;
	}
	else if(iControl == OtherSpeakerScale)
	{
		return voice_scale.GetFloat();
	}
	else
	{
		return 1;
	}
}


IVoiceTweak g_VoiceTweakAPI =
{
	VoiceTweak_StartVoiceTweakMode,
	VoiceTweak_EndVoiceTweakMode,
	VoiceTweak_SetControlFloat,
	VoiceTweak_GetControlFloat
};


