//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Portable code to mix sounds for snd_dma.cpp.
//
//=============================================================================

#define WIN32_LEAN_AND_MEAN
#pragma warning(push, 1)
#include <windows.h>
#include <mmsystem.h>
#pragma warning(pop)

#include "voice.h"
#include "vox_private.h"

#include "tier0/dbg.h"
#include "sound.h"
#include "sound_private.h"
#include "soundflags.h"
#include "snd_device.h"
#include "measure_section.h"
#include "iprediction.h"
#include "client_class.h"
#include "mouthinfo.h"
#include "icliententitylist.h"
#include "snd_wave_temp.h"
#include "snd_mix_buf.h"
#include "snd_env_fx.h"
#include "snd_channels.h"
#include "snd_audio_source.h"
#include "snd_sfx.h"
#include "snd_convars.h"
#include "icliententity.h"
#include "SoundService.h"
#include "commonmacros.h" 

// NOTE: !!!!!! YOU MUST UPDATE SND_MIXA.S IF THIS VALUE IS CHANGED !!!!!
#define SND_SCALE_BITS		7
#define SND_SCALE_SHIFT		(8-SND_SCALE_BITS)
#define SND_SCALE_LEVELS	(1<<SND_SCALE_BITS)

// Used by other C files.
extern "C"
{
	extern void Snd_WriteLinearBlastStereo16 (void);
	extern void SND_PaintChannelFrom8(int *volume, byte *pData8, int endtime);
	extern void SND_PaintChannelFrom8toDry(int *volume, byte *pData8, int count);
	extern int	snd_scaletable[SND_SCALE_LEVELS][256];
	extern int 	*snd_p, snd_linear_count, snd_vol;
	extern short	*snd_out;
};

bool Con_IsVisible( void );
void SND_RecordBuffer( void );

extern int soundtime;
extern char cl_moviename[];

extern int g_SND_VoiceOverdriveInt;

extern ConVar dsp_room;
extern ConVar dsp_water;
extern ConVar dsp_player;
extern ConVar dsp_facingaway;
extern ConVar snd_showstart;

extern bool SURROUND_ON;
extern float DSP_ROOM_MIX;
extern float DSP_NOROOM_MIX;

// UNDONE: dynamically allocate these, 10 paintbuffers = 80k of static data

portable_samplepair_t paintbuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t roombuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t facingbuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t facingawaybuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t drybuffer[(PAINTBUFFER_SIZE+1)];

portable_samplepair_t rearpaintbuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t rearroombuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t rearfacingbuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t rearfacingawaybuffer[(PAINTBUFFER_SIZE+1)];
portable_samplepair_t reardrybuffer[(PAINTBUFFER_SIZE+1)];

// temp paintbuffer - not included in main list of paintbuffers

portable_samplepair_t temppaintbuffer[(PAINTBUFFER_SIZE+1)];

// #define CPAINTBUFFERS	5	defined in snd_mix_buf.h

// array of paintbuffers, 

paintbuffer_t paintbuffers[CPAINTBUFFERS];

#define IPAINTBUFFER			0
#define IROOMBUFFER				1
#define IFACINGBUFFER			2
#define IFACINGAWAYBUFFER		3
#define IDRYBUFFER				4
	
// pointer to current paintbuffer (front and reare), used by all mixing, upsampling and dsp routines

portable_samplepair_t *g_curpaintbuffer;
portable_samplepair_t *g_currearpaintbuffer;	
bool g_bdirectionalfx;
bool g_bDspOff;

// dsp performance timing

unsigned g_snd_call_time_debug = 0;
unsigned g_snd_time_debug = 0;
unsigned g_snd_count_debug = 0;
unsigned g_snd_samplecount = 0;
unsigned g_snd_frametime = 0;
unsigned g_snd_frametime_total = 0;
int	g_snd_profile_type = 0;		// type 1 dsp, type 2 mixer, type 3 load sound, type 4 all sound

#define FILTERTYPE_NONE		0
#define FILTERTYPE_LINEAR	1
#define FILTERTYPE_CUBIC	2

// filter memory for upsampling

portable_samplepair_t cubicfilter1[3] = {{0,0},{0,0},{0,0}};
portable_samplepair_t cubicfilter2[3] = {{0,0},{0,0},{0,0}};

portable_samplepair_t linearfilter1[1] = {0,0};
portable_samplepair_t linearfilter2[1] = {0,0};
portable_samplepair_t linearfilter3[1] = {0,0};
portable_samplepair_t linearfilter4[1] = {0,0};
portable_samplepair_t linearfilter5[1] = {0,0};
portable_samplepair_t linearfilter6[1] = {0,0};
portable_samplepair_t linearfilter7[1] = {0,0};
portable_samplepair_t linearfilter8[1] = {0,0};

int		snd_scaletable[SND_SCALE_LEVELS][256];
int 	*snd_p, snd_linear_count, snd_vol;
short	*snd_out;

float DSP_GetGain( int idsp );

//===============================================================================
// Mix buffer (paintbuffer) management routines
//===============================================================================


#if 0
HPSTR	g_lppaintbufferdata = lpData;		// allocated memory for all paintbuffer data
HANDLE	g_hpaintbufferdata = hData;			// handle to allocated memory for all paintbuffer data
#endif // 0

void MIX_FreeAllPaintbuffers(void)
{

#if 0
	if (g_lppaintbufferdata)
	{
		GlobalUnlock(g_lppaintbufferdata);
		GlobalFree(g_hpaintbufferdata);

		g_lppaintbufferdata = NULL;
		g_hpaintbufferdata = NULL;
	}
#endif // 0
		
	// clear paintbuffer structs

	Q_memset(paintbuffers, 0, CPAINTBUFFERS * sizeof(paintbuffer_t));
}

// Initialize paintbuffers array, set current paint buffer to main output buffer IPAINTBUFFER

bool MIX_InitAllPaintbuffers(void)
{
	bool fsurround;

	// clear paintbuffer structs

	Q_memset(paintbuffers, 0, CPAINTBUFFERS * sizeof(paintbuffer_t));

	fsurround = SURROUND_ON;

#if 0	// UNDONE: alloc & free paintbuffer block from main memory

	HANDLE hData;
	HPSTR lpData;
	HPSTR lp;
	int cbsamples;

	cbsamples = (PAINTBUFFER_SIZE+1) * sizeof(portable_samplepair_t) * CPAINTBUFFERS * 2;  // *2 for front/rear buffers

	hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, cbsamples); 
	if (!hData) 
	{ 
		g_pSoundServices->ConSafePrint ("SOUND MIX: Couldn't alloc paintbuffers, Out of memory.\n");
		return false; 
	}
		
	lpData = (char *)GlobalLock(hData);
	if (!lpData)
	{ 
		g_pSoundServices->ConSafePrint ("SOUND MIX: Failed to lock paintbuffers.\n");
		GlobalFree(hData);		
		return false; 
	}
	
	g_lppaintbufferdata = lpData;
	g_hpaintbufferdata = hData;

	memset (lpData, 0, cbsamples);

	lp = lpData;

	for (i = 0; i < CPAINTBUFFERS; i++)
	{	
		paintbuffers[i].pbuf = lp;
		lp += (PAINTBUFFER_SIZE+1) * sizeof(portable_samplepair_t);

		paintbuffers[i].pbufrear = lp;
		lp += (PAINTBUFFER_SIZE+1) * sizeof(portable_samplepair_t);
	}
	
#endif // 0	

	// front, rear & dry paintbuffers
	
	paintbuffers[IPAINTBUFFER].pbuf				= paintbuffer;
	paintbuffers[IPAINTBUFFER].pbufrear			= rearpaintbuffer;

	paintbuffers[IROOMBUFFER].pbuf				= roombuffer;
	paintbuffers[IROOMBUFFER].pbufrear			= rearroombuffer;

	paintbuffers[IFACINGBUFFER].pbuf			= facingbuffer;
	paintbuffers[IFACINGBUFFER].pbufrear		= rearfacingbuffer;
	
	paintbuffers[IFACINGAWAYBUFFER].pbuf		= facingawaybuffer;
	paintbuffers[IFACINGAWAYBUFFER].pbufrear	= rearfacingawaybuffer;

	paintbuffers[IDRYBUFFER].pbuf				= drybuffer;
	paintbuffers[IDRYBUFFER].pbufrear			= reardrybuffer;
	
	// buffer flags

	paintbuffers[IROOMBUFFER].flags				= SOUND_BUSS_ROOM;
	paintbuffers[IFACINGBUFFER].flags			= SOUND_BUSS_FACING;
	paintbuffers[IFACINGAWAYBUFFER].flags		= SOUND_BUSS_FACINGAWAY;

	
	// buffer surround sound flag

	paintbuffers[IPAINTBUFFER].fsurround		= fsurround;
	paintbuffers[IFACINGBUFFER].fsurround		= fsurround;
	paintbuffers[IFACINGAWAYBUFFER].fsurround	= fsurround;
	paintbuffers[IDRYBUFFER].fsurround			= fsurround;
	
	// room buffer mixes down to mono

	paintbuffers[IROOMBUFFER].fsurround			= false;
	
	MIX_SetCurrentPaintbuffer(IPAINTBUFFER);

	return true;
}



// Transfer paintbuffer into dma buffer

