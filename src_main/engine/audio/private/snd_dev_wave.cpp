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

#include <windows.h>
#include "tier0/dbg.h"
#include "convar.h"
#include "snd_device.h"
#include "sound_private.h"
#include "snd_mix_buf.h"
#include "snd_channels.h"
#include "vstdlib/strtools.h"
#include "SoundService.h"

extern qboolean snd_firsttime;
extern ConVar snd_mixahead;
extern void MIX_ScaleChannelVolume( paintbuffer_t *ppaint, channel_t *pChannel, int volume[CCHANVOLUMES], int mixchans );

// UNDONE: Make these class members
static HGLOBAL		hWaveHdr;
static LPWAVEHDR	lpWaveHdr;
static HWAVEOUT    hWaveOut; 
#if DEAD
static WAVEOUTCAPS	wavecaps;
#endif
static HANDLE		hData;
static HPSTR		lpData;

// 64K is > 1 second at 16-bit, 22050 Hz
// 44k: UNDONE - need to double buffers now that we're playing back at 44100?
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400

// NOTE: This only allows 16-bit, stereo wave out
class CAudioWave : public IAudioDevice
{
public:

	bool		IsActive( void );
	bool		Init( void );
	void		Shutdown( void );
	void		PaintEnd( void );
	int			GetOutputPosition( void );
	void		ChannelReset( int entnum, int channelIndex, float distanceMod );
	void		Pause( void );
	void		UnPause( void );
	float		MixDryVolume( void );
	bool		Should3DMix( void );
	void		StopAllSounds( void );

	int			PaintBegin( int soundtime, int paintedtime );
	void		ClearBuffer( void );
	void		UpdateListener( const Vector& position, const Vector& forward, const Vector& right, const Vector& up );
	void		MixBegin( int sampleCount );
	void		MixUpsample( int sampleCount, int filtertype );
	void		Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );
	void		Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress );

	void		TransferSamples( int end );
	void		SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront  );
	void		ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount );

	const char *DeviceName( void )			{ return "Windows WAVE"; }
	int			DeviceChannels( void )		{ return 2; }
	int			DeviceSampleBits( void )	{ return 16; }
	int			DeviceSampleBytes( void )	{ return 2; }
	int			DeviceDmaSpeed( void )		{ return SOUND_DMA_SPEED; }
	int			DeviceSampleCount( void )	{ return m_deviceSampleCount; }

	void		*DeviceLockBuffer( void );
	void		DeviceUnlockBuffer( void *pbuffer );

private:
	int			m_deviceSampleCount;
	void		*m_pBuffer;

	int			m_buffersSent;
	int			m_buffersCompleted;
	int			m_pauseCount;
};


IAudioDevice *Audio_CreateWaveDevice( void )
{
	static CAudioWave *wave = NULL;

	if ( !wave )
		wave = new CAudioWave;

	if ( wave->Init() )
		return wave;

	delete wave;
	wave = NULL;

	return NULL;
}

