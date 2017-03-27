//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================

#include <dsound.h>

#include "tier0/dbg.h"
#include "sound.h"
#include "convar.h"
#include "sound_private.h"
#include "snd_device.h"
#include "iprediction.h"
#include "eax.h"
#include "snd_mix_buf.h"
#include "snd_env_fx.h"
#include "snd_channels.h"
#include "snd_audio_source.h"
#include "snd_convars.h"
#include "SoundService.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern qboolean snd_firsttime;
extern ConVar snd_mixahead;

extern ConVar s_surround;
extern void DEBUG_StartSoundMeasure(int type, int samplecount );
extern void DEBUG_StopSoundMeasure(int type, int samplecount );

// legacy support
extern ConVar sxroom_off;
extern ConVar sxroom_type;
extern ConVar sxroomwater_type;
extern float sxroom_typeprev;

extern HWND* pmainwindow;

typedef enum {SIS_SUCCESS, SIS_FAILURE, SIS_NOTAVAIL} sndinitstat;
#define SECONDARY_BUFFER_SIZE	0x10000

#define iDirectSoundCreate(a,b,c)	pDirectSoundCreate(a,b,c)

HRESULT (WINAPI *pDirectSoundCreate)(GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter);
extern void ReleaseSurround(void);

extern void MIX_ScaleChannelVolume( paintbuffer_t *ppaint, channel_t *pChannel, int volume[CCHANVOLUMES], int mixchans );

LPDIRECTSOUND pDS;
LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;

//////////////////////////////////////////////////////////////////////////////////
typedef struct _EAXPRESET 
{
	int dwEnvironment;
	float fVolume;
	float fDecay;
	float fDamping;
} EAXPRESET;

EAXPRESET eax_preset[CSXROOM] = {

 {EAX_ENVIRONMENT_GENERIC,0.0F,0.0F,0.0F},
 {EAX_ENVIRONMENT_ROOM,0.417F,0.4F,0.666F},
 {EAX_ENVIRONMENT_BATHROOM,0.3F,1.499F,0.166F},
 {EAX_ENVIRONMENT_BATHROOM,0.4F,1.499F,0.166F},
 {EAX_ENVIRONMENT_BATHROOM,0.6F,1.499F,0.166F},
 {EAX_ENVIRONMENT_SEWERPIPE,0.4F,2.886F,0.25F},
 {EAX_ENVIRONMENT_SEWERPIPE,0.6F,2.886F,0.25F},
 {EAX_ENVIRONMENT_SEWERPIPE,0.8F,2.886F,0.25F},
 {EAX_ENVIRONMENT_STONEROOM,0.5F,2.309F,0.888F},
 {EAX_ENVIRONMENT_STONEROOM,0.65F,2.309F,0.888F},
 {EAX_ENVIRONMENT_STONEROOM,0.8F,2.309F,0.888F},
 {EAX_ENVIRONMENT_STONECORRIDOR,0.3F,2.697F,0.638F},
 {EAX_ENVIRONMENT_STONECORRIDOR,0.5F,2.697F,0.638F},
 {EAX_ENVIRONMENT_STONECORRIDOR,0.65F,2.697F,0.638F},
 {EAX_ENVIRONMENT_UNDERWATER,1.0F,1.499F,0.0F},
 {EAX_ENVIRONMENT_UNDERWATER,1.0F,2.499F,0.0F},
 {EAX_ENVIRONMENT_UNDERWATER,1.0F,3.499F,0.0F},
 {EAX_ENVIRONMENT_GENERIC,0.65F,1.493F,0.5F},
 {EAX_ENVIRONMENT_GENERIC,0.85F,1.493F,0.5F},
 {EAX_ENVIRONMENT_GENERIC,1.0F,1.493F,0.5F},
 {EAX_ENVIRONMENT_ARENA,0.40F,7.284F,0.332F},
 {EAX_ENVIRONMENT_ARENA,0.55F,7.284F,0.332F},
 {EAX_ENVIRONMENT_ARENA,0.70F,7.284F,0.332F},
 {EAX_ENVIRONMENT_CONCERTHALL,0.5F,3.961F,0.5F},
 {EAX_ENVIRONMENT_CONCERTHALL,0.7F,3.961F,0.5F},
 {EAX_ENVIRONMENT_CONCERTHALL,1.0F,3.961F,0.5F},
 {EAX_ENVIRONMENT_DIZZY,0.2F,17.234F,0.666F},
 {EAX_ENVIRONMENT_DIZZY,0.3F,17.234F,0.666F},
 {EAX_ENVIRONMENT_DIZZY,0.4F,17.234F,0.666F},

};


//////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// Purpose: Implementation of direct sound & EAX as an audio device
// UNDONE: Split EAX into a derived class?
//-----------------------------------------------------------------------------
class CAudioDirectSound : public IAudioDevice
{
public:
	~CAudioDirectSound( void );
	bool		IsActive( void ) { return true; }

	bool		Init( void );
	void		Shutdown( void );

	int			GetOutputPosition( void );
	int			GetOutputPositionSurround( void );
	void		Pause( void );
	void		UnPause( void );
	float		MixDryVolume( void );
	bool		Should3DMix( void );

	void		StopAllSounds( void );
	void		SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront  );
	void		ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount );
	void		ClearBuffer( void );
	void		UpdateListener( const Vector& position, const Vector& forward, const Vector& right, const Vector& up );
	void		MixBegin( int sampleCount );
	void		MixUpsample( int sampleCount, int filtertype );
	void		Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );

	int			PaintBegin( int soundtime, int paintedtime );
	void		PaintEnd( void );
	void		ChannelReset( int entnum, int channelIndex, float distanceMod );

	void		TransferSamples( int end );

	const char *DeviceName( void );
	int			DeviceChannels( void )		{ return m_deviceChannels; }
	int			DeviceSampleBits( void )	{ return m_deviceSampleBits; }
	int			DeviceSampleBytes( void )	{ return m_deviceSampleBits/8; }
	
	int			DeviceDmaSpeed( void )		{ return m_deviceDmaSpeed; }
	
	int			DeviceSampleCount( void )	{ return m_deviceSampleCount; }
	void		*DeviceLockBuffer( void );
	void		DeviceUnlockBuffer( void *pbuffer );

	sndinitstat SNDDMA_InitDirect( void );
	bool		SNDDMA_InitSurround(LPDIRECTSOUND lpDS, WAVEFORMATEX* lpFormat, DSBCAPS* lpdsbc);
	void		S_TransferStereo16Surround( portable_samplepair_t *pfront, portable_samplepair_t *prear, int lpaintedtime, int endtime);

	// Singleton object
	static		CAudioDirectSound *m_pSingleton;

private:
	int			m_bufferRefCount;

	int			m_deviceChannels;
	int			m_deviceSampleBits;
	int			m_deviceSampleCount;
	int			m_deviceDmaSpeed;
	int			m_bufferSize;
	
	// save this to do front/back on EAX
	Vector		m_listenerForward;
	MMTIME		m_mmstarttime;
	DWORD		m_lockBufferSize;
	HINSTANCE	m_hInstDS;
};