void S_TransferStereo16 (int end)
{
	int		lpos;
	int		lpaintedtime;
	DWORD	*pbuf;
	int		endtime;
	
	snd_vol = S_GetMasterVolume()*256;

	snd_p = (int *) PAINTBUFFER;
	
	lpaintedtime = paintedtime; 
	endtime = end;				


	pbuf = (DWORD *)g_AudioDevice->DeviceLockBuffer();
	if ( !pbuf )
		return;

	int samplePairCount = g_AudioDevice->DeviceSampleCount() >> 1;
	int sampleMask = samplePairCount  - 1;

	while (lpaintedtime < endtime)
	{														
															// pbuf can hold 16384, 16 bit L/R samplepairs.
		// handle recirculating buffer issues				// lpaintedtime - where to start painting into dma buffer. 
															//		(modulo size of dma buffer for current position).
		lpos = lpaintedtime & sampleMask;					// lpos - samplepair index into dma buffer. First samplepair from paintbuffer to be xfered here.

		snd_out = (short *) pbuf + (lpos<<1);				// snd_out - L/R sample index into dma buffer.  First L sample from paintbuffer goes here.

		snd_linear_count = samplePairCount - lpos;			// snd_linear_count - number of samplepairs between end of dma buffer and xfer start index.
		if (snd_linear_count > endtime - lpaintedtime)		// make sure we write at most snd_linear_count, and at least as many samplepairs as we've premixed
			snd_linear_count = endtime - lpaintedtime;		// endtime - lpaintedtime = number of premixed sample pairs ready for xfer.

		snd_linear_count <<= 1;								// snd_linear_count is now number of 16 bit samples (L or R) to xfer.

		// write a linear blast of samples

		SND_RecordBuffer();
 		Snd_WriteLinearBlastStereo16 ();					// transfer 16bit samples from snd_p into snd_out, multiplying each sample by volume.

		snd_p += snd_linear_count;							// advance paintbuffer pointer
		lpaintedtime += (snd_linear_count>>1);				// advance lpaintedtime by number of samplepairs just xfered.

	}

	g_AudioDevice->DeviceUnlockBuffer( pbuf );
}

// Transfer contents of main paintbuffer PAINTBUFFER out to 
// device.  Perform volume multiply on each sample.

void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	out_mask;
	int 	*p;
	int 	step;
	int		val;
	int		snd_vol;
	DWORD	*pbuf;

 
	p = (int *) PAINTBUFFER;

	count = ((endtime - paintedtime) * g_AudioDevice->DeviceChannels()); 
	
	out_mask = g_AudioDevice->DeviceSampleCount() - 1; 

	// 44k: remove old 22k sound support << HISPEED_DMA
	// out_idx = ((paintedtime << HISPEED_DMA) * g_AudioDevice->DeviceChannels()) & out_mask;

	out_idx = (paintedtime * g_AudioDevice->DeviceChannels()) & out_mask;
	
	step = 3 - g_AudioDevice->DeviceChannels();
	snd_vol = S_GetMasterVolume()*256;

	pbuf = (DWORD *)g_AudioDevice->DeviceLockBuffer();

	if ( !pbuf )
		return;

	if (g_AudioDevice->DeviceSampleBits() == 16)
	{
		short *out = (short *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p+= step;
			val = CLIP(val);
			
			out[out_idx] = val;
			out_idx = (out_idx + 1) & out_mask;
		}
	}
	else if (g_AudioDevice->DeviceSampleBits() == 8)
	{
		unsigned char *out = (unsigned char *) pbuf;
		while (count--)
		{
			val = (*p * snd_vol) >> 8;
			p+= step;
			val = CLIP(val);
			
			out[out_idx] = (val>>8) + 128;
			out_idx = (out_idx + 1) & out_mask;
		}
	}

	g_AudioDevice->DeviceUnlockBuffer( pbuf );
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/


// free channel so that it may be allocated by the
// next request to play a sound.  If sound is a 
// word in a sentence, release the sentence.
// Works for static, dynamic, sentence and stream sounds

void S_FreeChannel(channel_t *ch)
{
	ch->isSentence = false;
//	Msg("End sound %s\n", ch->sfx->getname() );
	
	delete ch->pMixer;
	ch->pMixer = NULL;
	ch->sfx = NULL;

	SND_CloseMouth(ch);
}


// Mix all channels into active paintbuffers until paintbuffer is full or 'endtime' is reached.
// endtime: time in 44khz samples to mix
// rate: ignore samples which are not natively at this rate (for multipass mixing/filtering)
//		 if rate == SOUND_ALL_RATES then mix all samples this pass
// flags: if SOUND_MIX_DRY, then mix only samples with channel flagged as 'dry'
// outputRate: target mix rate for all samples.  Note, if outputRate = SOUND_DMA_SPEED, then
//		 this routine will fill the paintbuffer to endtime.  Otherwise, fewer samples are mixed.
//		 if (endtime - paintedtime) is not aligned on boundaries of 4, 
//		 we'll miss data if outputRate < SOUND_DMA_SPEED!
void MIX_MixChannelsToPaintbuffer(int endtime, int flags, int rate, int outputRate)
{
	int		i;
	channel_t *ch;
	int		sampleCount;

	// mix each channel into paintbuffer

	ch = channels;
	
	// validate parameters
	Assert (outputRate <= SOUND_DMA_SPEED);
	Assert (!((endtime - paintedtime) & 0x3) || (outputRate == SOUND_DMA_SPEED)); // make sure we're not discarding data
											  
	// 44k: try to mix this many samples at outputRate
	sampleCount = (endtime - paintedtime) / (SOUND_DMA_SPEED / outputRate);
	
	if ( sampleCount <= 0 )
		return;

	for ( i = 0; i < total_channels ; i++, ch++ )
	{
		if (!ch->sfx) 
		{
			continue;
		}
		
		// if mixing with SOUND_MIX_DRY flag, ignore all channels not flagged as 'dry'

		if ( flags == SOUND_MIX_DRY )
		{
			if (!ch->bdry)
				continue;
		}
		
		// if mixing with SOUND_MIX_WET flag, ignore all channels flagged as 'dry'

		if ( flags == SOUND_MIX_WET )
		{
			if (ch->bdry)
				continue;
		}

		// UNDONE: Can get away with not filling the cache until
		// we decide it should be mixed

		CAudioSource *pSource = S_LoadSound( ch->sfx, ch );

		// Don't mix sound data for sounds with zero volume. If it's a non-looping sound, 
		// just remove the sound when its volume goes to zero.

		bool bZeroVolume = !ch->leftvol && !ch->rightvol && !ch->rleftvol && !ch->rrightvol;
 
		if( !bZeroVolume )
		{
			if( ch->leftvol <= 5 && ch->rightvol <= 5 && ch->rleftvol <= 5 && ch->rrightvol <= 5 )
				bZeroVolume = true;
		}

		if ( !pSource || ( bZeroVolume && !pSource->IsLooped() ) )
		{
			// NOTE: Since we've loaded the sound, check to see if it's a sentence.  Play them at zero anyway
			// to keep the character's lips moving and the captions happening.
			if ( !pSource || pSource->GetSentence() == NULL )
			{
				S_FreeChannel( ch );
				continue;
			}
		}
		else if ( bZeroVolume )
		{
			continue;
		}

		// multipass mixing - only mix samples of specified sample rate

		switch (rate)
		{
		case SOUND_11k:
		case SOUND_22k:
		case SOUND_44k:
			if (rate != pSource->SampleRate())
				continue;
			break;
		default:
		case SOUND_ALL_RATES:
			break;
		}

			
		// get playback pitch
		ch->pitch = ch->pMixer->ModifyPitch( ch->basePitch * 0.01f );

		if (entitylist->GetClientEntity(ch->soundsource) && 
			(ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_STREAM))
		{
			// UNDONE: recode this as a member function of CAudioMixer
			SND_MoveMouth8(ch, pSource, sampleCount);
		}

		// mix channel to all active paintbuffers.
		// NOTE: must be called once per channel only - consecutive calls retrieve additional data.

		ch->pMixer->MixDataToDevice( g_AudioDevice, ch, sampleCount, outputRate, 0 );

		if ( !ch->pMixer->ShouldContinueMixing() )
		{
			S_FreeChannel( ch );
		}
	}
}

// pass in index -1...count+2, return pointer to source sample in either paintbuffer or delay buffer
inline portable_samplepair_t * S_GetNextpFilter(int i, portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem)
{
	// The delay buffer is assumed to precede the paintbuffer by 6 duplicated samples
	if (i == -1)
		return (&(pfiltermem[0]));
	if (i == 0)
		return (&(pfiltermem[1]));
	if (i == 1)
		return (&(pfiltermem[2]));

	// return from paintbuffer, where samples are doubled.  
	// even samples are to be replaced with interpolated value.

	return (&(pbuffer[(i-2)*2 + 1]));
}

// pass forward over passed in buffer and cubic interpolate all odd samples
// pbuffer: buffer to filter (in place)
// prevfilter:  filter memory. NOTE: this must match the filtertype ie: filtercubic[] for FILTERTYPE_CUBIC
//				if NULL then perform no filtering. UNDONE: should have a filter memory array type
// count: how many samples to upsample. will become count*2 samples in buffer, in place.