bool CAudioWave::Init( void )
{
	WAVEFORMATEX  format;
	int				i, bufferSize;
	HRESULT			hr;
	
	m_buffersSent = 0;
	m_buffersCompleted = 0;
	m_pauseCount = 0;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = DeviceChannels();
	format.wBitsPerSample = DeviceSampleBits();
	format.nSamplesPerSec = DeviceDmaSpeed();
	format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec
		*format.nBlockAlign; 
	
	/* Open a waveform device for output using window callback. */ 
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER, 
					&format, 
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			DevMsg ("waveOutOpen failed\n");
			return false;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
					    "Select Retry to try to start sound again or Cancel to run Half-Life with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			g_pSoundServices->ConSafePrint ("waveOutOpen failure;\n"
							"  hardware already in use\n");
			return false;
		}
	} 

	/* 
	 * Allocate and lock memory for the waveform data. The memory 
	 * for waveform data must be globally allocated with 
	 * GMEM_MOVEABLE and GMEM_SHARE flags. 

	*/ 
	bufferSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, bufferSize); 
	if (!hData) 
	{ 
		g_pSoundServices->ConSafePrint ("Sound: Out of memory.\n");
		Shutdown();
		return false; 
	}
	lpData = (char *)GlobalLock(hData);
	if (!lpData)
	{ 
		g_pSoundServices->ConSafePrint ("Sound: Failed to lock.\n");
		Shutdown();
		return false; 
	} 
	memset (lpData, 0, bufferSize);

	/* 
	 * Allocate and lock memory for the header. This memory must 
	 * also be globally allocated with GMEM_MOVEABLE and 
	 * GMEM_SHARE flags. 
	 */ 
	hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, 
		(DWORD) sizeof(WAVEHDR) * WAV_BUFFERS); 

	if (hWaveHdr == NULL)
	{ 
		g_pSoundServices->ConSafePrint ("Sound: Failed to Alloc header.\n");
		Shutdown();
		return false; 
	} 

	lpWaveHdr = (LPWAVEHDR) GlobalLock(hWaveHdr); 

	if (lpWaveHdr == NULL)
	{ 
		g_pSoundServices->ConSafePrint ("Sound: Failed to lock header.\n");
		Shutdown();
		return false; 
	}

	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */ 
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE; 
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			g_pSoundServices->ConSafePrint ("Sound: failed to prepare wave headers\n");
			Shutdown();
			return false;
		}
	}

	m_deviceSampleCount = bufferSize/(DeviceSampleBytes());
	
	m_pBuffer = (void *)lpData;

	if (snd_firsttime)
		DevMsg ("Wave sound initialized\n");

	return true;
}


void CAudioWave::Shutdown( void )
{
	int		i;

	/////////////////////////////////////////////////////////////////////////////////////////////
	if (hWaveOut)
	{
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			for (i=0 ; i< WAV_BUFFERS ; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		waveOutClose (hWaveOut);

		if (hWaveHdr)
		{
			GlobalUnlock(hWaveHdr); 
			GlobalFree(hWaveHdr);
		}

		if (hData)
		{
			GlobalUnlock(hData);
			GlobalFree(hData);
		}

	}

	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
}


void CAudioWave::PaintEnd( void )
{
	LPWAVEHDR	h;
	int			wResult;
	int			cblocks;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( m_buffersCompleted == m_buffersSent )
		{
			DevMsg ("Sound overrun\n");
			break;
		}

		if ( ! (lpWaveHdr[ m_buffersCompleted & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		m_buffersCompleted++;	// this buffer has been played
	}

	//
	// submit a few new sound blocks
	//
	// 22K sound support
	// 44k: UNDONE - double blocks out now that we're at 44k playback? 
	cblocks = 4 << 1; 

	while (((m_buffersSent - m_buffersCompleted) >> SAMPLE_16BIT_SHIFT) < cblocks)
	{
		h = lpWaveHdr + ( m_buffersSent&WAV_MASK );

		m_buffersSent++;
		/* 
		 * Now the data block can be sent to the output device. The 
		 * waveOutWrite function returns immediately and waveform 
		 * data is sent to the output device in the background. 
		 */ 
		wResult = waveOutWrite(hWaveOut, h, sizeof(WAVEHDR)); 

		if (wResult != MMSYSERR_NOERROR)
		{ 
			g_pSoundServices->ConSafePrint ("Failed to write block to device\n");
			Shutdown();
			return; 
		} 
	}
}

int CAudioWave::GetOutputPosition( void )
{
	int s = m_buffersSent * WAV_BUFFER_SIZE;

	s >>= SAMPLE_16BIT_SHIFT;

	s &= (DeviceSampleCount()-1);

	return s;
}


int CAudioWave::PaintBegin( int soundtime, int paintedtime )
{
	//  soundtime - total samples that have been played out to hardware at dmaspeed
	//  paintedtime - total samples that have been mixed at speed
	//  endtime - target for samples in mixahead buffer at speed

	unsigned int endtime = soundtime + snd_mixahead.GetFloat() * DeviceDmaSpeed();
	
	int samps = DeviceSampleCount() >> (DeviceChannels()-1);

	if ((int)(endtime - soundtime) > samps)
		endtime = soundtime + samps;

	return endtime;
}


void CAudioWave::Pause( void )
{
	m_pauseCount++;

	if (m_pauseCount == 1)
		waveOutReset (hWaveOut);
}


void CAudioWave::UnPause( void )
{
	if ( m_pauseCount > 0 )
		m_pauseCount--;
}

bool CAudioWave::IsActive( void )
{
	if ( m_pauseCount )
		return false;

	return true;
}

float CAudioWave::MixDryVolume( void )
{
	return 0;
}


bool CAudioWave::Should3DMix( void )
{
	return false;
}


void CAudioWave::ClearBuffer( void )
{
	int		clear;

	if ( !m_pBuffer )
		return;

	clear = 0;

	Q_memset(m_pBuffer, clear, DeviceSampleCount() * DeviceSampleBytes() );
}

void CAudioWave::UpdateListener( const Vector& position, const Vector& forward, const Vector& right, const Vector& up )
{
}


void CAudioWave::MixBegin( int sampleCount )
{
	MIX_ClearAllPaintBuffers( sampleCount, false );
}


void CAudioWave::MixUpsample( int sampleCount, int filtertype )
{
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();
	int ifilter = ppaint->ifilter;
	
	Assert (ifilter < CPAINTFILTERS);

	S_MixBufferUpsample2x( sampleCount, ppaint->pbuf, &(ppaint->fltmem[ifilter][0]), CPAINTFILTERMEM, filtertype );

	ppaint->ifilter++;
}


void CAudioWave::Mix8Mono( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 1);

	Mix8MonoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, (byte *)pData, inputOffset, rateScaleFix, outCount );
}


void CAudioWave::Mix8Stereo( channel_t *pChannel, char *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 2 );

	Mix8StereoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, (byte *)pData, inputOffset, rateScaleFix, outCount );
}