CAudioDirectSound *CAudioDirectSound::m_pSingleton = NULL;

bool SURROUND_ON;

/////////////////////////////////////////////////////////////////////////////////////////////

void ReleaseSurround(void);
GUID IID_IKsPropertySetDef = {0x31efac30, 0x515c, 0x11d0, {0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93}};
GUID DSPROPSETID_EAX_ReverbPropertiesDef = {0x4a4e6fc1, 0xc341, 0x11d1, {0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
GUID IID_IDirectSound3DBufferDef = {0x279AFA86, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
GUID DSPROPSETID_EAXBUFFER_ReverbPropertiesDef = {0x4a4e6fc0, 0xc341, 0x11d1, {0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
LPDIRECTSOUNDBUFFER pDSBufFL = NULL;
LPDIRECTSOUNDBUFFER pDSBufFR = NULL;
LPDIRECTSOUNDBUFFER pDSBufRL = NULL;
LPDIRECTSOUNDBUFFER pDSBufRR = NULL;
LPDIRECTSOUND3DBUFFER pDSBuf3DFL = NULL;
LPDIRECTSOUND3DBUFFER pDSBuf3DFR = NULL;
LPDIRECTSOUND3DBUFFER pDSBuf3DRL = NULL;
LPDIRECTSOUND3DBUFFER pDSBuf3DRR = NULL;
LPKSPROPERTYSET pDSPropSetFL = NULL;
LPKSPROPERTYSET pDSPropSetFR = NULL;
LPKSPROPERTYSET pDSPropSetRL = NULL;
LPKSPROPERTYSET pDSPropSetRR = NULL;
DWORD dwSurroundStart = 0;
int g_iEaxPreset = 0;
extern EAXPRESET eax_preset[29];


// ----------------------------------------------------------------------------- //
// Helpers.
// ----------------------------------------------------------------------------- //



CAudioDirectSound::~CAudioDirectSound( void )
{
	m_pSingleton = NULL;
}

bool CAudioDirectSound::Init( void )
{
	m_hInstDS = NULL;
	m_bufferRefCount = 1;

	if ( SNDDMA_InitDirect() == SIS_SUCCESS)
	{
		return true;
	}

	return false;
}

void CAudioDirectSound::Shutdown( void )
{
	/////////////////////////////////////////////////////////////////////////////////////////////
	ReleaseSurround();
	/////////////////////////////////////////////////////////////////////////////////////////////

	if (pDSBuf)
	{
		pDSBuf->Stop();
		pDSBuf->Release();
	}

// only release primary buffer if it's not also the mixing buffer we just released
	if (pDSPBuf && (pDSBuf != pDSPBuf))
	{
		pDSPBuf->Release();
	}

	if (pDS)
	{
		pDS->SetCooperativeLevel (*pmainwindow, DSSCL_NORMAL);
		pDS->Release();
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	if ( m_hInstDS )
	{
		FreeLibrary( m_hInstDS );
		m_hInstDS = NULL;
	}
}

int	CAudioDirectSound::GetOutputPosition( void )
{
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if( SURROUND_ON )
	{
		return GetOutputPositionSurround();
	} 
	else
	{
		mmtime.wType = TIME_SAMPLES;
		pDSBuf->GetCurrentPosition(&mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - m_mmstarttime.u.sample;
	}

	s >>= SAMPLE_16BIT_SHIFT;

	s &= (DeviceSampleCount()-1);

	return s;
}

void CAudioDirectSound::Pause( void )
{
	if (pDSBuf)
		pDSBuf->Stop();

///////////////////////////////////////////////////////////////////////////////////
// Need to shut off reverb.
	if ( pDSPropSetFL )
		 pDSPropSetFL->Set(DSPROPSETID_EAX_ReverbPropertiesDef, 
							 DSPROPERTY_EAX_ALL, NULL, 0, &eax_preset[0], 
							 sizeof(EAX_REVERBPROPERTIES));
	if ( pDSBufFL ) pDSBufFL->Stop(); 
	if ( pDSBufFR ) pDSBufFR->Stop(); 
	if ( pDSBufRL ) pDSBufRL->Stop(); 
	if ( pDSBufRR ) pDSBufRR->Stop(); 
///////////////////////////////////////////////////////////////////////////////////
}


void CAudioDirectSound::UnPause( void )
{
	if (pDSBuf)
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);

///////////////////////////////////////////////////////////////////////////////////
// Need to restore reverb if restarting in middle somewhere.
	if ( pDSPropSetFL )
		pDSPropSetFL->Set(DSPROPSETID_EAX_ReverbPropertiesDef, 
                         DSPROPERTY_EAX_ALL, NULL, 0, &eax_preset[g_iEaxPreset], 
                         sizeof(EAX_REVERBPROPERTIES));

	if (pDSBufFL) pDSBufFL->Play(0, 0, DSBPLAY_LOOPING); 
	if (pDSBufFR) pDSBufFR->Play(0, 0, DSBPLAY_LOOPING); 
	if (pDSBufRL) pDSBufRL->Play(0, 0, DSBPLAY_LOOPING); 
	if (pDSBufRR) pDSBufRR->Play( 0, 0, DSBPLAY_LOOPING); 
///////////////////////////////////////////////////////////////////////////////////
}


float CAudioDirectSound::MixDryVolume( void )
{
	return 0;
}


bool CAudioDirectSound::Should3DMix( void )
{
	if ( SURROUND_ON )
		return true;
	return false;
}


IAudioDevice *Audio_CreateDirectSoundDevice( void )
{
	if ( !CAudioDirectSound::m_pSingleton )
		CAudioDirectSound::m_pSingleton = new CAudioDirectSound;

	if ( CAudioDirectSound::m_pSingleton->Init() )
	{
		if (snd_firsttime)
			DevMsg ("DirectSound initialized\n");

		return CAudioDirectSound::m_pSingleton;
	}

	DevMsg ("DirectSound failed to init\n");

	delete CAudioDirectSound::m_pSingleton;
	CAudioDirectSound::m_pSingleton = NULL;

	return NULL;
}


int CAudioDirectSound::PaintBegin( int soundtime, int paintedtime )
{
	//  soundtime - total samples that have been played out to hardware at dmaspeed
	//  paintedtime - total samples that have been mixed at speed
	//  endtime - target for samples in mixahead buffer at speed
	int endtime = soundtime + snd_mixahead.GetFloat() * DeviceDmaSpeed();
	int samps = DeviceSampleCount() >> (DeviceChannels()-1);

	if ((int)(endtime - soundtime) > samps)
		endtime = soundtime + samps;
	
	if ((endtime - paintedtime) & 0x3)
	{
		// The difference between endtime and painted time should align on 
		// boundaries of 4 samples.  This is important when upsampling from 11khz -> 44khz.
		endtime -= (endtime - paintedtime) & 0x3;
	}

	DWORD	dwStatus;

	/////////////////////////////////////////////////////////////////////////////////////////
	// If using surround, there are 4 different buffers being used and the pDSBuf is NULL.

	if( SURROUND_ON ) 
	{
		if (pDSBufFL->GetStatus(&dwStatus) != DS_OK)
			Msg ("Couldn't get SURROUND FL sound buffer status\n");
		
		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSBufFL->Restore();
		
		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSBufFL->Play(0, 0, DSBPLAY_LOOPING);

		if (pDSBufFR->GetStatus(&dwStatus) != DS_OK)
			Msg ("Couldn't get SURROUND FR sound buffer status\n");
		
		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSBufFR->Restore();
		
		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSBufFR->Play(0, 0, DSBPLAY_LOOPING);

		if (pDSBufRL->GetStatus(&dwStatus) != DS_OK)
			Msg ("Couldn't get SURROUND RL sound buffer status\n");
		
		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSBufRL->Restore();
		
		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSBufRL->Play(0, 0, DSBPLAY_LOOPING);

		if (pDSBufRR->GetStatus(&dwStatus) != DS_OK)
			Msg ("Couldn't get SURROUND RR sound buffer status\n");
		
		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSBufRR->Restore();
		
		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSBufRR->Play(0, 0, DSBPLAY_LOOPING);
	}
	else if (pDSBuf)
	{
		if (pDSBuf->GetStatus (&dwStatus) != DS_OK)
			Msg ("Couldn't get sound buffer status\n");
		
		if (dwStatus & DSBSTATUS_BUFFERLOST)
			pDSBuf->Restore ();
		
		if (!(dwStatus & DSBSTATUS_PLAYING))
			pDSBuf->Play(0, 0, DSBPLAY_LOOPING);
	}

	return endtime;
}


void CAudioDirectSound::PaintEnd( void )
{
}


void CAudioDirectSound::ClearBuffer( void )
{
	int		clear;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
	DWORD	dwSizeFL, dwSizeFR, dwSizeRL, dwSizeRR;
	char	*pDataFL, *pDataFR, *pDataRL, *pDataRR;
 
	if ( SURROUND_ON )
	{
		int		SURROUNDreps;
		HRESULT	SURROUNDhresult;
		SURROUNDreps = 0;

		if ( !pDSBufFL && !pDSBufFR && !pDSBufRL && !pDSBufRR )
			return;

		while ((SURROUNDhresult = pDSBufFL->Lock(0, m_bufferSize/2, (void**)&pDataFL, &dwSizeFL, NULL, NULL, 0)) != DS_OK)
		{
			if (SURROUNDhresult != DSERR_BUFFERLOST)
			{
				Msg ("S_ClearBuffer: DS::Lock FL Sound Buffer Failed\n");
				S_Shutdown ();
				return;
			}

			if (++SURROUNDreps > 10000)
			{
				Msg ("S_ClearBuffer: DS: couldn't restore FL buffer\n");
				S_Shutdown ();
				return;
			}
		}

		SURROUNDreps = 0;
		while ((SURROUNDhresult = pDSBufFR->Lock(0, m_bufferSize/2, (void**)&pDataFR, &dwSizeFR, NULL, NULL, 0)) != DS_OK)
		{
			if (SURROUNDhresult != DSERR_BUFFERLOST)
			{
				Msg ("S_ClearBuffer: DS::Lock FR Sound Buffer Failed\n");
				S_Shutdown ();
				return;
			}

			if (++SURROUNDreps > 10000)
			{
				Msg ("S_ClearBuffer: DS: couldn't restore FR buffer\n");
				S_Shutdown ();
				return;
			}
		}

		SURROUNDreps = 0;
		while ((SURROUNDhresult = pDSBufRL->Lock(0, m_bufferSize/2, (void**)&pDataRL, &dwSizeRL, NULL, NULL, 0)) != DS_OK)
		{
			if (SURROUNDhresult != DSERR_BUFFERLOST)
			{
				Msg ("S_ClearBuffer: DS::Lock RL Sound Buffer Failed\n");
				S_Shutdown ();
				return;
			}

			if (++SURROUNDreps > 10000)
			{
				Msg ("S_ClearBuffer: DS: couldn't restore RL buffer\n");
				S_Shutdown ();
				return;
			}
		}

		SURROUNDreps = 0;
		while ((SURROUNDhresult = pDSBufRR->Lock(0, m_bufferSize/2, (void**)&pDataRR, &dwSizeRR, NULL, NULL, 0)) != DS_OK)
		{
			if (SURROUNDhresult != DSERR_BUFFERLOST)
			{
				Msg ("S_ClearBuffer: DS::Lock RR Sound Buffer Failed\n");
				S_Shutdown ();
				return;
			}

			if (++SURROUNDreps > 10000)
			{
				Msg ("S_ClearBuffer: DS: couldn't restore RR buffer\n");
				S_Shutdown ();
				return;
			}
		}

		Q_memset(pDataFL, 0, m_bufferSize/2);
		Q_memset(pDataFR, 0, m_bufferSize/2);
		Q_memset(pDataRL, 0, m_bufferSize/2);
		Q_memset(pDataRR, 0, m_bufferSize/2);

		pDSBufFL->Unlock(pDataFL, dwSizeFL, NULL, 0);
		pDSBufFR->Unlock(pDataFR, dwSizeFR, NULL, 0);
		pDSBufRL->Unlock(pDataRL, dwSizeRL, NULL, 0);
		pDSBufRR->Unlock(pDataRR, dwSizeRR, NULL, 0);

		return;
	}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
	if ( !pDSBuf )
		return;

	if ( DeviceSampleBits() == 8)
		clear = 0x80;
	else
		clear = 0;

	if (pDSBuf)
	{
		DWORD	dwSize;
		DWORD	*pData;
		int		reps;
		HRESULT	hresult;

		reps = 0;

		while ((hresult = pDSBuf->Lock(0, m_bufferSize, (void**)&pData, &dwSize, NULL, NULL, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Msg ("S_ClearBuffer: DS::Lock Sound Buffer Failed\n");
				S_Shutdown ();
				return;
			}

			if (++reps > 10000)
			{
				Msg ("S_ClearBuffer: DS: couldn't restore buffer\n");
				S_Shutdown ();
				return;
			}
		}

		Q_memset(pData, clear, DeviceSampleCount() * DeviceSampleBytes());

		pDSBuf->Unlock(pData, dwSize, NULL, 0);

	}
}

void CAudioDirectSound::StopAllSounds( void )
{
}

/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
sndinitstat CAudioDirectSound::SNDDMA_InitDirect( void )
{
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	DWORD			dwSize, dwWrite;
	DSCAPS			dscaps;
	WAVEFORMATEX	format, pformat; 
	HRESULT			hresult;
	int				reps;
	void			*lpData = NULL;
	qboolean		primary_format_set;

	// BUGBUG: Making 8 bit mono/stereo output has weird latency problems.
	// UNDONE: Revisit and test this to make sure 8-bit devices will work.

	m_deviceChannels = 2;
	m_deviceSampleBits = 16;
	m_deviceDmaSpeed = SOUND_DMA_SPEED;	// 44k: hardware playback rate

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = m_deviceChannels;
    format.wBitsPerSample = m_deviceSampleBits;
    format.nSamplesPerSec = m_deviceDmaSpeed;
    format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec
		*format.nBlockAlign; 

	if (!m_hInstDS)
	{
		m_hInstDS = LoadLibrary("dsound.dll");
		
		if (m_hInstDS == NULL)
		{
			g_pSoundServices->ConSafePrint ("Couldn't load dsound.dll\n");
			return SIS_FAILURE;
		}

		pDirectSoundCreate = (long (__stdcall *)(struct _GUID *,struct IDirectSound ** ,struct IUnknown *))GetProcAddress(m_hInstDS,"DirectSoundCreate");

		if (!pDirectSoundCreate)
		{
			g_pSoundServices->ConSafePrint ("Couldn't get DS proc addr\n");
			return SIS_FAILURE;
		}
	}

	while ((hresult = iDirectSoundCreate(NULL, &pDS, NULL)) != DS_OK)
	{
		if (hresult != DSERR_ALLOCATED)
		{
			DevMsg ("DirectSound create failed\n");
			return SIS_FAILURE;
		}

#if 0
		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
					    "Select Retry to try to start sound again or Cancel to run Half-Life with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			g_pSoundServices->ConSafePrint ("DirectSoundCreate failure\n"
							"  hardware already in use\n");
			return SIS_NOTAVAIL;
		}
#else
	return SIS_NOTAVAIL;
#endif

	}

	dscaps.dwSize = sizeof(dscaps);

	if (DS_OK != pDS->GetCaps (&dscaps))
	{
		g_pSoundServices->ConSafePrint ("Couldn't get DS caps\n");
	}

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
	{
		g_pSoundServices->ConSafePrint ("No DirectSound driver installed\n");
		Shutdown();
		return SIS_FAILURE;
	}

	if (DS_OK != pDS->SetCooperativeLevel (*pmainwindow, DSSCL_EXCLUSIVE))
	{
		g_pSoundServices->ConSafePrint ("Set coop level failed\n");
		Shutdown();
		return SIS_FAILURE;
	}

// get access to the primary buffer, if possible, so we can set the
// sound hardware format
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_CTRL3D;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = false;

	if (!g_pSoundServices->CheckParm ("-snoforceformat"))
	{
		if (DS_OK == pDS->CreateSoundBuffer(&dsbuf, &pDSPBuf, NULL))
		{
			pformat = format;

			if (DS_OK != pDSPBuf->SetFormat (&pformat))
			{
				if (snd_firsttime)
					DevMsg ("Set primary sound buffer format: no\n");
			}
			else
			{
				if (snd_firsttime)
					DevMsg ("Set primary sound buffer format: yes\n");

				primary_format_set = true;
			}
		}
	}


	SURROUND_ON = FALSE;

	if (s_surround.GetInt()) 
		SURROUND_ON = SNDDMA_InitSurround(pDS, &format, &dsbcaps);

	if ( !SURROUND_ON )
	{
		s_surround.SetValue( 0 );

		if (!primary_format_set || !g_pSoundServices->CheckParm ("-primarysound"))
		{
		// create the secondary buffer we'll actually work with
			memset (&dsbuf, 0, sizeof(dsbuf));
			dsbuf.dwSize = sizeof(DSBUFFERDESC);
			dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
			dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
			dsbuf.lpwfxFormat = &format;

			memset(&dsbcaps, 0, sizeof(dsbcaps));
			dsbcaps.dwSize = sizeof(dsbcaps);

			if (DS_OK != pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL))
			{
				g_pSoundServices->ConSafePrint ("DS:CreateSoundBuffer Failed");
				Shutdown();
				return SIS_FAILURE;
			}

			m_deviceChannels = format.nChannels;
			m_deviceSampleBits = format.wBitsPerSample;
			m_deviceDmaSpeed = format.nSamplesPerSec;

			if (DS_OK != pDSBuf->GetCaps (&dsbcaps))
			{
				g_pSoundServices->ConSafePrint ("DS:GetCaps failed\n");
				Shutdown();
				return SIS_FAILURE;
			}

			if (snd_firsttime)
				DevMsg ("Using secondary sound buffer\n");
		}
		else
		{
			if (DS_OK != pDS->SetCooperativeLevel (*pmainwindow, DSSCL_WRITEPRIMARY))
			{
				g_pSoundServices->ConSafePrint ("Set coop level failed\n");
				Shutdown();
				return SIS_FAILURE;
			}

			if (DS_OK != pDSPBuf->GetCaps (&dsbcaps))
			{
				Msg ("DS:GetCaps failed\n");
				return SIS_FAILURE;
			}

			pDSBuf = pDSPBuf;
			DevMsg ("Using primary sound buffer\n");
		}

		// Make sure mixer is active
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);

		if (snd_firsttime)
			DevMsg("   %d channel(s)\n"
						   "   %d bits/sample\n"
						   "   %d samples/sec\n",
						   DeviceChannels(), DeviceSampleBits(), DeviceDmaSpeed());
		
		m_bufferSize = dsbcaps.dwBufferBytes;

	// initialize the buffer
		reps = 0;

		while ((hresult = pDSBuf->Lock(0, m_bufferSize, (void**)&lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed\n");
				Shutdown();
				return SIS_FAILURE;
			}

			if (++reps > 10000)
			{
				g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS: couldn't restore buffer\n");
				Shutdown();
				return SIS_FAILURE;
			}

		}

		memset(lpData, 0, dwSize);
	//		lpData[4] = lpData[5] = 0x7f;	// force a pop for debugging

		pDSBuf->Unlock(lpData, dwSize, NULL, 0);

		/* we don't want anyone to access the buffer directly w/o locking it first. */
		lpData = NULL; 

		pDSBuf->Stop();
		pDSBuf->GetCurrentPosition(&m_mmstarttime.u.sample, &dwWrite);
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);
	}

	m_deviceSampleCount = m_bufferSize/(DeviceSampleBytes());
	// UNDONE: Do I need this?
	//shm->buffer = (unsigned char *) lpData;

	return SIS_SUCCESS;
}


/*
  This routine is used to intercept SNDDMA_GetDMAPos. We need to return the total
  number based on both buffers so the rest of the framework behaves the same.
*/
int CAudioDirectSound::GetOutputPositionSurround()
{
	DWORD dwPos;
	DWORD dwRtn;

	pDSBufFL->GetCurrentPosition(&dwPos, NULL);
	// Causing problems on some EAX cards
#if 0
	pDSBufFR->SetCurrentPosition(dwPos);
	pDSBufRL->SetCurrentPosition(dwPos);
	pDSBufRR->SetCurrentPosition(dwPos);
#endif

	dwPos -= dwSurroundStart;
	dwRtn = dwPos << 1;
	dwRtn >>= SAMPLE_16BIT_SHIFT;
	dwRtn &= DeviceSampleCount() - 1;
	return (int)dwRtn;
}

/*
 Release all Surround buffer pointers
*/
void ReleaseSurround(void)
{
	if ( pDSPropSetFL != NULL ) pDSPropSetFL->Release();
	pDSPropSetFL = NULL;

	if ( pDSPropSetFR != NULL ) pDSPropSetFR->Release();
	pDSPropSetFR = NULL;

	if ( pDSPropSetRL != NULL ) pDSPropSetRL->Release();
	pDSPropSetRL = NULL;

	if ( pDSPropSetRR != NULL ) pDSPropSetRR->Release();
	pDSPropSetRR = NULL;

	if ( pDSBuf3DFL != NULL ) pDSBuf3DFL->Release();
	pDSBuf3DFL = NULL;

	if ( pDSBuf3DFR != NULL ) pDSBuf3DFR->Release();
	pDSBuf3DFR = NULL;

	if ( pDSBuf3DRL != NULL ) pDSBuf3DRL->Release();
	pDSBuf3DRL = NULL;

	if ( pDSBuf3DRR != NULL ) pDSBuf3DRR->Release();
	pDSBuf3DRR = NULL;

	if ( pDSBufFL != NULL ) pDSBufFL->Release();
	pDSBufFL = NULL;

	if ( pDSBufFR != NULL ) pDSBufFR->Release();
	pDSBufFR = NULL;

	if ( pDSBufRL != NULL ) pDSBufRL->Release();
	pDSBufRL = NULL;

	if ( pDSBufRR != NULL ) pDSBufRR->Release();
	pDSBufRR = NULL;
}


// Initialization for Surround sound support (4 channel). 
// Creates 4 mono 3D buffers to be used as Front Left, Front Right, Rear Left, Rear Right
 
bool CAudioDirectSound::SNDDMA_InitSurround(LPDIRECTSOUND lpDS, WAVEFORMATEX* lpFormat, DSBCAPS* lpdsbc)
{
	DSBUFFERDESC	dsbuf;
	WAVEFORMATEX wvex;
	DWORD dwSize, dwWrite;
	int reps;
	HRESULT hresult;
	void			*lpData = NULL;

	if ( lpDS == NULL ) return FALSE;
 
	// Force format to mono channel

	memcpy(&wvex, lpFormat, sizeof(WAVEFORMATEX));
	wvex.nChannels = 1;
	wvex.nBlockAlign = wvex.nChannels * wvex.wBitsPerSample / 8;
	wvex.nAvgBytesPerSec = wvex.nSamplesPerSec	* wvex.nBlockAlign; 

	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);
														// LOCHARDWARE causes SB AWE64 to crash in it's DSOUND driver
	dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY|DSBCAPS_CTRL3D;//|DSBCAPS_LOCHARDWARE;
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE/2;
	dsbuf.lpwfxFormat = &wvex;

	// create 4 mono buffers FL, FR, RL, RR

	if (DS_OK != lpDS->CreateSoundBuffer(&dsbuf, &pDSBufFL, NULL))
	{
		g_pSoundServices->ConSafePrint ("DS:CreateSoundBuffer for 3d front left failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != lpDS->CreateSoundBuffer(&dsbuf, &pDSBufFR, NULL))
	{
		g_pSoundServices->ConSafePrint ("DS:CreateSoundBuffer for 3d front right failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != lpDS->CreateSoundBuffer(&dsbuf, &pDSBufRL, NULL))
	{
		g_pSoundServices->ConSafePrint ("DS:CreateSoundBuffer for 3d rear left failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != lpDS->CreateSoundBuffer(&dsbuf, &pDSBufRR, NULL))
	{
		g_pSoundServices->ConSafePrint ("DS:CreateSoundBuffer for 3d rear right failed");
		ReleaseSurround();
		return FALSE;
	}

	// Try to get 4 3D buffers from the mono DS buffers

	if (DS_OK != pDSBufFL->QueryInterface(IID_IDirectSound3DBufferDef, (void**)&pDSBuf3DFL))
	{
		g_pSoundServices->ConSafePrint ("DS:Query 3DBuffer for 3d front left failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != pDSBufFR->QueryInterface(IID_IDirectSound3DBufferDef, (void**)&pDSBuf3DFR))
	{
		g_pSoundServices->ConSafePrint ("DS:Query 3DBuffer for 3d front right failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != pDSBufRL->QueryInterface(IID_IDirectSound3DBufferDef, (void**)&pDSBuf3DRL))
	{
		g_pSoundServices->ConSafePrint ("DS:Query 3DBuffer for 3d rear left failed");
		ReleaseSurround();
		return FALSE;
	}

	if (DS_OK != pDSBufRR->QueryInterface(IID_IDirectSound3DBufferDef, (void**)&pDSBuf3DRR))
	{
		g_pSoundServices->ConSafePrint ("DS:Query 3DBuffer for 3d rear right failed");
		ReleaseSurround();
		return FALSE;
	}

	// UNDONE: set speaker configuration to 2, 4 or headphone
	//lpDS->lpVtbl->SetSpeakerConfig(lpDS, DSSPEAKER_QUAD);

	// Position the 4 3D buffers to FL, FR, RL, RR

	pDSBuf3DFL->SetPosition(-1.0f, 0.0f, 1.0f, DS3D_IMMEDIATE);
	pDSBuf3DFR->SetPosition(1.0f, 0.0f, 1.0f, DS3D_IMMEDIATE);
	pDSBuf3DRL->SetPosition(-1.0f, 0.0f, -1.0f, DS3D_IMMEDIATE);
	pDSBuf3DRR->SetPosition(1.0f, 0.0f, -1.0f, DS3D_IMMEDIATE);

#if 0 
	// try to get EAX fx on the 4 buffers (unused)
	ULONG ulAnswer;
	
	ulAnswer = 0;
	if ( (pDSBufFL->QueryInterface(IID_IKsPropertySetDef, (LPVOID *)&pDSPropSetFL)) == DS_OK )
	{
		if ( (pDSPropSetFL->QuerySupport(DSPROPSETID_EAX_ReverbPropertiesDef, DSPROPERTY_EAX_ALL, &ulAnswer)) == DS_OK )
		{
			if ( (ulAnswer & KSPROPERTY_SUPPORT_SET|KSPROPERTY_SUPPORT_GET) != (KSPROPERTY_SUPPORT_SET|KSPROPERTY_SUPPORT_GET) )
			{
				g_pSoundServices->ConSafePrint ("The current device is not EAX capable.");
				ReleaseSurround();
				return FALSE;
			}
		}
		else
		{
			g_pSoundServices->ConSafePrint ("EAX:QuerySupport for DSPROPSETID_EAX_ReverbProperties Failed\n");
			ReleaseSurround();
			return FALSE;
		}
	}
	else
	{
		g_pSoundServices->ConSafePrint ("EAX:QueryInterface for IID_IKsPropertySet FL Failed");
		ReleaseSurround();
		return FALSE;
	}

	if ( (pDSBufFR->QueryInterface(IID_IKsPropertySetDef, (LPVOID *)&pDSPropSetFR)) != DS_OK )
	{
		g_pSoundServices->ConSafePrint ("EAX:QueryInterface for IID_IKsPropertySet FR Failed");
		ReleaseSurround();
		return FALSE;
	}

	if ( (pDSBufRL->QueryInterface(IID_IKsPropertySetDef, (LPVOID *)&pDSPropSetRL)) != DS_OK )
	{
		g_pSoundServices->ConSafePrint ("EAX:QueryInterface for IID_IKsPropertySet RL Failed");
		ReleaseSurround();
		return FALSE;
	}

	if ( (pDSBufRR->QueryInterface(IID_IKsPropertySetDef, (LPVOID *)&pDSPropSetRR)) != DS_OK )
	{
		g_pSoundServices->ConSafePrint ("EAX:QueryInterface for IID_IKsPropertySet RR Failed");
		ReleaseSurround();
		return FALSE;
	}

#endif // 0

	m_deviceChannels = lpFormat->nChannels;
	m_deviceSampleBits = lpFormat->wBitsPerSample;
	m_deviceDmaSpeed = lpFormat->nSamplesPerSec;

	memset(lpdsbc, 0, sizeof(DSBCAPS));
	lpdsbc->dwSize = sizeof(DSBCAPS);

	if (DS_OK != pDSBufFL->GetCaps (lpdsbc))
	{
		g_pSoundServices->ConSafePrint ("DS:GetCaps failed for 3d sound buffer\n");
		ReleaseSurround();
		return FALSE;
	}

	pDSBufFL->Play(0, 0, DSBPLAY_LOOPING);
	pDSBufFR->Play(0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime)
		DevMsg("   %d channel(s)\n"
					"   %d bits/sample\n"
					"   %d samples/sec\n",
					DeviceChannels(), DeviceSampleBits(), DeviceDmaSpeed());

	m_bufferSize = lpdsbc->dwBufferBytes*2;

	// Test everything just like in the normal initialization.
	reps = 0;
	while ((hresult = pDSBufFL->Lock(0, lpdsbc->dwBufferBytes, (void**)&lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitSurround: DS::Lock Sound Buffer Failed for 3d FL\n");
			ReleaseSurround();
			return FALSE;
		}

		if (++reps > 10000)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitSurround: DS: couldn't restore buffer for 3d FL\n");
			ReleaseSurround();
			return FALSE;
		}
	}
	memset(lpData, 0, dwSize);
	pDSBufFL->Unlock(lpData, dwSize, NULL, 0);

	reps = 0;
	while ((hresult = pDSBufFR->Lock(0, lpdsbc->dwBufferBytes, (void**)&lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitSurround: DS::Lock Sound Buffer Failed for 3d FR\n");
			ReleaseSurround();
			return FALSE;
		}

		if (++reps > 10000)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitSurround: DS: couldn't restore buffer for FR\n");
			ReleaseSurround();
			return FALSE;
		}
	}
	memset(lpData, 0, dwSize);
	pDSBufFR->Unlock(lpData, dwSize, NULL, 0);

	reps = 0;
	while ((hresult = pDSBufRL->Lock(0, lpdsbc->dwBufferBytes, (void**)&lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed for RL\n");
			ReleaseSurround();
			return FALSE;
		}

		if (++reps > 10000)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS: couldn't restore buffer for RL\n");
			ReleaseSurround();
			return FALSE;
		}
	}
	memset(lpData, 0, dwSize);
	pDSBufRL->Unlock(lpData, dwSize, NULL, 0);

	reps = 0;
	while ((hresult = pDSBufRR->Lock(0, lpdsbc->dwBufferBytes, (void**)&lpData, &dwSize, NULL, NULL, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS::Lock Sound Buffer Failed for RR\n");
			ReleaseSurround();
			return FALSE;
		}

		if (++reps > 10000)
		{
			g_pSoundServices->ConSafePrint ("SNDDMA_InitDirect: DS: couldn't restore buffer for RR\n");
			ReleaseSurround();
			return FALSE;
		}
	}
	memset(lpData, 0, dwSize);
	pDSBufRR->Unlock(lpData, dwSize, NULL, 0);
	lpData = NULL; // this is invalid now

	// OK Stop and get our positions and were good to go.
	pDSBufFL->Stop();
	pDSBufFR->Stop();
	pDSBufRL->Stop();
	pDSBufRR->Stop();
	pDSBufFL->GetCurrentPosition(&dwSurroundStart, &dwWrite);
	pDSBufFR->SetCurrentPosition(dwSurroundStart);
	pDSBufRL->SetCurrentPosition(dwSurroundStart);
	pDSBufRR->SetCurrentPosition(dwSurroundStart);
	pDSBufFL->Play(0, 0, DSBPLAY_LOOPING);
	pDSBufFR->Play(0, 0, DSBPLAY_LOOPING);
	pDSBufRL->Play(0, 0, DSBPLAY_LOOPING);
	pDSBufRR->Play(0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime)
		g_pSoundServices->ConSafePrint ("3d surround sound initialization successful\n");

	return TRUE;
}

void CAudioDirectSound::UpdateListener( const Vector& position, const Vector& forward, const Vector& right, const Vector& up )
{
	VectorCopy( forward, m_listenerForward );
}

void CAudioDirectSound::MixBegin( int sampleCount )
{
	MIX_ClearAllPaintBuffers( sampleCount, false );
}

void CAudioDirectSound::MixUpsample( int sampleCount, int filtertype )
{
	
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();
	int ifilter = ppaint->ifilter;

	Assert (ifilter < CPAINTFILTERS);

	S_MixBufferUpsample2x( sampleCount, ppaint->pbuf, &(ppaint->fltmem[ifilter][0]), CPAINTFILTERMEM, filtertype );

	if ( ppaint->fsurround )
	{
		Assert( ppaint->pbufrear );
		S_MixBufferUpsample2x( sampleCount, ppaint->pbufrear, &(ppaint->fltmemrear[ifilter][0]), CPAINTFILTERMEM, filtertype );
	}

	// make sure on next upsample pass for this paintbuffer, new filter memory is used

	ppaint->ifilter++;
}



// UNDONE: Implement fast 3D mixing routines instead of doing 2D mix times 2 ??
void CAudioDirectSound::Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];		// fl, fr, rl, rr

	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 1);
	
	Mix8MonoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, (byte *)pData, inputOffset, rateScaleFix, outCount);

	if ( ppaint->fsurround )
	{
		Assert( ppaint->pbufrear );

		Mix8MonoWavtype( pChannel, ppaint->pbufrear + outputOffset, &volume[IREAR_LEFT], (byte *)pData, inputOffset, rateScaleFix, outCount  );
	}
}


void CAudioDirectSound::Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 2 );

	Mix8StereoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, (byte *)pData, inputOffset, rateScaleFix, outCount );


	if ( ppaint->fsurround )
	{
		Assert( ppaint->pbufrear );

		Mix8StereoWavtype( pChannel, ppaint->pbufrear + outputOffset, &volume[IREAR_LEFT], (byte *)pData, inputOffset, rateScaleFix, outCount );
	}
}



void CAudioDirectSound::Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 1 );

	Mix16MonoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, pData, inputOffset, rateScaleFix, outCount );

	if ( ppaint->fsurround )
	{
		Assert( ppaint->pbufrear );

		Mix16MonoWavtype( pChannel, ppaint->pbufrear + outputOffset, &volume[IREAR_LEFT], pData, inputOffset, rateScaleFix, outCount );
	}
}

void CAudioDirectSound::Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 2 );

	Mix16StereoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, pData, inputOffset, rateScaleFix, outCount );

	if ( ppaint->fsurround )
	{
		Assert( ppaint->pbufrear );

		Mix16StereoWavtype( pChannel, ppaint->pbufrear  + outputOffset, &volume[IREAR_LEFT], pData, inputOffset, rateScaleFix, outCount );
	}
}