void S_Interpolate2xCubic( portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem, int cfltmem, int count )
{

// implement cubic interpolation on 2x upsampled buffer.   Effectively delays buffer contents by 2 samples.
// pbuffer: contains samples at 0, 2, 4, 6...
// temppaintbuffer is temp buffer, same size as paintbuffer, used to store processed values
// count: number of samples to process in buffer ie: how many samples at 0, 2, 4, 6...

// finpos is the fractional, inpos the integer part.
//		finpos = 0.5 for upsampling by 2x
//		inpos is the position of the sample

//		xm1 = x [inpos - 1];
//		x0 = x [inpos + 0];
//		x1 = x [inpos + 1];
//		x2 = x [inpos + 2];
//		a = (3 * (x0-x1) - xm1 + x2) / 2;
//		b = 2*x1 + xm1 - (5*x0 + x2) / 2;
//		c = (x1 - xm1) / 2;
//		y [outpos] = (((a * finpos) + b) * finpos + c) * finpos + x0;

	int i, upCount = count << 1;
	int a, b, c;
	int xm1, x0, x1, x2;
	portable_samplepair_t *psamp0;
	portable_samplepair_t *psamp1;
	portable_samplepair_t *psamp2;
	portable_samplepair_t *psamp3;
	int outpos = 0;

	Assert (upCount <= PAINTBUFFER_SIZE);

	// pfiltermem holds 6 samples from previous buffer pass

	// process 'count' samples

	for ( i = 0; i < count; i++)
	{
		
		// get source sample pointer

		psamp0 = S_GetNextpFilter(i-1, pbuffer, pfiltermem);
		psamp1 = S_GetNextpFilter(i,   pbuffer, pfiltermem);
		psamp2 = S_GetNextpFilter(i+1, pbuffer, pfiltermem);
		psamp3 = S_GetNextpFilter(i+2, pbuffer, pfiltermem);

		// write out original sample to interpolation buffer

		temppaintbuffer[outpos++] = *psamp1;

		// get all left samples for interpolation window

		xm1 = psamp0->left;
		x0 = psamp1->left;
		x1 = psamp2->left;
		x2 = psamp3->left;
		
		// interpolate

		a = (3 * (x0-x1) - xm1 + x2) / 2;
		b = 2*x1 + xm1 - (5*x0 + x2) / 2;
		c = (x1 - xm1) / 2;
		
		// write out interpolated sample

		temppaintbuffer[outpos].left = a/8 + b/4 + c/2 + x0;
		
		// get all right samples for window

		xm1 = psamp0->right;
		x0 = psamp1->right;
		x1 = psamp2->right;
		x2 = psamp3->right;
		
		// interpolate

		a = (3 * (x0-x1) - xm1 + x2) / 2;
		b = 2*x1 + xm1 - (5*x0 + x2) / 2;
		c = (x1 - xm1) / 2;
		
		// write out interpolated sample, increment output counter

		temppaintbuffer[outpos++].right = a/8 + b/4 + c/2 + x0;
		
		Assert (outpos <= ARRAYSIZE(temppaintbuffer));
	}
	
	Assert(cfltmem >= 3);

	// save last 3 samples from paintbuffer
	
	pfiltermem[0] = pbuffer[upCount - 5];
	pfiltermem[1] = pbuffer[upCount - 3];
	pfiltermem[2] = pbuffer[upCount - 1];

	// copy temppaintbuffer back into paintbuffer

	for (i = 0; i < upCount; i++)
		pbuffer[i] = temppaintbuffer[i];
}

// pass forward over passed in buffer and linearly interpolate all odd samples
// pbuffer: buffer to filter (in place)
// prevfilter:  filter memory. NOTE: this must match the filtertype ie: filterlinear[] for FILTERTYPE_LINEAR
//				if NULL then perform no filtering.
// count: how many samples to upsample. will become count*2 samples in buffer, in place.

void S_Interpolate2xLinear( portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem, int cfltmem, int count )
{
	int i, upCount = count<<1;

	Assert (upCount <= PAINTBUFFER_SIZE);
	Assert (cfltmem >= 1);

	// use interpolation value from previous mix

	pbuffer[0].left = (pfiltermem->left + pbuffer[0].left) >> 1;
	pbuffer[0].right = (pfiltermem->right + pbuffer[0].right) >> 1;

	for ( i = 2; i < upCount; i+=2)
	{
		// use linear interpolation for upsampling

		pbuffer[i].left = (pbuffer[i].left + pbuffer[i-1].left) >> 1;
		pbuffer[i].right = (pbuffer[i].right + pbuffer[i-1].right) >> 1;
	}

	// save last value to be played out in buffer

	*pfiltermem = pbuffer[upCount - 1]; 
}

// upsample by 2x, optionally using interpolation
// count: how many samples to upsample. will become count*2 samples in buffer, in place.
// pbuffer: buffer to upsample into (in place)
// pfiltermem:  filter memory. NOTE: this must match the filtertype ie: filterlinear[] for FILTERTYPE_LINEAR
//				if NULL then perform no filtering.
// cfltmem: max number of sample pairs filter can use
// filtertype: FILTERTYPE_NONE, _LINEAR, _CUBIC etc.  Must match prevfilter.

void S_MixBufferUpsample2x( int count, portable_samplepair_t *pbuffer, portable_samplepair_t *pfiltermem, int cfltmem, int filtertype )
{
	int i, j, upCount = count<<1;
	
	// reverse through buffer, duplicating contents for 'count' samples

	for (i = upCount - 1, j = count - 1; j >= 0; i-=2, j--)
	{	
		pbuffer[i] = pbuffer[j];
		pbuffer[i-1] = pbuffer[j];
	}
	
	// pass forward through buffer, interpolate all even slots

	switch (filtertype)
	{
	default:
		break;
	case FILTERTYPE_LINEAR:
		S_Interpolate2xLinear(pbuffer, pfiltermem, cfltmem, count);
		break;
	case FILTERTYPE_CUBIC:
		S_Interpolate2xCubic(pbuffer, pfiltermem, cfltmem, count);
		break;
	}
}

//===============================================================================
// PAINTBUFFER ROUTINES
//===============================================================================


// Set current paintbuffer to pbuf.  
// The set paintbuffer is used by all subsequent mixing, upsampling and dsp routines.
// Also sets the rear paintbuffer if paintbuffer has fsurround true.
// (otherwise, rearpaintbuffer is NULL)

inline void MIX_SetCurrentPaintbuffer(int ipaintbuffer)
{
	// set front and rear paintbuffer

	Assert(ipaintbuffer < CPAINTBUFFERS);
	
	g_curpaintbuffer = paintbuffers[ipaintbuffer].pbuf;

	if ( paintbuffers[ipaintbuffer].fsurround )
		g_currearpaintbuffer = paintbuffers[ipaintbuffer].pbufrear;
	else
		g_currearpaintbuffer = NULL;

	Assert(g_curpaintbuffer != NULL);
}

// return index to current paintbuffer

inline int MIX_GetCurrentPaintbufferIndex( void )
{
	int i;

	for (i = 0; i < CPAINTBUFFERS; i++)
	{
		if (g_curpaintbuffer == paintbuffers[i].pbuf)
			return i;
	}

	return 0;
}


// return pointer to current paintbuffer struct

inline paintbuffer_t *MIX_GetCurrentPaintbufferPtr( void )
{
	int ipaint = MIX_GetCurrentPaintbufferIndex();
	
	Assert(ipaint < CPAINTBUFFERS);

	return &paintbuffers[ipaint];
}

// return pointer to front paintbuffer pbuf, given index

inline portable_samplepair_t *MIX_GetPFrontFromIPaint(int ipaintbuffer)
{
	return paintbuffers[ipaintbuffer].pbuf;
}

inline paintbuffer_t *MIX_GetPPaintFromIPaint( int ipaint )
{	
	Assert(ipaint < CPAINTBUFFERS);

	return &paintbuffers[ipaint];
}


// return pointer to rear buffer, given index.
// returns null if fsurround is false;

inline portable_samplepair_t *MIX_GetPRearFromIPaint(int ipaintbuffer)
{
	if ( paintbuffers[ipaintbuffer].fsurround )
		return paintbuffers[ipaintbuffer].pbufrear;

	return NULL;
}

// return index to paintbuffer, given buffer pointer

inline int MIX_GetIPaintFromPFront( portable_samplepair_t *pbuf )
{
	int i;

	for (i = 0; i < CPAINTBUFFERS; i++)
	{
		if (pbuf == paintbuffers[i].pbuf)
			return i;
	}

	return 0;
}

// return pointer to paintbuffer struct, given ptr to buffer data

inline paintbuffer_t *MIX_GetPPaintFromPFront( portable_samplepair_t *pbuf )
{
	int i;
	i = MIX_GetIPaintFromPFront( pbuf );

	return &paintbuffers[i];
}

// Activate a paintbuffer.  All active paintbuffers are mixed in parallel within 
// MIX_MixChannelsToPaintbuffer, according to flags

inline void MIX_ActivatePaintbuffer(int ipaintbuffer)
{
	Assert(ipaintbuffer < CPAINTBUFFERS);
	paintbuffers[ipaintbuffer].factive = true;
}

// Don't mix into this paintbuffer

inline void MIX_DeactivatePaintbuffer(int ipaintbuffer)
{
	Assert(ipaintbuffer < CPAINTBUFFERS);
	paintbuffers[ipaintbuffer].factive = false;
}

// Don't mix into any paintbuffers

inline void MIX_DeactivateAllPaintbuffers(void)
{
	int i;
	for (i = 0; i < CPAINTBUFFERS; i++)
		paintbuffers[i].factive = false;
}

// set upsampling filter indexes back to 0

inline void MIX_ResetPaintbufferFilterCounters( void )

{
	int i;
	for (i = 0; i < CPAINTBUFFERS; i++)
		paintbuffers[i].ifilter = 0;
}

inline void MIX_ResetPaintbufferFilterCounter( int ipaintbuffer )
{
	Assert (ipaintbuffer < CPAINTBUFFERS);
	paintbuffers[ipaintbuffer].ifilter = 0;
}

// Change paintbuffer's flags

inline void MIX_SetPaintbufferFlags(int ipaintbuffer, int flags)
{
	Assert(ipaintbuffer < CPAINTBUFFERS);
	paintbuffers[ipaintbuffer].flags = flags;
}


// zero out all paintbuffers

void MIX_ClearAllPaintBuffers( int SampleCount, bool clearFilters )
{
	int i;
	int count = min(SampleCount, PAINTBUFFER_SIZE);

	// zero out all paintbuffer data (ignore sampleCount)

	for (i = 0; i < CPAINTBUFFERS; i++)
	{
		if (paintbuffers[i].pbuf != NULL)
			Q_memset(paintbuffers[i].pbuf, 0, (count+1) * sizeof(portable_samplepair_t));

		if (paintbuffers[i].pbufrear != NULL)
			Q_memset(paintbuffers[i].pbufrear, 0, (count+1) * sizeof(portable_samplepair_t));

		if ( clearFilters )
		{
			Q_memset( paintbuffers[i].fltmem, 0, sizeof(paintbuffers[i].fltmem) );
			Q_memset( paintbuffers[i].fltmemrear, 0, sizeof(paintbuffers[i].fltmemrear) );
		}
	}

	if ( clearFilters )
	{
		MIX_ResetPaintbufferFilterCounters();
	}
}

#define SWAP(a,b,t)				{(t) = (a); (a) = (b); (b) = (t);}