void CAudioWave::Mix16Mono( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 1 );

	Mix16MonoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, pData, inputOffset, rateScaleFix, outCount );
}


void CAudioWave::Mix16Stereo( channel_t *pChannel, short *pData, int outputOffset, int inputOffset, fixedint rateScaleFix, int outCount, int timecompress )
{
	int volume[CCHANVOLUMES];
	paintbuffer_t *ppaint = MIX_GetCurrentPaintbufferPtr();

	MIX_ScaleChannelVolume( ppaint, pChannel, volume, 2 );

	Mix16StereoWavtype( pChannel, ppaint->pbuf + outputOffset, volume, pData, inputOffset, rateScaleFix, outCount );
}


void CAudioWave::ChannelReset( int entnum, int channelIndex, float distanceMod )
{
}


void *CAudioWave::DeviceLockBuffer( void )
{
	return m_pBuffer;
}

void CAudioWave::DeviceUnlockBuffer( void *pbuffer )
{
}


void CAudioWave::TransferSamples( int end )
{
	S_TransferStereo16( end );
}

void CAudioWave::SpatializeChannel( int volume[4], int master_vol, const Vector& sourceDir, float gain, float dotRight, float dotFront )
{
	S_SpatializeChannel( volume, master_vol, gain, dotRight );
}

void CAudioWave::StopAllSounds( void )
{
}


void CAudioWave::ApplyDSPEffects( int idsp, portable_samplepair_t *pbuffront, portable_samplepair_t *pbufrear, int samplecount )
{
	//SX_RoomFX( endtime, filter, timefx );
	DSP_Process( idsp, pbuffront, pbufrear, samplecount );
}

/*
==================
S_GetWAVPointer

Returns a pointer to pDS, if it exists, NULL otherwise
==================
*/
void *S_GetWAVPointer(void)
{
	if (hWaveOut)
		return ( void * )&hWaveOut;

	return NULL;
}