void CAudioDirectSound::ChannelReset( int entnum, int channelIndex, float distanceMod )
{
}


const char *CAudioDirectSound::DeviceName( void )
{ 
	if ( SURROUND_ON )
		return "4 Channel Surround";
	return "Direct Sound"; 
}

void *CAudioDirectSound::DeviceLockBuffer( void )
{
	int reps = 0;
	HRESULT hresult;
	void *pbuf = NULL, *pbuf2 = NULL;
	DWORD dwSize2;

 //////////////////////////////////////////////////////////////////////////////////////////////////
	if (pDSBuf)
	{
		while ((hresult = pDSBuf->Lock(0, m_bufferSize, (void**)&pbuf, &m_lockBufferSize, 
									   (void**)&pbuf2, &dwSize2, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Msg ("DS::Lock Sound Buffer Failed\n");
				S_Shutdown ();
				S_Startup ();
				return NULL;
			}

			if (++reps > 10000)
			{
				Msg ("DS: couldn't restore buffer\n");
				S_Shutdown ();
				S_Startup ();
				return NULL;
			}
		}
	}

	return pbuf;
}



void CAudioDirectSound::DeviceUnlockBuffer( void *pbuf )
{
	if (pDSBuf)
		pDSBuf->Unlock(pbuf, m_lockBufferSize, NULL, 0);
}

void CAudioDirectSound::TransferSamples( int end )
{
	// When Surround is enabled, divert to 4 chan xfer scheme.
	if ( SURROUND_ON )
	{
		int		lpaintedtime;
		int		endtime;

		lpaintedtime = paintedtime; 
		endtime = end;				

		S_TransferStereo16Surround( PAINTBUFFER, REARPAINTBUFFER, lpaintedtime, endtime);
		return;
	}
	else
	{
		if ( DeviceChannels() == 2 && DeviceSampleBits() == 16 )
		{
			S_TransferStereo16( end );
		}
		else
		{
			S_TransferPaintBuffer( end );
		}
	}
}

// given unit vector from listener to sound source,
// determine proportion of volume for sound in FL, FR, RL, RR quadrants
// Scale this proportion by the distance scalar 'gain'
// NOTE: the sum of the quadrant volumes should be approximately 1.0!

void CAudioDirectSound::SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront )
{
	float scale, lscale, rscale;
	float rfscale, rrscale, lfscale, lrscale;
	float frontscale, rearscale;
	
	if (DeviceChannels() == 1)
	{
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 1.0 + dotRight;			// 0.0->2.0,  2.0 is source fully right		
		lscale = 1.0 - dotRight;			// 2.0->0.0,  2.0 is source fully left
	}

	if ( SURROUND_ON )
	{
		
		frontscale = 1.0 + dotFront;		// 0.0->2.0, 2.0 is source fully front
		rearscale  = 1.0 - dotFront;		// 2.0->0.0, 2.0 is source fully rear

		rfscale = (rscale * frontscale)/2;	// 0.0->2.0, 2.0 is source fully right front
		rrscale = (rscale * rearscale )/2;	// 0.0->2.0, 2.0 is source fully right rear
		lfscale = (lscale * frontscale)/2;	// 0.0->2.0, 2.0 is source fully left front
		lrscale = (lscale * rearscale )/2;	// 0.0->2.0, 2.0 is source fully left rear

		//DevMsg("lfscale=%f rfscale=%f lrscale=%f rrscale=%f\n",lfscale,rfscale,lrscale,rrscale);

		rscale = rfscale;
		lscale = lfscale;
	}
	else
	{
		rrscale = lrscale = 0;	// Compiler warning..
	}

	// scale volumes in each quadrant by distance attenuation.

	// volumes are 0-255:
	// gain is 0.0->1.0, rscale is 0.0->2.0, so scale/2 is 0.0->1.0
	// master_vol is 0->255, so rightvol is 0->255

	scale = gain * rscale / 2.0;
	volume[IFRONT_RIGHT] = (int) (master_vol * scale);

	scale = gain * lscale / 2.0;
	volume[IFRONT_LEFT] = (int) (master_vol * scale);

	if ( SURROUND_ON )
	{
		scale = gain * rrscale / 2.0;
		volume[IREAR_RIGHT] = (int) (master_vol * scale);

		scale = gain * lrscale / 2.0;
		volume[IREAR_LEFT] = (int) (master_vol * scale);
	}
	
	volume[IFRONT_RIGHT] = clamp( volume[IFRONT_RIGHT], 0, 255 );
	volume[IFRONT_LEFT]  = clamp( volume[IFRONT_LEFT], 0, 255 );

	if ( SURROUND_ON )
	{
		volume[IREAR_RIGHT] = clamp( volume[IREAR_RIGHT], 0, 255 );
		volume[IREAR_LEFT] = clamp( volume[IREAR_LEFT], 0, 255 );
	}
}