#define AVG(a,b)			(((a) + (b)) >> 1 )

#define AVG4(a,b,c,d)	(((a) + (b) + (c) + (d)) >> 2 )

// mixes pbuf1 + pbuf2 into pbuf3, count samples
// fgain is output gain 0-1.0
// NOTE: pbuf3 may equal pbuf1 or pbuf2!

// mixing algorithms:

// destination 2ch:
// pb1 2ch		  + pb2 2ch			-> pb3 2ch
// pb1 (4ch->2ch) + pb2 2ch			-> pb3 2ch
// pb1 2ch		  + pb2 (4ch->2ch)	-> pb3 2ch
// pb1 (4ch->2ch) + pb2 (4ch->2ch)	-> pb3 2ch

// destination 4ch:
// pb1 4ch		  + pb2 4ch			-> pb3 4ch
// pb1 (2ch->4ch) + pb2 4ch			-> pb3 4ch
// pb1 4ch		  + pb2 (2ch->4ch)	-> pb3 4ch
// pb1 (2ch->4ch) + pb2 (2ch->4ch)	-> pb3 4ch

// if all buffers are 4 ch surround, mix rear channels into ibuf3 as well.
void MIX_MixPaintbuffers(int ibuf1, int ibuf2, int ibuf3, int count, float fgain)
{
	int i;
	portable_samplepair_t *pbuf1, *pbuf2, *pbuf3, *pbuft;
	portable_samplepair_t *pbufrear1, *pbufrear2, *pbufrear3, *pbufreart;
	int cchan1, cchan2, cchan3, cchant;
	int xl,xr;
	int gain;

	gain = 256 * fgain;
	
	Assert (count <= PAINTBUFFER_SIZE);
	Assert (ibuf1 < CPAINTBUFFERS);
	Assert (ibuf2 < CPAINTBUFFERS);
	Assert (ibuf3 < CPAINTBUFFERS);

	pbuf1 = paintbuffers[ibuf1].pbuf;
	pbuf2 = paintbuffers[ibuf2].pbuf;
	pbuf3 = paintbuffers[ibuf3].pbuf;
	
	pbufrear1 = paintbuffers[ibuf1].pbufrear;
	pbufrear2 = paintbuffers[ibuf2].pbufrear;
	pbufrear3 = paintbuffers[ibuf3].pbufrear;
	
	cchan1 = 2 + (paintbuffers[ibuf1].fsurround ? 2 : 0);
	cchan2 = 2 + (paintbuffers[ibuf2].fsurround ? 2 : 0);
	cchan3 = 2 + (paintbuffers[ibuf3].fsurround ? 2 : 0);

	// make sure pbuf1 always has fewer or equal channels than pbuf2
	// NOTE: pbuf3 may equal pbuf1 or pbuf2!

	if ( cchan2 < cchan1 )
	{
		SWAP( cchan1, cchan2, cchant );
		SWAP( pbuf1, pbuf2, pbuft );
		SWAP( pbufrear1, pbufrear2, pbufreart );
	}

	// UNDONE: implement fast mixing routines for each of the following sections

	// destination buffer stereo - average n chans down to stereo 

	if ( cchan3 == 2 )
	{
		// destination 2ch:
		// pb1 2ch		  + pb2 2ch			-> pb3 2ch
		// pb1 2ch		  + pb2 (4ch->2ch)	-> pb3 2ch
		// pb1 (4ch->2ch) + pb2 (4ch->2ch)	-> pb3 2ch

		if ( cchan1 == 2 && cchan2 == 2 )
		{
			// mix front channels

			for (i = 0; i < count; i++)
			{
				pbuf3[i].left  = pbuf1[i].left  + pbuf2[i].left;
				pbuf3[i].right = pbuf1[i].right + pbuf2[i].right;
			}
			goto gain2ch;
		}

		if ( cchan1 == 2 && cchan2 == 4 )
		{
			// avg rear chan l/r

			for (i = 0; i < count; i++)
			{
				pbuf3[i].left  = pbuf1[i].left  + AVG( pbuf2[i].left,  pbufrear2[i].left );
				pbuf3[i].right = pbuf1[i].right + AVG( pbuf2[i].right, pbufrear2[i].right );
			}
			goto gain2ch;
		}

		if ( cchan1 == 4 && cchan2 == 4 )
		{
			// avg rear chan l/r

			for (i = 0; i < count; i++)
			{
				pbuf3[i].left  = AVG( pbuf1[i].left, pbufrear1[i].left)  + AVG( pbuf2[i].left,  pbufrear2[i].left );
				pbuf3[i].right = AVG( pbuf1[i].right, pbufrear1[i].right) + AVG( pbuf2[i].right, pbufrear2[i].right );
			}
			goto gain2ch;
		}
	
	}
	
	// destination buffer quad - duplicate n chans up to quad

	if ( cchan3 == 4 )
	{
		
		// pb1 4ch		  + pb2 4ch			-> pb3 4ch
		// pb1 (2ch->4ch) + pb2 4ch			-> pb3 4ch
		// pb1 (2ch->4ch) + pb2 (2ch->4ch)	-> pb3 4ch
		
		if ( cchan1 == 4 && cchan2 == 4)
		{
			// mix front -> front, rear -> rear

			for (i = 0; i < count; i++)
			{
				pbuf3[i].left  = pbuf1[i].left  + pbuf2[i].left;
				pbuf3[i].right = pbuf1[i].right + pbuf2[i].right;

				pbufrear3[i].left  = pbufrear1[i].left  + pbufrear2[i].left;
				pbufrear3[i].right = pbufrear1[i].right + pbufrear2[i].right;
			}
			goto gain4ch;
		}

		if ( cchan1 == 2 && cchan2 == 4)
		{
				
			for (i = 0; i < count; i++)
			{
				// split 2 ch left ->  front left, rear left
				// split 2 ch right -> front right, rear right

				xl = pbuf1[i].left;
				xr = pbuf1[i].right;

				pbuf3[i].left  = xl + pbuf2[i].left;
				pbuf3[i].right = xr + pbuf2[i].right;

				pbufrear3[i].left  = xl + pbufrear2[i].left;
				pbufrear3[i].right = xr + pbufrear2[i].right;
			}
			goto gain4ch;
		}

		if ( cchan1 == 2 && cchan2 == 2)
		{
			// mix l,r, split into front l, front r

			for (i = 0; i < count; i++)
			{
				xl = pbuf1[i].left  + pbuf2[i].left;
				xr = pbuf1[i].right + pbuf2[i].right;

				pbufrear3[i].left  = pbuf3[i].left  = xl;
				pbufrear3[i].right = pbuf3[i].right = xr;
			}
			goto gain4ch;
		}
	}

gain2ch:
    if ( gain == 256)		// KDB: perf
		return;

	for (i = 0; i < count; i++)
	{
		pbuf3[i].left  = (pbuf3[i].left * gain) >> 8;
		pbuf3[i].right = (pbuf3[i].right * gain) >> 8;
	}
	return;

gain4ch:
	if ( gain == 256)		// KDB: perf
		return;

	for (i = 0; i < count; i++)
	{
		pbuf3[i].left  = (pbuf3[i].left * gain) >> 8;
		pbuf3[i].right = (pbuf3[i].right * gain) >> 8;
		pbufrear3[i].left  = (pbufrear3[i].left * gain) >> 8;
		pbufrear3[i].right = (pbufrear3[i].right * gain) >> 8;
	}
	return;
}


#if _DEBUG
void MIX_ScalePaintBuffer( int bufferIndex, int count, float fgain )
{
	portable_samplepair_t *pbuf = paintbuffers[bufferIndex].pbuf;
	portable_samplepair_t *pbufrear = paintbuffers[bufferIndex].pbufrear;

	int gain = 256 * fgain;
	int i;

	if ( !paintbuffers[bufferIndex].fsurround )
	{
		for (i = 0; i < count; i++)
		{
			pbuf[i].left  = (pbuf[i].left * gain) >> 8;
			pbuf[i].right = (pbuf[i].right * gain) >> 8;
		}
	}
	else
	{
		for (i = 0; i < count; i++)
		{
			pbuf[i].left  = (pbuf[i].left * gain) >> 8;
			pbuf[i].right = (pbuf[i].right * gain) >> 8;
			pbufrear[i].left  = (pbufrear[i].left * gain) >> 8;
			pbufrear[i].right = (pbufrear[i].right * gain) >> 8;
		}
	}
}
#endif

// DEBUG peak detection values
#define _SDEBUG 1

#ifdef _SDEBUG
float sdebug_avg_in = 0.0;
float sdebug_in_count = 0.0;
float sdebug_avg_out = 0.0;
float sdebug_out_count = 0.0;
#define SDEBUG_TOTAL_COUNT	(3*44100)
#endif // DEBUG

// DEBUG code - get and show peak value of specified paintbuffer
// DEBUG code - ibuf is buffer index, count is # samples to test, pppeakprev stores peak


void SDEBUG_GetAvgValue( int ibuf, int count, float *pav )
{
#ifdef _SDEBUG
	if (snd_showstart.GetInt() != 4 )
		return;

	float av = 0.0;
	
	for (int i = 0; i < count; i++)
		av += (float)(abs(paintbuffers[ibuf].pbuf->left) + abs(paintbuffers[ibuf].pbuf->right))/2.0;
	
	*pav = av / count;
#endif // DEBUG
}


void SDEBUG_GetAvgIn( int ibuf, int count)
{
	float av = 0.0;
	SDEBUG_GetAvgValue( ibuf, count, &av );

	sdebug_avg_in = ((av * count ) + (sdebug_avg_in * sdebug_in_count)) / (count + sdebug_in_count);
	sdebug_in_count += count;
}

void SDEBUG_GetAvgOut( int ibuf, int count)
{
	float av = 0.0;
	SDEBUG_GetAvgValue( ibuf, count, &av );

	sdebug_avg_out = ((av * count ) + (sdebug_avg_out * sdebug_out_count)) / (count + sdebug_out_count);
	sdebug_out_count += count;
}


void SDEBUG_ShowAvgValue()
{
#ifdef _SDEBUG
	

	if (sdebug_in_count > SDEBUG_TOTAL_COUNT)
	{
		if ((int)sdebug_avg_in > 20.0 && (int)sdebug_avg_out > 20.0)
			DevMsg("dsp avg gain:%1.2f in:%1.2f out:%1.2f 1/gain:%1.2f\n", sdebug_avg_out/sdebug_avg_in, sdebug_avg_in, sdebug_avg_out, sdebug_avg_in/sdebug_avg_out);

		sdebug_avg_in = 0.0;
		sdebug_avg_out = 0.0;
		sdebug_in_count = 0.0;
		sdebug_out_count = 0.0;
	}
#endif // DEBUG
}

// clip all values in paintbuffer to 16bit.
// if fsurround is set for paintbuffer, also process rear buffer samples

void MIX_CompressPaintbuffer(int ipaint, int count)
{
	int i;
	paintbuffer_t *ppaint = MIX_GetPPaintFromIPaint(ipaint);
	portable_samplepair_t *pbf;
	portable_samplepair_t *pbr;

	pbf = ppaint->pbuf;
	pbr = ppaint->pbufrear;
	
	for (i = 0; i < count; i++)
	{
		pbf->left = CLIP(pbf->left);
		pbf->right = CLIP(pbf->right);
		pbf++;
	}
	
	if ( ppaint->fsurround )
	{
		Assert (pbr);

		for (i = 0; i < count; i++)
		{
			pbr->left = CLIP(pbr->left);
			pbr->right = CLIP(pbr->right);
			pbr++;
		}
	}
}


// mix and upsample channels to 44khz 'ipaintbuffer'
// mix channels matching 'flags' (SOUND_MIX_DRY or SOUND_MIX_WET) into specified paintbuffer
// upsamples 11khz, 22khz channels to 44khz.

// NOTE: only call this on channels that will be mixed into only 1 paintbuffer
// and that will not be mixed until the next mix pass! otherwise, MIX_MixChannelsToPaintbuffer
// will advance any internal pointers on mixed channels; subsequent calls will be at 
// incorrect offset.

void MIX_MixUpsampleBuffer( int ipaintbuffer, int end, int count, int flags )
{
	int ipaintcur = MIX_GetCurrentPaintbufferIndex(); // save current paintbuffer

	// reset paintbuffer upsampling filter index

	MIX_ResetPaintbufferFilterCounter( ipaintbuffer );

	// prevent other paintbuffers from being mixed

	MIX_DeactivateAllPaintbuffers();
	
	MIX_ActivatePaintbuffer( ipaintbuffer );			// operates on MIX_MixChannelsToPaintbuffer	
	MIX_SetCurrentPaintbuffer( ipaintbuffer );			// operates on MixUpSample

	// mix 11khz channels to buffer

	MIX_MixChannelsToPaintbuffer( end, flags, SOUND_11k, SOUND_11k );

	// upsample 11khz buffer by 2x
	
	g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_11k), FILTERTYPE_LINEAR ); 

	// mix 22khz channels to buffer

	MIX_MixChannelsToPaintbuffer( end, flags, SOUND_22k, SOUND_22k );

	// upsample 22khz buffer by 2x

#if (SOUND_DMA_SPEED > SOUND_22k)

	g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_22k), FILTERTYPE_LINEAR );
	
#endif

	// mix 44khz channels to buffer

	// UNDONE: if (hisound.GetInt() > 0)
	MIX_MixChannelsToPaintbuffer( end, flags, SOUND_44k, SOUND_DMA_SPEED);

	MIX_DeactivateAllPaintbuffers();
	
	// restore previous paintbuffer

	MIX_SetCurrentPaintbuffer( ipaintcur );
}

// upsample and mix sounds into final 44khz versions of:
// IROOMBUFFER, IFACINGBUFFER, IFACINGAWAY, IDRYBUFFER
// dsp fx are then applied to these buffers by the caller.
// caller also remixes all into final IPAINTBUFFER output.

void MIX_UpsampleAllPaintbuffers( int end, int count )
{

	// mix and upsample all 'dry' sounds (channels) to 44khz IDRYBUFFER paintbuffer

	MIX_MixUpsampleBuffer( IDRYBUFFER, end, count, SOUND_MIX_DRY );

	// 11khz sounds are mixed into 3 buffers based on distance from listener, and facing direction
	// These buffers are facing, facingaway, room
	// These 3 mixed buffers are then each upsampled to 22khz.

	// 22khz sounds are mixed into the 3 buffers based on distance from listener, and facing direction
	// These 3 mixed buffers are then each upsampled to 44khz.

	// 44khz sounds are mixed into the 3 buffers based on distance from listener, and facing direction

	MIX_DeactivateAllPaintbuffers();

	// set paintbuffer upsample filter indices to 0

	MIX_ResetPaintbufferFilterCounters();

	if ( !g_bDspOff )		
	{
		// only mix to roombuffer if dsp fx are on KDB: perf

		MIX_ActivatePaintbuffer(IROOMBUFFER);					// operates on MIX_MixChannelsToPaintbuffer
	}

	MIX_ActivatePaintbuffer(IFACINGBUFFER);					

	if ( g_bdirectionalfx )
	{
		// mix to facing away buffer only if directional presets are set

		MIX_ActivatePaintbuffer(IFACINGAWAYBUFFER);		
	}
	
	// mix 11khz sounds: 
	// pan sounds between 3 busses: facing, facingaway and room buffers
	
	MIX_MixChannelsToPaintbuffer( end, SOUND_MIX_WET, SOUND_11k, SOUND_11k);

	// upsample all 11khz buffers by 2x

	if ( !g_bDspOff )
	{
		// only upsample roombuffer if dsp fx are on KDB: perf

		MIX_SetCurrentPaintbuffer(IROOMBUFFER);			// operates on MixUpSample
		g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_11k), FILTERTYPE_LINEAR ); 
	}

	MIX_SetCurrentPaintbuffer(IFACINGBUFFER);			
	g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_11k), FILTERTYPE_LINEAR ); 

	if ( g_bdirectionalfx )
	{
		MIX_SetCurrentPaintbuffer(IFACINGAWAYBUFFER);	
		g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_11k), FILTERTYPE_LINEAR ); 
	}

	// mix 22khz sounds: 
	// pan sounds between 3 busses: facing, facingaway and room buffers
	
	MIX_MixChannelsToPaintbuffer( end, SOUND_MIX_WET, SOUND_22k, SOUND_22k);
	
	// upsample all 22khz buffers by 2x

#if (SOUND_DMA_SPEED > SOUND_22k)
	if ( !g_bDspOff )
	{
		// only upsample roombuffer if dsp fx are on KDB: perf

		MIX_SetCurrentPaintbuffer(IROOMBUFFER);
		g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_22k), FILTERTYPE_LINEAR );
	}

	MIX_SetCurrentPaintbuffer(IFACINGBUFFER);
	g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_22k), FILTERTYPE_LINEAR );

	if ( g_bdirectionalfx )
	{
		MIX_SetCurrentPaintbuffer(IFACINGAWAYBUFFER);
		g_AudioDevice->MixUpsample( count / (SOUND_DMA_SPEED / SOUND_22k), FILTERTYPE_LINEAR );
	}
	
#endif
	// mix all 44khz sounds to all active paintbuffers

	// UNDONE: if (hisound.GetInt() > 0)
	MIX_MixChannelsToPaintbuffer( end, SOUND_MIX_WET, SOUND_44k, SOUND_DMA_SPEED);

	MIX_DeactivateAllPaintbuffers();

	MIX_SetCurrentPaintbuffer(IPAINTBUFFER);

	return;
}


// main mixing rountine - mix up to 'endtime' samples.
// All channels are mixed in a paintbuffer and then sent to 
// hardware.

// A mix pass is performed, resulting in mixed sounds in IROOMBUFFER, IFACINGBUFFER, IFACINGAWAYBUFFER, IDRYBUFFER:
                                  
	// directional sounds are panned and mixed between IFACINGBUFFER and IFACINGAWAYBUFFER
	// omnidirectional sounds are panned 100% into IFACINGBUFFER
	// sound sources far from player (ie: near back of room ) are mixed in proportion to this distance
	// into IROOMBUFFER

// dsp_facingaway fx (2 or 4ch filtering) are then applied to the IFACINGAWAYBUFFER
// dsp_room fx (1ch reverb) are then applied to the IROOMBUFFER

// All 3 buffers are then recombined into the IPAINTBUFFER

// The dsp_water and dsp_player fx are applied in series to the IPAINTBUFFER

// Finally, the IDRYBUFFER buffer is mixed into the IPAINTBUFFER

extern ConVar dsp_off;
extern ConVar snd_profile;
extern void DEBUG_StartSoundMeasure(int type, int samplecount );
extern void DEBUG_StopSoundMeasure(int type, int samplecount );