void CAudioDirectSound::ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount)
{


#if 0
	
	// pDSPropSetFL is only valid if EAX is enabled. Use EAX reverbs instead and bypass
	// all the sw rendering.

	// EAX dsp fx currently disabled.
	float fMixLevel;
	int i;
	float roomType;
	HRESULT hResult;

	if ( pDSPropSetFL )
	{
		if ( g_pClientSidePrediction->GetWaterLevel() > 2 )
		{
			roomType = sxroom_type.GetFloat();
			fMixLevel = 1.0f;
		}
		else
		{
			roomType = sxroomwater_type.GetFloat();
			fMixLevel = 0.38f;
		}

		if (roomType != sxroom_typeprev) 
		{
			i = (int)(roomType);
			
			// set reverb params for all 4 mono EAX buffers FL, FR, RL, RR

			sxroom_typeprev = roomType;
			pDSPropSetFL->Set(DSPROPSETID_EAXBUFFER_ReverbPropertiesDef, 
								DSPROPERTY_EAXBUFFER_REVERBMIX, NULL, 0, &fMixLevel, sizeof(float));
			pDSPropSetFR->Set(DSPROPSETID_EAXBUFFER_ReverbPropertiesDef, 
								DSPROPERTY_EAXBUFFER_REVERBMIX, NULL, 0, &fMixLevel, sizeof(float));
			pDSPropSetRL->Set(DSPROPSETID_EAXBUFFER_ReverbPropertiesDef, 
								DSPROPERTY_EAXBUFFER_REVERBMIX, NULL, 0, &fMixLevel, sizeof(float));
			pDSPropSetRR->Set(DSPROPSETID_EAXBUFFER_ReverbPropertiesDef, 
								DSPROPERTY_EAXBUFFER_REVERBMIX, NULL, 0, &fMixLevel, sizeof(float));
			hResult = pDSPropSetFL->Set(DSPROPSETID_EAX_ReverbPropertiesDef, 
								DSPROPERTY_EAX_ALL, NULL, 0, &eax_preset[i], 
								sizeof(EAX_REVERBPROPERTIES));
			if ( hResult != DS_OK )
				DevMsg("Eax failed preset %d\n", i);
			else
				DevMsg("Eax preset %d\n", i);

			// Need to save this to restore preset in a restart.
			g_iEaxPreset = i;
		}
		return;
	}
#endif // 0

	//SX_RoomFX( endtime, filter, timefx );

	DEBUG_StartSoundMeasure( 1, samplecount );

	DSP_Process( idsp, pbuffront, pbufrear, samplecount );

	DEBUG_StopSoundMeasure( 1, samplecount );
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Given front and rear stereo paintbuffers, split samples into 4 mono directsound buffers (FL, FR, RL, RR)
//
void CAudioDirectSound::S_TransferStereo16Surround( portable_samplepair_t *pfront, portable_samplepair_t *prear, int lpaintedtime, int endtime )
{
	int		lpos;
	HRESULT hr;
	DWORD *pdwWriteFL, *pdwWriteFR, *pdwWriteRL, *pdwWriteRR;
	DWORD dwSizeFL, dwSizeFR, dwSizeRL, dwSizeRR;
	int reps;
	int i, val, *snd_p, *snd_rp, linearCount, volumeFactor;
	short	*snd_out_fleft, *snd_out_fright, *snd_out_rleft, *snd_out_rright;

	snd_p = (int *)pfront;
	volumeFactor = S_GetMasterVolume() * 256;

	// lock all 4 mono directsound buffers FL, FR, RL, RR

	reps = 0;
	while ((hr = pDSBufFL->Lock(0, m_bufferSize/2, (void**)&pdwWriteFL, &dwSizeFL, 
		NULL, NULL, 0)) != DS_OK)
	{
		if (hr != DSERR_BUFFERLOST)
		{
			Msg ("S_TransferStereo16Surround: DS::Lock Sound Buffer Failed FL\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}

		if (++reps > 10000)
		{
			Msg ("S_TransferStereo16Surround: DS: couldn't restore buffer FL\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}
	}

	reps = 0;
	while ((hr = pDSBufFR->Lock(0, m_bufferSize/2, (void**)&pdwWriteFR, &dwSizeFR, 
	NULL, NULL, 0)) != DS_OK)
	{
		if (hr != DSERR_BUFFERLOST)
		{
			Msg ("S_TransferStereo16Surround: DS::Lock Sound Buffer Failed FR\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}

		if (++reps > 10000)
		{
			Msg ("S_TransferStereo16Surround: DS: couldn't restore buffer FR\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}
	}

	reps = 0;
	while ((hr = pDSBufRL->Lock(0, m_bufferSize/2, (void**)&pdwWriteRL, &dwSizeRL, 
	NULL, NULL, 0)) != DS_OK)
	{
		if (hr != DSERR_BUFFERLOST)
		{
			Msg ("S_TransferStereo16Surround: DS::Lock Sound Buffer Failed RL\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}

		if (++reps > 10000)
		{
			Msg ("S_TransferStereo16Surround: DS: couldn't restore buffer RL\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}
	}

	reps = 0;
	while ((hr = pDSBufRR->Lock(0, m_bufferSize/2, (void**)&pdwWriteRR, &dwSizeRR, 
		NULL, NULL, 0)) != DS_OK)
	{
		if (hr != DSERR_BUFFERLOST)
		{
			Msg ("S_TransferStereo16Surround: DS::Lock Sound Buffer Failed RR\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}

		if (++reps > 10000)
		{
			Msg ("S_TransferStereo16Surround: DS: couldn't restore buffer RR\n");
			S_Shutdown ();
			S_Startup ();
			return;
		}
	}

	// take stereo front and rear paintbuffers and copy samples into the 4
	// mono directsound buffers

	snd_rp = (int *)prear;

	int samplePairCount = (DeviceSampleCount()>>1);
	int sampleMask = samplePairCount - 1;
	while (lpaintedtime < endtime)
	{														
		lpos = lpaintedtime & sampleMask;

		linearCount = samplePairCount - lpos;		

		if (linearCount > endtime - lpaintedtime)		
		linearCount = endtime - lpaintedtime;		

		snd_out_fleft = (short *)pdwWriteFL + lpos;
		snd_out_fright = (short *)pdwWriteFR + lpos;
		snd_out_rleft = (short *)pdwWriteRL + lpos;
		snd_out_rright = (short *)pdwWriteRR + lpos;

		linearCount <<= 1;								

		// for 16 bit sample in the front and rear stereo paintbuffers, copy
		// into the 4 FR, FL, RL, RR directsound paintbuffers

		for (i=0 ; i<linearCount ; i+=2)
		{
			val = (snd_p[i]*volumeFactor)>>8;
			snd_out_fleft[i>>1] = CLIP(val);

			val = (snd_p[i + 1]*volumeFactor)>>8;
			snd_out_fright[i>>1] = CLIP(val);

			val = (snd_rp[i]*volumeFactor)>>8;
			snd_out_rleft[i>>1] = CLIP(val);

			val = (snd_rp[i + 1]*volumeFactor)>>8;
			snd_out_rright[i>>1] = CLIP(val);
		}

		snd_p += linearCount;
		snd_rp += linearCount;
		lpaintedtime += (linearCount>>1);
	}

	pDSBufFL->Unlock(pdwWriteFL, dwSizeFL, NULL, 0);
	pDSBufFR->Unlock(pdwWriteFR, dwSizeFR, NULL, 0);
	pDSBufRL->Unlock(pdwWriteRL, dwSizeRL, NULL, 0);
	pDSBufRR->Unlock(pdwWriteRR, dwSizeRR, NULL, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

/*
==================
LPDIRECTSOUND S_GetDSPointer

Returns a pointer to pDS, if it exists, NULL otherwise
==================
*/
void S_GetDSPointer(LPDIRECTSOUND *lpDS, LPDIRECTSOUNDBUFFER *lpDSBuf)
{
	*lpDS = pDS;
	*lpDSBuf = pDSBuf;
}