void MIX_PaintChannels(int endtime)
{
	int 	end;
	int		count;
	float	dsp_room_gain;
	float	dsp_facingaway_gain;
	float	dsp_player_gain;
	float	dsp_water_gain;

	CheckNewDspPresets();

	// dsp performance tuning

	g_snd_profile_type = snd_profile.GetInt();

	// dsp_off is true if no dsp processing is to run
	// directional dsp processing is enabled if dsp_facingaway is non-zero

	g_bDspOff = dsp_off.GetInt() ? 1 : 0;

	if ( ! g_bDspOff )
		g_bdirectionalfx = dsp_facingaway.GetInt() ? 1 : 0;
	else
		g_bdirectionalfx = 0;
	
	// get dsp preset gain values, update gain crossfaders, used when mixing dsp processed buffers into paintbuffer

	dsp_room_gain		= DSP_GetGain ( idsp_room );		// update crossfader - gain only used in MIX_ScaleChannelVolume
	dsp_facingaway_gain	= DSP_GetGain ( idsp_facingaway );	// update crossfader - gain only used in MIX_ScaleChannelVolume
	dsp_player_gain		= DSP_GetGain ( idsp_player );
	dsp_water_gain		= DSP_GetGain ( idsp_water );

	SDEBUG_ShowAvgValue();

	while (paintedtime < endtime)
	{
		// mix a full 'paintbuffer' of sound
		
		// clamp at paintbuffer size

		end = endtime;
		if (endtime - paintedtime > PAINTBUFFER_SIZE)
			end = paintedtime + PAINTBUFFER_SIZE;

		// number of 44khz samples to mix into paintbuffer, up to paintbuffer size

		count = end - paintedtime;

		// clear all mix buffers

		g_AudioDevice->MixBegin( count );
		
		// upsample all mix buffers.
		// results in 44khz versions of:
		// IROOMBUFFER, IFACINGBUFFER, IFACINGAWAYBUFFER, IDRYBUFFER

		MIX_UpsampleAllPaintbuffers( end, count );
	
		// apply 2 or 4ch filtering to IFACINGAWAY buffer
		
		if ( g_bdirectionalfx )
		{
			g_AudioDevice->ApplyDSPEffects( idsp_facingaway, MIX_GetPFrontFromIPaint(IFACINGAWAYBUFFER), MIX_GetPRearFromIPaint(IFACINGAWAYBUFFER), count );
		}

		// apply dsp_room effects to room buffer

		g_AudioDevice->ApplyDSPEffects( idsp_room, MIX_GetPFrontFromIPaint(IROOMBUFFER), MIX_GetPRearFromIPaint(IROOMBUFFER), count );

		if (  g_bdirectionalfx )		// KDB: perf
		{

			// Recombine IFACING and IFACINGAWAY buffers into IPAINTBUFFER

			MIX_MixPaintbuffers( IFACINGBUFFER, IFACINGAWAYBUFFER, IPAINTBUFFER, count, DSP_NOROOM_MIX );
			
			// Add in dsp room fx to paintbuffer, mix at 75%

			MIX_MixPaintbuffers( IROOMBUFFER, IPAINTBUFFER, IPAINTBUFFER, count, DSP_ROOM_MIX );
		} 
		else
		{
			// Mix IFACING buffer with IROOMBUFFER
			// (IFACINGAWAYBUFFER contains no data, IFACINGBBUFFER has full dry mix based on distance from listener)

			// if dsp disabled, mix 100% facingbuffer, otherwise, mix 75% facingbuffer + roombuffer

			float mix = g_bDspOff ? 1.0 : DSP_ROOM_MIX;

			MIX_MixPaintbuffers( IROOMBUFFER, IFACINGBUFFER, IPAINTBUFFER, count, mix );	
		}

		// Apply underwater fx dsp_water (serial in-line)

		if ( g_pClientSidePrediction->GetWaterLevel() > 2 )
			// BUG: if underwater->out of water->underwater, previous delays will be heard. must clear dly buffers.
			g_AudioDevice->ApplyDSPEffects( idsp_water, MIX_GetPFrontFromIPaint(IPAINTBUFFER), MIX_GetPRearFromIPaint(IPAINTBUFFER), count );


// DEBUG: - find dsp gain
		
SDEBUG_GetAvgIn(IPAINTBUFFER, count);

		// Apply player fx dsp_player (serial in-line) - does nothing if dsp fx are disabled

		g_AudioDevice->ApplyDSPEffects( idsp_player, MIX_GetPFrontFromIPaint(IPAINTBUFFER),  MIX_GetPRearFromIPaint(IPAINTBUFFER), count );

// DEBUG: - display dsp gain
SDEBUG_GetAvgOut(IPAINTBUFFER, count);

		// Add dry buffer

		MIX_MixPaintbuffers( IPAINTBUFFER, IDRYBUFFER, IPAINTBUFFER, count, dsp_water_gain * dsp_player_gain );

		// clip all values > 16 bit down to 16 bit

		MIX_CompressPaintbuffer( IPAINTBUFFER, count );

		// transfer IPAINTBUFFER paintbuffer out to DMA buffer

		MIX_SetCurrentPaintbuffer( IPAINTBUFFER );

		g_AudioDevice->TransferSamples( end );
		paintedtime = end;
	}
}

// Applies volume scaling (evenly) to all fl,fr,rl,rr volumes
// used for voice ducking and panning between various mix busses

// Called just before mixing wav data to current paintbuffer.
// a) if another player in a multiplayer game is speaking, scale all volumes down.
// b) if mixing to IROOMBUFFER, scale all volumes by ch.dspmix and dsp_room gain
// c) if mixing to IFACINGAWAYBUFFER, scale all volumes by ch.dspface and dsp_facingaway gain
// d) If SURROUND_ON, but buffer is not surround, recombined front/rear volumes

void MIX_ScaleChannelVolume( paintbuffer_t *ppaint, channel_t *pChannel, int volume[CCHANVOLUMES], int mixchans )
{
	int i;
	int *pvol;
	int	mixflag = ppaint->flags;
	float scale;
	char wavtype = pChannel->wavtype;

	pvol = &pChannel->leftvol;

	// copy channel volumes into output array

	for (i = 0; i < CCHANVOLUMES; i++)
		volume[i] = pvol[i];

	// duck all sound volumes except speaker's voice

	int duckScale = min((int)(g_DuckScale * 256), g_SND_VoiceOverdriveInt);
	if( duckScale < 256 )
	{
		if( pChannel->pMixer )
		{
			CAudioSource *pSource = pChannel->pMixer->GetSource();
			if( !pSource->IsVoiceSource() )
			{
				// Apply voice overdrive..
				for (i = 0; i < CCHANVOLUMES; i++)
					volume[i] = (volume[i] * duckScale) >> 8;
			}
		}
	}

	// If mixing to the room buss, adjust volume based on channel's dspmix setting.
	// dspmix is DSP_MIX_MAX (~0.78) if sound is far from player, DSP_MIX_MIN (~0.24) if sound is near player
	
	if ( mixflag & SOUND_BUSS_ROOM )
	{

		// get current idsp_room gain

		float dsp_gain = DSP_GetGain( idsp_room );

		// if dspmix is 1.0, 100% of sound goes to both IROOMBUFFER and IFACINGBUFFER

		for (i = 0; i < CCHANVOLUMES; i++)
			volume[i] = (int)((float)(volume[i]) * pChannel->dspmix * dsp_gain );
	}

	// If mixing to facing/facingaway buss, adjust volume based on sound entity's facing direction.

	// If sound directly faces player, ch->dspface = 1.0.  If facing directly away, ch->dspface = -1.0.
	// mix to lowpass buffer if facing away, to allpass if facing
	
	// scale 1.0 - facing player, scale 0, facing away

	scale = (pChannel->dspface + 1.0) / 2.0;

	// UNDONE: get front cone % from channel to set this.

	// bias scale such that 1.0 to 'cone' is considered facing.  Facing cone narrows as cone -> 1.0
	// and 'cone' -> 0.0 becomes 1.0 -> 0.0

	float cone = 0.6f;

	scale = scale * (1/cone);

	scale = clamp( scale, 0.0f, 1.0f );

	// pan between facing and facing away buffers

	// if ( !g_bdirectionalfx || wavtype == CHAR_DOPPLER || wavtype == CHAR_OMNI || (wavtype == CHAR_DIRECTIONAL && mixchans == 2) )
	if ( !g_bdirectionalfx || wavtype != CHAR_DIRECTIONAL )
	{
		// if no directional fx mix 0% to facingaway buffer
		// if wavtype is DOPPLER, mix 0% to facingaway buffer - DOPPLER wavs have a custom mixer
		// if wavtype is OMNI, mix 0% to faceingaway buffer - OMNI wavs have no directionality
		// if wavtype is DIRECTIONAL and stereo encoded, mix 0% to facingaway buffer - DIRECTIONAL STEREO wavs have a custom mixer
		
		scale = 1.0;
	}

	if ( mixflag & SOUND_BUSS_FACING )
	{
		// facing player
		// if dspface is 1.0, 100% of sound goes to IFACINGBUFFER

		for (i = 0; i < CCHANVOLUMES; i++)
			volume[i] = (int)((float)(volume[i]) * scale * (1.0 - pChannel->dspmix));
	}
	else if ( mixflag & SOUND_BUSS_FACINGAWAY )
	{
		// facing away from player
		// if dspface is 0.0, 100% of sound goes to IFACINGAWAYBUFFER

		// get current idsp_facingaway gain

		float dsp_gain = DSP_GetGain( idsp_facingaway );

		for (i = 0; i < CCHANVOLUMES; i++)
			volume[i] = (int)((float)(volume[i]) * (1.0 - scale) * dsp_gain * (1.0 - pChannel->dspmix));
	}

	// NOTE: this must occur last in this routine: 

	if ( SURROUND_ON && !ppaint->fsurround )
	{
		// if 4ch spatialization on, but current mix buffer is 2ch, 
		// recombine front + rear volumes (revert to 2ch spatialization)

		volume[IFRONT_RIGHT] += volume[IREAR_RIGHT];
		volume[IFRONT_LEFT] += volume[IREAR_LEFT];

		volume[IFRONT_RIGHTD] += volume[IREAR_RIGHTD];
		volume[IFRONT_LEFTD] += volume[IREAR_LEFTD];
	}

	for (i = 0; i < CCHANVOLUMES; i++)
		volume[i] = clamp(volume[i], 0, 255);
}


//===============================================================================
// Low level mixing routines
//===============================================================================

#if	!id386
void Snd_WriteLinearBlastStereo16 (void)
{
	int		i;
	int		val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = (snd_p[i]*snd_vol)>>8;
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i] = (short)0x8000;
		else
			snd_out[i] = val;

		val = (snd_p[i+1]*snd_vol)>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < (short)0x8000)
			snd_out[i+1] = (short)0x8000;
		else
			snd_out[i+1] = val;
	}
}
#endif


void SND_InitScaletable (void)
{
	int		i, j;
	
	for (i=0 ; i<SND_SCALE_LEVELS; i++)
		for (j=0 ; j<256 ; j++)
			snd_scaletable[i][j] = ((signed char)j) * i * (1<<SND_SCALE_SHIFT);
}

#if	!id386
void SND_PaintChannelFrom8( int *volume, byte *pData8, int count )
{
	int 	data;
	int		*lscale, *rscale;
	int		i;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];

	for (i=0 ; i<count ; i++)
	{
		data = pData8[i];

		paintbuffer[i].left += lscale[data];
		paintbuffer[i].right += rscale[data];
	}
}

#endif	// !id386


//===============================================================================
// SOFTWARE MIXING ROUTINES
//===============================================================================

// UNDONE: optimize these

// grab samples from left source channel only and mix as if mono. 
// volume array contains appropriate spatialization volumes for doppler left (incoming sound)

void SW_Mix8StereoDopplerLeft( portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int		*lscale, *rscale;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += lscale[pData[sampleIndex]];
		pOutput[i].right += rscale[pData[sampleIndex]];
		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

// grab samples from right source channel only and mix as if mono.
// volume array contains appropriate spatialization volumes for doppler right (outgoing sound)

void SW_Mix8StereoDopplerRight( portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int		*lscale, *rscale;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += lscale[pData[sampleIndex+1]];
		pOutput[i].right += rscale[pData[sampleIndex+1]];
		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}

}

// grab samples from left source channel only and mix as if mono. 
// volume array contains appropriate spatialization volumes for doppler left (incoming sound)

void SW_Mix16StereoDopplerLeft( portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += (volume[0] * (int)(pData[sampleIndex]))>>8;
		pOutput[i].right += (volume[1] * (int)(pData[sampleIndex]))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}


// grab samples from right source channel only and mix as if mono.
// volume array contains appropriate spatialization volumes for doppler right (outgoing sound)

void SW_Mix16StereoDopplerRight( portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	
	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += (volume[0] * (int)(pData[sampleIndex+1]))>>8;
		pOutput[i].right += (volume[1] * (int)(pData[sampleIndex+1]))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

// mix left wav (front facing) with right wav (rear facing) based on soundfacing direction

void SW_Mix8StereoDirectional( float soundfacing, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int	x;
	int l,r;
	signed char lb,rb;
	
	// if soundfacing -1.0, sound source is facing away from player
	// if soundfacing 0.0, sound source is perpendicular to player
	// if soundfacing 1.0, sound source is facing player

	int	frontmix = (int)(256.0f * ((1.f + soundfacing) / 2.f));	// 0 -> 256
	int rearmix  = (int)(256.0f * ((1.f - soundfacing) / 2.f));	// 256 -> 0

	
	for ( int i = 0; i < outCount; i++ )
	{
		lb = (pData[sampleIndex]);		// get left byte
		rb = (pData[sampleIndex+1]);	// get right byte

		l = ((int)lb) << 8;			// convert to 16 bit. UNDONE: better dithering
		r = ((int)rb) << 8;

		x = ((l * frontmix) >> 8) + ((r * rearmix) >> 8);

		pOutput[i].left += (volume[0] * (int)(x))>>8;
		pOutput[i].right += (volume[1] * (int)(x))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

// mix left wav (front facing) with right wav (rear facing) based on soundfacing direction

void SW_Mix16StereoDirectional( float soundfacing, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int	x;
	int l,r;
	
	// if soundfacing -1.0, sound source is facing away from player
	// if soundfacing 0.0, sound source is perpendicular to player
	// if soundfacing 1.0, sound source is facing player

	int	frontmix = (int)(256.0f * ((1.f + soundfacing) / 2.f));	// 0 -> 256
	int rearmix  = (int)(256.0f * ((1.f - soundfacing) / 2.f));	// 256 -> 0

	for ( int i = 0; i < outCount; i++ )
	{
		l = (int)(pData[sampleIndex]);
		r = (int)(pData[sampleIndex+1]);

		x = ((l * frontmix) >> 8) + ((r * rearmix) >> 8);

		pOutput[i].left += (volume[0] * (int)(x))>>8;
		pOutput[i].right += (volume[1] * (int)(x))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

void SW_Mix8StereoDistVar( float distmix, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int	x;
	int l,r;
	signed char lb, rb;

	// distmix 0 - sound is near player (100% wav left)
	// distmix 1.0 - sound is far from player (100% wav right)

	int nearmix  = (int)(256.0f * (1.f - distmix));
	int	farmix = (int)(256.0f * distmix);
	
	// if mixing at max or min range, skip crossfade (KDB: perf)

	if (!nearmix)
	{
		for ( int i = 0; i < outCount; i++ )
		{
			rb = (pData[sampleIndex+1]);	// get right byte

			x = ((int)rb) << 8;				// convert to 16 bit. UNDONE: better dithering
			
			pOutput[i].left += (volume[0] * (int)(x))>>8;
			pOutput[i].right += (volume[1] * (int)(x))>>8;

			sampleFrac += rateScaleFix;
			sampleIndex += FIX_INTPART(sampleFrac)<<1;
			sampleFrac = FIX_FRACPART(sampleFrac);
		}
		return;
	}

	if (!farmix)
	{
		for ( int i = 0; i < outCount; i++ )
		{

			lb = (pData[sampleIndex]);		// get left byte

			x = ((int)lb) << 8;				// convert to 16 bit. UNDONE: better dithering
			
			pOutput[i].left += (volume[0] * (int)(x))>>8;
			pOutput[i].right += (volume[1] * (int)(x))>>8;

			sampleFrac += rateScaleFix;
			sampleIndex += FIX_INTPART(sampleFrac)<<1;
			sampleFrac = FIX_FRACPART(sampleFrac);
		}
		return;
	}

	// crossfade left/right

	for ( int i = 0; i < outCount; i++ )
	{

		lb = (pData[sampleIndex]);		// get left byte
		rb = (pData[sampleIndex+1]);	// get right byte

		l = ((int)lb) << 8;			// convert to 16 bit. UNDONE: better dithering
		r = ((int)rb) << 8;
		
		x = ((l * nearmix) >> 8) + ((r * farmix) >> 8);

		pOutput[i].left += (volume[0] * (int)(x))>>8;
		pOutput[i].right += (volume[1] * (int)(x))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

void SW_Mix16StereoDistVar( float distmix, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount ) 
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset; 
	int	x;
	int l,r;
	
	// distmix 0 - sound is near player (100% wav left)
	// distmix 1.0 - sound is far from player (100% wav right)

	int nearmix  = Float2Int(256.0f * (1.f - distmix));
	int	farmix =  Float2Int(256.0f * distmix);

	// if mixing at max or min range, skip crossfade (KDB: perf)

	if (!nearmix)
	{
		for ( int i = 0; i < outCount; i++ )
		{
			x = pData[sampleIndex+1];	// right sample

			pOutput[i].left += (volume[0] * x)>>8;
			pOutput[i].right += (volume[1] * x)>>8;

			sampleFrac += rateScaleFix;
			sampleIndex += FIX_INTPART(sampleFrac)<<1;
			sampleFrac = FIX_FRACPART(sampleFrac);
		}
		return;
	}

	if (!farmix)
	{
		for ( int i = 0; i < outCount; i++ )
		{
			x = pData[sampleIndex];		// left sample
		
			pOutput[i].left += (volume[0] * x)>>8;
			pOutput[i].right += (volume[1] * x)>>8;

			sampleFrac += rateScaleFix;
			sampleIndex += FIX_INTPART(sampleFrac)<<1;
			sampleFrac = FIX_FRACPART(sampleFrac);
		}
		return;
	}

	// crossfade left/right

	for ( int i = 0; i < outCount; i++ )
	{
		l = pData[sampleIndex];
		r = pData[sampleIndex+1];

		x = ((l * nearmix) >> 8) + ((r * farmix) >> 8);

		pOutput[i].left += (volume[0] * x)>>8;
		pOutput[i].right += (volume[1] * x)>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}


void SW_Mix8Mono( portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	// Not using pitch shift?
	if ( rateScaleFix == FIX(1) )
	{
		// paintbuffer native code
		if ( pOutput == paintbuffer )
		{
			SND_PaintChannelFrom8( volume, (byte *)pData, outCount );
			return;
		}

	}

	// UNDONE: Native code this and use adc to integrate the int/frac parts separately
	// UNDONE: Optimize when not pitch shifting?
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int		*lscale, *rscale;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += lscale[pData[sampleIndex]];
		pOutput[i].right += rscale[pData[sampleIndex]];
		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac);
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

void SW_Mix8Stereo( portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;
	int		*lscale, *rscale;

	lscale = snd_scaletable[volume[0] >> SND_SCALE_SHIFT];
	rscale = snd_scaletable[volume[1] >> SND_SCALE_SHIFT];

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += lscale[pData[sampleIndex]];
		pOutput[i].right += rscale[pData[sampleIndex+1]];
		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

#if 0

void SW_Mix16Mono( portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += (volume[0] * (int)(pData[sampleIndex]))>>8;
		pOutput[i].right += (volume[1] * (int)(pData[sampleIndex]))>>8;
		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac);
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}

#else  

void SW_Mix16Mono( portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	int vol0 = volume[0];
	int vol1 = volume[1];

	__asm
	{
		mov eax, volume					;
		movq mm0, DWORD PTR [eax]		; vol1, vol0 (32-bits each)
		packssdw mm0, mm0				; pack and replicate... vol1, vol0, vol1, vol0 (16-bits each)
		//pxor mm7, mm7					; mm7 is my zero register...

		xor esi, esi
		mov	eax, DWORD PTR [pOutput]	; store initial output ptr
		mov edx, DWORD PTR [pData]		; store initial input ptr
		mov ebx, inputOffset;
		mov ecx, outCount;
		
BEGINLOAD:
		movd mm2, WORD PTR [edx+2*esi]	; load first piece o' data from pData
		punpcklwd mm2, mm2				; 0, 0, pData_1st, pData_1st

		add ebx, rateScaleFix			; do the crazy fixed integer math
		mov edi, ebx					; store a copy of sampleFrac
		sar ebx, 01Ch					; compute the increment-by value for sampleIndex
		add esi, ebx					; increment sampleIndex
		and edi, FIX_MASK				; adjust sampleFrac
		mov ebx, edi					;

		movd mm3, WORD PTR [edx+2*esi]	; load second piece o' data from pData
		punpcklwd mm3, mm3				; 0, 0, pData_2nd, pData_2nd
		punpckldq mm2, mm3				; pData_2nd, pData_2nd, pData_2nd, pData_2nd

		add ebx, rateScaleFix			; do the crazy fixed integer math
		mov edi, ebx					; store a copy of sampleFrac
		sar ebx, 01Ch					; compute the increment-by value for sampleIndex
		add esi, ebx					; increment sampleIndex
		and edi, FIX_MASK				; adjust sampleFrac
		mov ebx, edi					;
	
        movq mm3, mm2					; copy the goods
		pmullw mm2, mm0					; pData_2nd*vol1, pData_2nd*vol0, pData_1st*vol1, pData_1st*vol0 (bits 0-15)
		pmulhw mm3, mm0					; pData_2nd*vol1, pData_2nd*vol0, pData_1st*vol1, pData_1st*vol0 (bits 16-31)

		movq mm4, mm2					; copy
		movq mm5, mm3					; copy

		punpcklwd mm2, mm3				; pData_1st*vol1, pData_1st*vol0 (bits 0-31)
		punpckhwd mm4, mm5				; pData_2nd*vol1, pData_2nd*vol0 (bits 0-31)
		psrad mm2, 8					; shift right by 8
		psrad mm4, 8					; shift right by 8

		add ecx, -2                     ; decrement i-value
		paddd mm2, QWORD PTR [eax]		; add to existing vals
		paddd mm4, QWORD PTR [eax+8]	;

		movq QWORD PTR [eax], mm2		; store back
		movq QWORD PTR [eax+8], mm4		;

		add eax, 10h					;
		cmp ecx, 01h                    ; see if we can quit
		jg BEGINLOAD                    ; Kipp Owens is a doof...
		jl END							; Nick Shaffner is killing me...

		movsx edi, WORD PTR [edx+2*esi] ; load first 16 bit val and zero-extend
		imul  edi, vol0					; multiply pData[sampleIndex] by volume[0]
		sar   edi, 08h                  ; divide by 256
		add DWORD PTR [eax], edi        ; add to pOutput[i].left
		
		movsx edi, WORD PTR [edx+2*esi] ; load same 16 bit val and zero-extend ('cuz I thrashed the reg)
		imul  edi, vol1					; multiply pData[sampleIndex] by volume[1]
		sar   edi, 08h                  ; divide by 256
		add DWORD PTR [eax+04h], edi    ; add to pOutput[i].right
END:
		emms;
	}
}
#endif

void SW_Mix16Stereo( portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	int sampleIndex = 0;
	fixedint sampleFrac = inputOffset;

	for ( int i = 0; i < outCount; i++ )
	{
		pOutput[i].left += (volume[0] * (int)(pData[sampleIndex]))>>8;
		pOutput[i].right += (volume[1] * (int)(pData[sampleIndex+1]))>>8;

		sampleFrac += rateScaleFix;
		sampleIndex += FIX_INTPART(sampleFrac)<<1;
		sampleFrac = FIX_FRACPART(sampleFrac);
	}
}


//===============================================================================
// DISPATCHERS FOR MIXING ROUTINES
//===============================================================================

void Mix8MonoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	SW_Mix8Mono( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
}

void Mix16MonoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	SW_Mix16Mono( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
}

void Mix8StereoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, byte *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	switch ( pChannel->wavtype )
	{
	case CHAR_DOPPLER:
		SW_Mix8StereoDopplerLeft( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		SW_Mix8StereoDopplerRight( pOutput, &volume[IFRONT_LEFTD], pData, inputOffset, rateScaleFix, outCount );
		break;

	case CHAR_DIRECTIONAL:
		SW_Mix8StereoDirectional( pChannel->dspface, pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		break;

	case CHAR_DISTVARIANT:
		SW_Mix8StereoDistVar( pChannel->distmix, pOutput, volume, pData, inputOffset, rateScaleFix, outCount);
		break;

	default:
		SW_Mix8Stereo( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		break;
	}
}

void Mix16StereoWavtype( channel_t *pChannel, portable_samplepair_t *pOutput, int *volume, short *pData, int inputOffset, fixedint rateScaleFix, int outCount )
{
	switch ( pChannel->wavtype )
	{
	case CHAR_DOPPLER:
		SW_Mix16StereoDopplerLeft( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		SW_Mix16StereoDopplerRight( pOutput, &volume[IFRONT_LEFTD], pData, inputOffset, rateScaleFix, outCount );
		break;

	case CHAR_DIRECTIONAL:
		SW_Mix16StereoDirectional( pChannel->dspface, pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		break;

	case CHAR_DISTVARIANT:
		SW_Mix16StereoDistVar( pChannel->distmix, pOutput, volume, pData, inputOffset, rateScaleFix, outCount);
		break;

	default:
		SW_Mix16Stereo( pOutput, volume, pData, inputOffset, rateScaleFix, outCount );
		break;
	}
}


//===============================================================================
// Client entity mouth movement code.  Set entity mouthopen variable, based
// on the sound envelope of the voice channel playing.
// KellyB 10/22/97
//===============================================================================


// called when voice channel is first opened on this entity

void SND_InitMouth(int entnum, int entchannel)
{
	if ( !entitylist )
		return;

	if ((entchannel == CHAN_VOICE || entchannel == CHAN_STREAM) && entnum > 0)
	{
		IClientEntity *clientEntity;

		// init mouth movement vars
		clientEntity = entitylist->GetClientEntity( entnum );
		if ( clientEntity )
		{
			CMouthInfo *mouth = clientEntity->GetMouth();
			if ( mouth )
			{
				mouth->mouthopen = 0;
				mouth->sndavg = 0;
				mouth->sndcount = 0;
			}
		}
	}
}

// called when channel stops

void SND_CloseMouth(channel_t *ch) 
{
	if ( !entitylist )
		return;

	IClientEntity *clientEntity = entitylist->GetClientEntity( ch->soundsource );
	if( clientEntity )
	{
		CMouthInfo *m = clientEntity->GetMouth();
		if ( m && 
			( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_STREAM ) )
		{
			// shut mouth
			m->ClearVoiceSources();
			m->mouthopen = 0;
		}
	}
}

static CMouthInfo *GetMouthInfoForChannel( channel_t *pChannel )
{
	IClientEntity *pClientEntity = entitylist->GetClientEntity( pChannel->soundsource );
	
	if( !pClientEntity )
		return NULL;

	return pClientEntity->GetMouth();
}

#define CAVGSAMPLES 10

void SND_MoveMouth8( channel_t *ch, CAudioSource *pSource, int count ) 
{
	int 	data;
	char	*pdata = NULL;
	int		i;
	int		savg;
	int		scount;

	CMouthInfo *pMouth = GetMouthInfoForChannel( ch );
	
	if ( !pMouth )
		return;

	int idx = pMouth->GetIndexForSource( pSource );
	
	if ( idx == UNKNOWN_VOICE_SOURCE )
	     pMouth->AddSource( pSource, g_pSoundServices->GetClientTime());

	pSource->GetOutputData((void**)&pdata, ch->pMixer->GetSamplePosition(), count, NULL );

	if( pdata == NULL )
		return;
	
	i = 0;
	scount = pMouth->sndcount;
	savg = 0;

	while ( i < count && scount < CAVGSAMPLES )
	{
		data = pdata[i];
		savg += abs(data);	

		i += 80 + ((byte)data & 0x1F);
		scount++;
	}

	pMouth->sndavg += savg;
	pMouth->sndcount = (byte) scount;

	if ( pMouth->sndcount >= CAVGSAMPLES ) 
	{
		pMouth->mouthopen = pMouth->sndavg / CAVGSAMPLES;
		pMouth->sndavg = 0;
		pMouth->sndcount = 0;
	}
}


void SND_UpdateMouth( channel_t *pChannel )
{
	CMouthInfo *m = GetMouthInfoForChannel( pChannel );
	if ( !m )
		return;

	if ( pChannel->sfx )
		m->AddSource( pChannel->sfx->pSource, g_pSoundServices->GetClientTime() );
}


void SND_ClearMouth( channel_t *pChannel )
{
	CMouthInfo *m = GetMouthInfoForChannel( pChannel );
	if ( !m )
		return;

	if ( pChannel->sfx )
		m->RemoveSource( pChannel->sfx->pSource );
}


bool SND_IsMouth( channel_t *pChannel )
{
	if ( entitylist->GetClientEntity( pChannel->soundsource ) )
	{
		if ( pChannel->entchannel == CHAN_VOICE || pChannel->entchannel == CHAN_STREAM )
			return true;
	}
	
	return false;
}

//===============================================================================
// Movie recording support
//===============================================================================

void SND_MovieStart( void )
{
	if ( !cl_moviename[0] )
		return;

	paintedtime = 0;
	soundtime = 0;
	// 44k: engine playback rate is now 44100...changed from 22050
	WaveCreateTmpFile( cl_moviename, SOUND_DMA_SPEED, 16, 2 );
}

void SND_MovieEnd( void )
{
	if ( !cl_moviename[0] )
		return;

	WaveFixupTmpFile( cl_moviename );
}


void SND_RecordBuffer( void )
{
	if ( !cl_moviename[0] || Con_IsVisible() )
		return;

	int		i;
	int		val;
	int		bufferSize = snd_linear_count * sizeof(short);
	short	*tmp = (short *)_alloca( bufferSize );
	
	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = (snd_p[i]*snd_vol)>>8;
		tmp[i] = CLIP(val);
		
		val = (snd_p[i+1]*snd_vol)>>8;
		tmp[i+1] = CLIP(val);
	}

	WaveAppendTmpFile( cl_moviename, tmp, bufferSize );
}
