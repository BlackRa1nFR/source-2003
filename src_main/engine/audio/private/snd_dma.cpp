//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Main control for any streaming sound output device.
//
//=============================================================================

#include "sound.h"
#include "tier0/dbg.h"
#include "soundflags.h"
#include "sound.h"
#include "sound_private.h"
#include "const.h"
#include "cdll_int.h"
#include "snd_audio_source.h"
#include "client_class.h"
#include "icliententitylist.h"
#include "tier0/vcrmode.h"
#include "con_nprint.h"
#include "snd_mix_buf.h"
#include "snd_channels.h"
#include "snd_device.h"
#include "snd_sfx.h"
#include "snd_convars.h"

#include "vox_private.h"
#include "../../traceinit.h"
#include "../../cmd.h"

#include "SoundService.h"
#include "vaudio/ivaudio.h"
#include "../../client.h"
#include "UtlDict.h"

#include "filesystem.h"
#include "../../FileSystem_Engine.h"
#include "../../enginetrace.h"			// for traceline
#include "../../public/bspflags.h"		// for traceline
#include "../../public/gametrace.h"		// for traceline

///////////////////////////////////
// DEBUGGING
//
// Turn this on to print channel output msgs.
//
//#define DEBUG_CHANNELS

#define SNDLVL_TO_DIST_MULT( sndlvl ) ( sndlvl ? ((pow( 10, snd_refdb.GetFloat() / 20 ) / pow( 10, (float)sndlvl / 20 )) / snd_refdist.GetFloat()) : 0 )
#define DIST_MULT_TO_SNDLVL( dist_mult ) (soundlevel_t)(int)( dist_mult ? ( 20 * log10( pow( 10, snd_refdb.GetFloat() / 20 ) / (dist_mult * snd_refdist.GetFloat()) ) ) : 0 )

extern char cl_moviename[];

void S_Play(void);
void S_PlayVol(void);
void S_SoundList(void);
void S_Say (void);
void S_Update_();
void S_StopAllSounds(qboolean clear);
void S_StopAllSoundsC(void);

float SND_GetGainObscured( channel_t *ch, bool fplayersound, bool flooping );
float dB_To_Radius ( float db );

// =======================================================================
// Internal sound data & structures
// =======================================================================

channel_t   channels[MAX_CHANNELS];

int			total_channels;

static qboolean	snd_ambient = 1;
bool		snd_initialized = false;

Vector		listener_origin;
Vector		listener_forward;
Vector		listener_right;
Vector		listener_up;
vec_t		sound_nominal_clip_dist=SOUND_NORMAL_CLIP_DIST;

// @TODO (toml 05-08-02): put this somewhere more reasonable
vec_t S_GetNominalClipDist()
{
	return sound_nominal_clip_dist;
}

int			soundtime;		// sample PAIRS
int   		paintedtime; 	// sample PAIRS

static CClassMemoryPool< CSfxTable > s_SoundPool( MAX_SFX );
struct SfxDictEntry
{
	CSfxTable *psfx;
};

static CUtlDict< SfxDictEntry, int > s_Sounds;

static CSfxTable dummySfx;

//-----------------------------------------------------------------------------
// Purpose: Wrapper for sfxtable->getname()
// Output : char const
//-----------------------------------------------------------------------------
char const *CSfxTable::getname()
{
	return name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//-----------------------------------------------------------------------------
void CSfxTable::setname( char const *pszName )
{
	strcpy( name, pszName );
}

float		g_DuckScale = 1.0f;

// Structure used for fading in and out client sound volume.
typedef struct
{
	float		initial_percent;

	// How far to adjust client's volume down by.
	float		percent;  

	// GetHostTime() when we started adjusting volume
	float		starttime;   

	// # of seconds to get to faded out state
	float		fadeouttime; 
    // # of seconds to hold
	float		holdtime;  
	// # of seconds to restore
	float		fadeintime;
} soundfade_t;

static soundfade_t soundfade;  // Client sound fading singleton object

static ConVar volume( "volume", "0.7", FCVAR_ARCHIVE, "Sound volume", true, 0.0f, true, 1.0f );
ConVar hisound( "hisound", "1", FCVAR_ARCHIVE );

ConVar s_surround( "snd_surround", "0", FCVAR_ARCHIVE );
	
ConVar snd_noextraupdate( "snd_noextraupdate", "0" );
ConVar snd_show( "snd_show", "0");
				
/*
#if defined( _DEBUG )
	0
#else
	FCVAR_SPONLY 
#endif
	);
*/
ConVar snd_mixahead( "snd_mixahead", "0.1", FCVAR_ARCHIVE );

// vaudio DLL
IVAudio *vaudio = NULL;
CSysModule *g_pVAudioModule = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float S_GetMasterVolume( void )
{
	float scale = 1.0f;
	if ( soundfade.percent != 0 )
	{
		scale = clamp( (float)soundfade.percent / 100.0f, 0.0f, 1.0f );
		scale = 1.0f - scale;
	}
	return volume.GetFloat() * scale;
}

// ====================================================================
// User-setable variables
// ====================================================================

#if DEAD
void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}
#endif


void S_SoundInfo_f(void)
{
	if (!g_AudioDevice->IsActive())
	{
		Msg ("sound system not started\n");
		return;
	}
	Msg("Sound Device: %s\n", g_AudioDevice->DeviceName() );
    Msg("%5d stereo\n", g_AudioDevice->DeviceChannels() - 1);
    Msg("%5d samples\n", g_AudioDevice->DeviceSampleCount());
    Msg("%5d samplebits\n", g_AudioDevice->DeviceSampleBits());
    Msg("%5d speed\n", g_AudioDevice->DeviceDmaSpeed());
	Msg("%5d total_channels\n", total_channels);
}


/*
================
S_Startup
================
*/

void S_Startup (void)
{
	if (!snd_initialized)
		return;

	if ( !g_AudioDevice )
	{
		g_AudioDevice = IAudioDevice::AutoDetectInit( g_pSoundServices->CheckParm("-wavonly") != 0 );
		if ( !g_AudioDevice )
		{
			Error( "Unable to init audio" );
		}
	}
}

static ConCommand play("play", S_Play);
static ConCommand playflush("playflush", S_Play);
static ConCommand playvol("playvol", S_PlayVol);
static ConCommand speak("speak", S_Say);
static ConCommand stopsound("stopsound", S_StopAllSoundsC, 0, FCVAR_CHEAT);		// Marked cheat because it gives an advantage to players minimising ambient noise.
static ConCommand soundlist("soundlist", S_SoundList);
static ConCommand soundinfo("soundinfo", S_SoundInfo_f);

/*
================
S_Init
================
*/
void S_Init (void)
{
	bool fsurround;

	DevMsg("Sound Initialization\n");

	// KDB: init sentence array

	TRACEINIT( VOX_Init(), VOX_Shutdown() );	

	g_pFileSystem->GetLocalCopy( "mss32.dll" ); // vaudio_miles.dll will load this...
	g_pVAudioModule = FileSystem_LoadModule( "vaudio_miles.dll" );
	CreateInterfaceFn vaudioFactory = Sys_GetFactory( g_pVAudioModule );
	vaudio = (IVAudio *)vaudioFactory( VAUDIO_INTERFACE_VERSION, NULL );
	
	if ( g_pSoundServices->CheckParm("-nosound") )
	{
		g_AudioDevice = Audio_GetNullDevice();
		return;
	}

	fsurround = g_pSoundServices->QuerySetting( "snd_surround", false );

	s_surround.SetValue( fsurround ? 1 : 0 );

	snd_initialized = true;

	S_Startup();

	MIX_InitAllPaintbuffers();

	SND_InitScaletable ();

// create a piece of DMA memory
	// UNDONE: JAY : Rebuild the fake dma as an audio device
#if 0
	if (fakedma)
	{
		shm = (volatile dma_t *) g_pSoundServices->LevelAlloc(sizeof(*shm), "shm");
		shm->samplebits = 16;
		shm->speed = SOUND_DMA_SPEED;
		shm->channels = 2;
		shm->samples = 32768;
		shm->buffer = (unsigned char *volatile)g_pSoundServices->LevelAlloc(1<<16, "shmbuf");
	}
#endif

	DevMsg( "Sound sampling rate: %i\n", g_AudioDevice->DeviceDmaSpeed() );

	S_StopAllSounds (true);

	// TRACEINIT( SX_Init(), SX_Free() );

	AllocDsps();
}


// =======================================================================
// Shutdown sound engine
// =======================================================================
void S_Shutdown(void)
{
	S_StopAllSounds( true );

	SNDDMA_Shutdown();

	int c = s_Sounds.Count();
	for ( int i = 0; i < c; i++ )
	{
		if ( s_Sounds[i].psfx )
		{
			delete s_Sounds[i].psfx->pSource;
			s_Sounds[i].psfx->pSource = NULL;
		}
	}
	s_Sounds.Purge();
	s_SoundPool.Clear();

	// release DSP resources
	// TRACESHUTDOWN( SX_Free() );
	FreeDsps();

	// release sentences resources
	TRACESHUTDOWN( VOX_Shutdown() );
	
	// shutdown vaudio
	FileSystem_UnloadModule( g_pVAudioModule );
	g_pVAudioModule = NULL;
	vaudio = NULL;

	MIX_FreeAllPaintbuffers();
}

// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
// Return sfx and set pfInCache to 1 if 
// name is in name cache. Otherwise, alloc
// a new spot in name cache and return 0 
// in pfInCache.
CSfxTable *S_FindName( const char *name, int *pfInCache )
{
	int		i;
	CSfxTable	*sfx = NULL;

	if (!name)
		Error ("S_FindName: NULL\n");

// see if already loaded
	i = s_Sounds.Find( name );
	if ( i != s_Sounds.InvalidIndex() )
	{
		sfx = s_Sounds[ i ].psfx;
		Assert( sfx );
		if ( pfInCache )
		{
			// indicate whether or not sound is currently in the cache.
			*pfInCache = ( sfx->pSource && sfx->pSource->IsCached() ) ? 1 : 0;
		}

		return sfx;
	}
	else
	{
		SfxDictEntry entry;
		entry.psfx = ( CSfxTable * )s_SoundPool.Alloc();

		Assert( entry.psfx );

		i = s_Sounds.Insert( name, entry );
		sfx = s_Sounds[ i ].psfx;
		sfx->setname( name );
		sfx->pSource = NULL;

		if ( pfInCache )
		{
			*pfInCache = 0;
		}
	}
	return sfx;
}

#if DEAD
// Used when switching from/to hires sound mode.
// discard sound's data from cache so that
// data will be reloaded (and resampled)
// 'rate' is sound_11k,22k or 44k

void S_FlushSoundData(int rate)
{
	int i, j;
	CAudioSource *pSource;
	channel_t *ch = channels;
	int fNoDiscard = FALSE;

	// scan precache, looking for sounds in memory
	// if a sound is in memory, make sure it's not
	// a block from a streaming sound
	// if not a streaming sound, then discard
	// according to speed
	
	for (i = 0; i < num_sfx; i++)
	{
		pSource = known_sfx[i].pSource;
		if ( !pSource || !pSource->IsCached() )
			continue;

		// make sure this data is not part of streaming sound
		for (j=0; j<total_channels ; j++, ch++)
		{
			// skip innactive channels
			if (!ch->sfx)
				continue;

			if (ch->entchannel != CHAN_STREAM)
				continue;

			if (ch->sfx == &known_sfx[i])
			{
				fNoDiscard = TRUE;
				break;
			}
		}

		if (fNoDiscard)
		{
			fNoDiscard = FALSE;
			continue;
		}
		// discard all sound data at this 'rate' from cache
		if (pSource->SampleRate() == rate)
			pSource->CacheUnload();
	}
}


/*
==================
S_TouchSound

==================
*/
void S_TouchSound (char *name)
{
	CSfxTable *sfx;
	
	if (!g_AudioDevice->IsActive())
		return;

	sfx = S_FindName (name, NULL);

	if ( sfx->pSource )
		sfx->pSource->CacheTouch();
}

#endif

/*
==================
S_PrecacheSound

Reserve space for the name of the sound in a global array.
Load the data for the sound and assign a valid pointer, 
unless the sound is a streaming or sentence type.  These
defer loading of data until just before playback.
==================
*/
CSfxTable *S_PrecacheSound( const char *name )
{
	CSfxTable	*sfx;

	if ( !g_AudioDevice )
		return NULL;

	if ( !g_AudioDevice->IsActive() )
		return NULL;

	if ( TestSoundChar(name, CHAR_STREAM) || TestSoundChar(name, CHAR_SENTENCE) ) 
	{		
		// This is a streaming sound or a sentence name.
		// don't actually precache data, just store name

		sfx = S_FindName (name, NULL);
		return sfx;

	}
	else
	{
		// Entity sound.
		sfx = S_FindName (name, NULL);
	
		// cache sound
		S_LoadSound (sfx, NULL);
	
		return sfx;
	}
}

/*
=================
SND_PickDynamicChannel
Select a channel from the dynamic channel allocation area.  For the given entity, 
override any other sound playing on the same channel (see code comments below for
exceptions).
=================
*/
channel_t *SND_PickDynamicChannel(SoundSource soundsource, int entchannel, CSfxTable *sfx)
{
	int	ch_idx;
	int	first_to_die;
	int	life_left;

	// Check for replacement sound, or find the best one to replace
	first_to_die = -1;
	life_left = 0x7fffffff;
  
	ch_idx=0;

	for ( ; ch_idx < MAX_DYNAMIC_CHANNELS ; ch_idx++)
	{
		channel_t *ch = &channels[ch_idx];
		
		// Never override a streaming sound that is currently playing or
		// voice over IP data that is playing or any sound on CHAN_VOICE( acting )
		if ( ch->sfx && 
			( ch->entchannel == CHAN_STREAM ) )
		{
			continue;
		}

		if (entchannel != 0		// channel 0 never overrides
			&& !ch->delayed_start  // delayed channels are never overridden
			&& ch->soundsource == soundsource
			&& (ch->entchannel == entchannel || entchannel == -1) )
		{
			// always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (ch->sfx &&
			g_pSoundServices->IsPlayer( ch->soundsource ) && 
			!g_pSoundServices->IsPlayer(soundsource))
			continue;

		// try to pick the sound with the least amount of data left to play
		int timeleft = 0;
		if ( ch->sfx )
		{
			timeleft = 1;	//ch->end - paintedtime
		}

		if ( timeleft < life_left )
		{
			life_left = timeleft;
			first_to_die = ch_idx;
		}
	}

	if (first_to_die == -1)
		return NULL;

	if (channels[first_to_die].sfx)
	{
		// Don't restart looping sounds for the same entity
		CAudioSource *pSource = channels[first_to_die].sfx->pSource;
		if ( pSource )
		{
			if ( pSource->IsLooped() )
			{
				channel_t *ch = &channels[first_to_die];
				if ( ch->soundsource == soundsource && ch->entchannel == entchannel && ch->sfx == sfx )
				{
					// same looping sound, same ent, same channel, don't restart the sound
					return NULL;
				}
			}
		}
		// be sure and release previous channel
		// if sentence.
		//DevMsg("Stealing channel from %s\n", channels[first_to_die].sfx->getname() );
		S_FreeChannel(&(channels[first_to_die]));
	}

	return &channels[first_to_die];    
}

  			

/*
=====================
SND_PickStaticChannel
=====================
Pick an empty channel from the static sound area, or allocate a new
channel.  Only fails if we're at max_channels (128!!!) or if 
we're trying to allocate a channel for a stream sound that is 
already playing.

*/
channel_t *SND_PickStaticChannel(int soundsource, CSfxTable *pSfx)
{
	int i;
	channel_t *ch = NULL;


	// Check for replacement sound, or find the best one to replace
 	for (i = MAX_DYNAMIC_CHANNELS; i<total_channels; i++)
		if (channels[i].sfx == NULL)
			break;


	if (i < total_channels) 
	{
		// reuse an empty static sound channel
		ch = &channels[i];
	}
	else
	{
		// no empty slots, alloc a new static sound channel
		if (total_channels == MAX_CHANNELS)
		{
			DevMsg ("total_channels == MAX_CHANNELS\n");
			return NULL;
		}


		// get a channel for the static sound

		ch = &channels[total_channels];
		total_channels++;
	}

	return ch;
}


void S_SpatializeChannel( int volume[4], int master_vol, float gain, float dotRight )
{
	float lscale, rscale, scale;

	rscale = 1.0 + dotRight;
	lscale = 1.0 - dotRight;

 // add in distance effect
	scale = gain * rscale / 2;
	volume[IFRONT_RIGHT] = (int) (master_vol * scale);

	scale = gain * lscale / 2;
	volume[IFRONT_LEFT] = (int) (master_vol * scale);

	volume[IFRONT_RIGHT] = clamp( volume[IFRONT_RIGHT], 0, 255 );
	volume[IFRONT_LEFT] = clamp( volume[IFRONT_LEFT], 0, 255 );

}

// use listener_right/origin/forward/up, sound source origin & cone, distance to L/R/F/B/U/D walls and
// sound source direction to calculate stereo delay and directional filtering/attenuation

void SND_GetRoomDelay ( channel_t *ch )
{
	// UNDONE:
}

// calculate ammount of sound to be mixed to dsp, based on distance from listener

#define DSP_DIST_MIN	0.0				// range at which sounds are mixed at dsp_mix_min
#define DSP_DIST_MAX	(120.0 * 12.0)	// range at which sounds are mixed at dsp_mix_max
#define DSP_MIX_MIN		0.24	
#define DSP_MIX_MAX		0.76	

float	DSP_ROOM_MIX	= 1.0;	// mix volume of dsp_room sounds when added back to 'dry' sounds
float	DSP_NOROOM_MIX	= 1.0;	// mix volume of facing + facing away sounds. added to dsp_room_mix sounds

extern ConVar dsp_off;

// returns 0-1.0 dsp mix value.  If sound source is at a range >= DSP_DIST_MAX, return a mix value of
// DSP_MIX_MAX.  This mix value is used later to determine wet/dry mix ratio of sounds.

// UNDONE: this ramp should change with db level of sound and room size
//	0.78 is mix for sound 100% at far end of room, 0.24 is mix for sound 25% into room

float SND_GetDspMix( channel_t *pchannel, int idist)
{
	float mix;
	float dist = (float)idist;
	
	// dspmix is 0 (100% mix to facing buffer) if dsp_off

	if ( dsp_off.GetInt() )
		return 0.0;

	// doppler wavs are mixed dry

	if ( pchannel->wavtype == CHAR_DOPPLER )
		return 0.0;

	// linear ramp - get dry mix %
	
	// dist: 0->(max - min)

	dist = clamp( dist, DSP_DIST_MIN, DSP_DIST_MAX ) - DSP_DIST_MIN;

	// dist: 0->1.0

	dist = dist / (DSP_DIST_MAX - DSP_DIST_MIN);

	// mix: min->max

	mix = ((DSP_MIX_MAX - DSP_MIX_MIN) * dist) + DSP_MIX_MIN;
				

	return mix;
}

// calculate crossfade between wav left (close sound) and wav right (far sound) based on
// distance fron listener

#define DVAR_DIST_MIN	(20.0  * 12.0)		// play full 'near' sound at 20' or less
#define DVAR_DIST_MAX	(110.0 * 12.0)		// play full 'far' sound at 110' or more
#define DVAR_MIX_MIN	0.0
#define DVAR_MIX_MAX	1.0

// calculate mixing parameter for CHAR_DISTVAR wavs
// returns 0 - 1.0, 1.0 is 100% far sound (wav right)

float SND_GetDistanceMix( channel_t *pchannel, int idist)
{
	float mix;
	float dist = (float)idist;
	
	// doppler wavs are 100% near

	if ( pchannel->wavtype == CHAR_DOPPLER )
		return 0.0;

	// linear ramp - get dry mix %
	
	// dist 0->(max - min)

	dist = clamp( dist, DVAR_DIST_MIN, DVAR_DIST_MAX ) - DVAR_DIST_MIN;

	// dist 0->1.0

	dist = dist / (DVAR_DIST_MAX - DVAR_DIST_MIN);

	// mix min->max

	mix = ((DVAR_MIX_MAX - DVAR_MIX_MIN) * dist) + DVAR_MIX_MIN;
	
	return mix;
}

// given facing direction of source, and channel, 
// return -1.0 - 1.0, where -1.0 is source facing away from listener
// and 1.0 is source facing listener


float SND_GetFacingDirection( channel_t *pChannel, const QAngle &source_angles )
{
	Vector SF;				// sound source forward direction unit vector
	Vector SL;				// sound -> listener unit vector
	float dotSFSL;

	// no facing direction unless wavtyp CHAR_DIRECTIONAL

	if ( pChannel->wavtype != CHAR_DIRECTIONAL )
		return 1.0;
	
	VectorSubtract(listener_origin, pChannel->origin, SL);
	VectorNormalize(SL);

	// compute forward vector for sound entity

	AngleVectors( source_angles, &SF, NULL, NULL );

	// dot source forward unit vector with source to listener unit vector to get -1.0 - 1.0 facing.
	// ie: projection of SF onto SL

	dotSFSL = DotProduct( SF, SL );
		
	return dotSFSL;
}

// calculate point of closest approach - caller must ensure that the 
// forward facing vector of the entity playing this sound points in exactly the direction of 
// travel of the sound. ie: for bullets or tracers, forward vector must point in traceline direction.
// return true if sound is to be played, false if sound cannot be heard (shot away from player)

bool SND_GetClosestPoint( channel_t *pChannel, QAngle &source_angles, Vector &vnearpoint )
{
	// S - sound source origin
	// L - listener origin
	
	Vector SF;				// sound source forward direction unit vector
	Vector SL;				// sound -> listener vector
	Vector SD;				// sound->closest point vector
	vec_t dSLSF;			// magnitude of project of SL onto SF

	// P = SF (SF . SL) + S

	// only perform this calculation for doppler wavs

	if ( pChannel->wavtype != CHAR_DOPPLER )
		return false;

	// get vector 'SL' from sound source to listener

	VectorSubtract(listener_origin, pChannel->origin, SL);

	// compute sound->forward vector 'SF' for sound entity

	AngleVectors( source_angles, &SF );
	VectorNormalize( SF );
	
	dSLSF = DotProduct( SL, SF );

	if ( dSLSF <= 0 )
	{
		// source is pointing away from listener, don't play anything.
		return false;
	}
		
	// project dSLSF along forward unit vector from sound source
	
	VectorMultiply( SF, dSLSF, SD );

	// output vector - add SD to sound source origin

	VectorAdd( SD, pChannel->origin, vnearpoint );

	return true;
}


// given point of nearest approach and sound source facing angles, 
// return vector pointing into quadrant in which to play 
// doppler left wav (incomming) and doppler right wav (outgoing).

// doppler left is point in space to play left doppler wav
// doppler right is point in space to play right doppler wav

// Also modifies channel pitch based on distance to nearest approach point

#define DOPPLER_DIST_LEFT_TO_RIGHT	(4*12)		// separate left/right sounds by 4'

#define DOPPLER_DIST_MAX			(20*12)		// max distance - causes min pitch
#define DOPPLER_DIST_MIN			(1*12)		// min distance - causes max pitch
#define DOPPLER_PITCH_MAX			1.5			// max pitch change due to distance
#define DOPPLER_PITCH_MIN			0.25		// min pitch change due to distance

#define DOPPLER_RANGE_MAX			(10*12)		// don't play doppler wav unless within this range
												// UNDONE: should be set by caller!

void SND_GetDopplerPoints( channel_t *pChannel, QAngle &source_angles, Vector &vnearpoint, Vector &source_doppler_left, Vector &source_doppler_right)
{
	Vector SF;			// direction sound source is facing (forward)
	Vector LN;			// vector from listener to closest approach point
	Vector DL;
	Vector DR;

	// nearpoint is closest point of approach, when playing CHAR_DOPPLER sounds

	// SF is normalized vector in direction sound source is facing

	AngleVectors( source_angles, &SF );
	VectorNormalize( SF );

	// source_doppler_left - location in space to play doppler left wav (incomming)
	// source_doppler_right	- location in space to play doppler right wav (outgoing)
	
	VectorMultiply( SF, -1*DOPPLER_DIST_LEFT_TO_RIGHT, DL );
	VectorMultiply( SF, DOPPLER_DIST_LEFT_TO_RIGHT, DR );

	VectorAdd( vnearpoint, DL, source_doppler_left );
	VectorAdd( vnearpoint, DR, source_doppler_right );
	
	// set pitch of channel based on nearest distance to listener

	// LN is vector from listener to closest approach point

	VectorSubtract(vnearpoint, listener_origin, LN);

	float pitch;
	float dist = VectorLength( LN );
	
	// dist varies 0->1

	dist = clamp(dist, DOPPLER_DIST_MIN, DOPPLER_DIST_MAX);
	dist = (dist - DOPPLER_DIST_MIN) / (DOPPLER_DIST_MAX - DOPPLER_DIST_MIN);

	// pitch varies from max to min

	pitch = DOPPLER_PITCH_MAX - dist * (DOPPLER_PITCH_MAX - DOPPLER_PITCH_MIN);
	
	pChannel->basePitch = (int)(pitch * 100.0);
}

// console variables used to construct gain curve - don't change these!

ConVar snd_refdist( "snd_refdist", "36");
ConVar snd_refdb( "snd_refdb", "60" );
ConVar snd_foliage_db_loss( "snd_foliage_db_loss", "4" ); 
ConVar snd_gain( "snd_gain", "1" );
ConVar snd_gain_max( "snd_gain_max", "1" );
ConVar snd_gain_min( "snd_gain_min", "0.01" );

ConVar snd_showstart( "snd_showstart", "0" );	// showstart always skips info on player footsteps!
												// 1 - show sound name, channel, volume, time 
												// 2 - show dspmix, distmix, dspface, l/r/f/r vols
												// 3 - show sound origin coords
												// 4 - show gain of dsp_room
												// 5 - show dB loss due to obscured sound
												// 6 - reserved
												// 7 - show 2 and total gain & dist in ft. to sound source

extern bool SURROUND_ON;

// calculate gain based on atmospheric attenuation.
// as gain excedes threshold, round off (compress) towards 1.0 using spline

#define SND_GAIN_COMP_EXP_MAX	2.5		// Increasing SND_GAIN_COMP_EXP_MAX fits compression curve more closely
										// to original gain curve as it approaches 1.0.  
#define SND_GAIN_COMP_EXP_MIN	0.8	

#define SND_GAIN_COMP_THRESH	0.5		// gain value above which gain curve is rounded to approach 1.0

#define SND_DB_MAX				140.0	// max db of any sound source
#define SND_DB_MED				90.0	// db at which compression curve changes
#define SND_DB_MIN				60.0	// min db of any sound source

#define SND_GAIN_PLAYER_WEAPON_DB 2.0	// increase player weapon gain by N dB

// dB = 20 log (amplitude/32768)		0 to -90.3dB
// amplitude = 32768 * 10 ^ (dB/20)		0 to +/- 32768
// gain = amplitude/32768				0 to 1.0

inline float Gain_To_dB ( float gain )
{
	float dB = 20 * log ( gain );
	return dB;
}

inline float dB_To_Gain ( float dB )
{
	float gain = powf (10, dB / 20.0);
	return gain;
}

inline float Gain_To_Amplitude ( float gain )
{
	return gain * 32768;
}

inline float Amplitude_To_Gain ( float amplitude )
{
	return amplitude / 32768;
}

// The complete gain calculation, with SNDLVL given in dB is:
//
// GAIN = 1/dist * snd_refdist * 10 ^ ( ( SNDLVL - snd_refdb - (dist * snd_foliage_db_loss / 1200)) / 20 )
// 
//		for gain > SND_GAIN_THRESH, start curve smoothing with
//
// GAIN = 1 - 1 / (Y * GAIN ^ SND_GAIN_POWER)
// 
//		 where Y = -1 / ( (SND_GAIN_THRESH ^ SND_GAIN_POWER) * (SND_GAIN_THRESH - 1) )
//

// gain curve construction

float SND_GetGain( channel_t *ch, bool fplayersound, bool flooping, vec_t dist )
{
	float gain = snd_gain.GetFloat();

	if ( ch->dist_mult )
	{
		// test additional attenuation
		// at 30c, 14.7psi, 60% humidity, 1000Hz == 0.22dB / 100ft.
		// dense foliage is roughly 2dB / 100ft

		float additional_dB_loss = snd_foliage_db_loss.GetFloat() * (dist / 1200);
		float additional_dist_mult = pow( 10, additional_dB_loss / 20);

		float relative_dist = dist * ch->dist_mult * additional_dist_mult;

		// hard code clamp gain to 10x normal (assumes volume and external clipping)

		if (relative_dist > 0.1)
		{
			gain *= (1/relative_dist);
		}
		else
			gain *= 10.0;

		// if gain passess threshold, compress gain curve such that gain smoothly approaches 1.0

		if ( gain > SND_GAIN_COMP_THRESH )
		{
			float snd_gain_comp_power = SND_GAIN_COMP_EXP_MAX;
			soundlevel_t sndlvl = DIST_MULT_TO_SNDLVL( ch->dist_mult );
			float Y;
			
			// decrease compression curve fit for higher sndlvl values

			if ( sndlvl > SND_DB_MED )
			{
				// snd_gain_power varies from max to min as sndlvl varies from 90 to 140

				snd_gain_comp_power = RemapVal ((float)sndlvl, SND_DB_MED, SND_DB_MAX, SND_GAIN_COMP_EXP_MAX, SND_GAIN_COMP_EXP_MIN);
			}

			// calculate crossover point

			Y = -1.0 / ( pow(SND_GAIN_COMP_THRESH, snd_gain_comp_power) * (SND_GAIN_COMP_THRESH - 1) );
			
			// calculate compressed gain

			gain = 1.0 - 1.0 / (Y * pow( gain, snd_gain_comp_power ) );

			gain = gain * snd_gain_max.GetFloat();
		}

		if ( gain < snd_gain_min.GetFloat() )
		{
			// sounds less than snd_gain_min fall off to 0 in distance it took them to fall to snd_gain_min

			gain = snd_gain_min.GetFloat() * (2.0 - relative_dist * snd_gain_min.GetFloat());
			
			if (gain <= 0.0)
				gain = 0.001;	// don't propagate 0 gain
		}
	}

	if ( fplayersound )
	{
		
		// player weapon sounds get extra gain - this compensates
		// for npc distance effect weapons which mix louder as L+R into L,R
		// Hack.

		if ( ch->entchannel == CHAN_WEAPON )
			gain = gain * dB_To_Gain( SND_GAIN_PLAYER_WEAPON_DB );
	}

	// modify gain if sound source not visible to player

	gain = gain * SND_GetGainObscured( ch, fplayersound, flooping );

	if (snd_showstart.GetInt() == 6)
	{
		DevMsg( "(gain %1.3f : dist ft %1.1f) ", gain, (float)dist/12.0 );
		snd_showstart.SetValue(5);	// display once
	}

	return gain; 
}

// always ramp channel gain changes over time
// returns ramped gain, given new target gain

#define SND_GAIN_FADE_TIME	0.25		// xfade seconds between obscuring gain changes

float SND_FadeToNewGain( channel_t *ch, float gain_new )
{

	if ( gain_new == -1.0 )
	{
		// if -1 passed in, just keep fading to existing target

		gain_new = ch->ob_gain_target;
	}

	// if first time updating, store new gain into gain & target, return
	// if gain_new is close to existing gain, store new gain into gain & target, return

	if ( ch->bfirstpass || (fabs (gain_new - ch->ob_gain) < 0.01))
	{
		ch->ob_gain			= gain_new;
		ch->ob_gain_target	= gain_new;
		ch->ob_gain_inc		= 0.0;
		return gain_new;
	}

	// set up new increment to new target
	
	float frametime = g_pSoundServices->GetHostFrametime();
	float speed;
	speed = ( frametime / SND_GAIN_FADE_TIME ) * (gain_new - ch->ob_gain);

	ch->ob_gain_inc = fabs(speed);

	// ch->ob_gain_inc = fabs(gain_new - ch->ob_gain) / 10.0;
	
	ch->ob_gain_target = gain_new;

	// if not hit target, keep approaching
	
	if ( fabs( ch->ob_gain - ch->ob_gain_target ) > 0.01 )
	{
		ch->ob_gain = Approach( ch->ob_gain_target, ch->ob_gain, ch->ob_gain_inc );
	}
	else
	{
		// close enough, set gain = target
		ch->ob_gain = ch->ob_gain_target;
	}

	return ch->ob_gain;
}

#define SND_TRACE_UPDATE_MAX  2			// max of N channels may be checked for obscured source per frame

static g_snd_trace_count = 0;
static g_snd_last_trace_chan = 0;

// All new sounds must traceline once,
// but cap the max number of tracelines performed per frame
// for longer or looping sounds to SND_TRACE_UPDATE_MAX.

bool SND_ChannelOkToTrace( channel_t *ch )
{
	int i, j;

	// always trace first time sound is spatialized

	if ( ch->bfirstpass )
		return true;

	// if already traced max channels, return

	if ( g_snd_trace_count >= SND_TRACE_UPDATE_MAX )
		return false;

	// search through all channels starting at g_snd_last_trace_chan index
	
	j = g_snd_last_trace_chan;

 	for (i = 0; i<total_channels; i++)
	{
		if ( &(channels[j]) == ch )
		{
			ch->bTraced = true;
			g_snd_trace_count++;
			return true;
		}

		// wrap channel index

		j++;
		if ( j >= total_channels )
			j = 0;
	}
	
	Assert(false);	// why didn't we find this channel?
	return false;			
}

// reset counters for traceline limiting per audio update

void SND_ChannelTraceReset( void )
{
	// reset search point - make sure we start counting from a new spot 
	// in channel list each time

	g_snd_last_trace_chan += SND_TRACE_UPDATE_MAX;
	
	// wrap at total_channels

	if (g_snd_last_trace_chan >= total_channels)
		g_snd_last_trace_chan = g_snd_last_trace_chan - total_channels;

	// reset traceline counter

	g_snd_trace_count = 0;

	// reset channel traceline flag

	for (int i = 0; i < total_channels; i++)
		channels[i].bTraced = false; 
}


extern Vector	r_origin;

static float g_snd_obscured_loss_db = -2.70;	// dB loss due to obscured sound source

// drop gain on channel if sound emitter obscured by
// world, unbroken windows, closed doors, large solid entities etc.

float SND_GetGainObscured( channel_t *ch, bool fplayersound, bool flooping )
{
	float gain = 1.0;
	int count = 1;

	if ( fplayersound )
		return gain;

	// During signon just apply regular state machine since world hasn't been
	//  created or settled yet...
	if ( cls.state != ca_active )
	{
		gain = SND_FadeToNewGain( ch, -1.0 );
		return gain;
	}

	// don't do gain obscuring more than once on short one-shot sounds

	if ( !ch->bfirstpass && !ch->isSentence && !flooping && (ch->entchannel != CHAN_STREAM) )
	{
		gain = SND_FadeToNewGain( ch, -1.0 );
		return gain;
	}

	// if long or looping sound, process N channels per frame - set 'processed' flag, clear by
	// cycling through all channels - this maintains a cap on traces per frame

	if ( !SND_ChannelOkToTrace( ch ) )
	{
		// just keep updating fade to existing target gain - no new trace checking

		gain = SND_FadeToNewGain( ch, -1.0 );
		return gain;
	}
	// set up traceline from player eyes to sound emitting entity origin

	Vector endpoint = ch->origin;
	
	trace_t tr;
	CTraceFilterWorldOnly filter;	// UNDONE: also test for static props?
	Ray_t ray;
	ray.Init( r_origin, endpoint );
	g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, &filter, &tr );

	if (tr.DidHit() && tr.fraction < 0.99)
	{
		// can't see center of sound source:
		// build extents based on dB sndlvl of source,
		// test to see how many extents are visible,
		// drop gain by g_snd_obscured_loss_db per extent hidden

		Vector endpoints[4];
		soundlevel_t sndlvl = DIST_MULT_TO_SNDLVL( ch->dist_mult );
		float radius;
		Vector vsrc_forward;
		Vector vsrc_right;
		Vector vsrc_up;
		Vector vecl;
		Vector vecr;
		Vector vecl2;
		Vector vecr2;
		int i;

		// get radius
		
		if ( ch->radius > 0 )
			radius = ch->radius;
		else
			radius = dB_To_Radius( sndlvl);		// approximate radius from soundlevel
		
		// set up extent endpoints - on upward or downward diagonals, facing player

		for (i = 0; i < 4; i++)
			endpoints[i] = endpoint;

		// vsrc_forward is normalized vector from sound source to listener

		VectorSubtract( listener_origin, endpoint, vsrc_forward );
		VectorNormalize( vsrc_forward );
		VectorVectors( vsrc_forward, vsrc_right, vsrc_up );

		VectorAdd( vsrc_up, vsrc_right, vecl );
		
		// if src above listener, force 'up' vector to point down - create diagonals up & down

		if ( endpoint.z > listener_origin.z + (10 * 12) )
			vsrc_up.z = -vsrc_up.z;

		VectorSubtract( vsrc_up, vsrc_right, vecr );
		VectorNormalize( vecl );
		VectorNormalize( vecr );

		// get diagonal vectors from sound source 

		vecl2 = radius * vecl;
		vecr2 = radius * vecr;
		vecl = (radius / 2.0) * vecl;
		vecr = (radius / 2.0) * vecr;

		// endpoints from diagonal vectors

		endpoints[0] += vecl;
		endpoints[1] += vecr;
		endpoints[2] += vecl2;
		endpoints[3] += vecr2;

		// drop gain for each point on radius diagonal that is obscured

		for (count = 0, i = 0; i < 4; i++)
		{
			// UNDONE: some endpoints are in walls - in this case, trace from the wall hit location

			Ray_t ray;
			ray.Init( r_origin, endpoints[i] );
			g_pEngineTraceClient->TraceRay( ray, MASK_OPAQUE, &filter, &tr );
			
			if (tr.DidHit() && tr.fraction < 0.99 && !tr.startsolid )
			{
				count++;	// skip first obscured point: at least 2 points + center should be obscured to hear db loss
				if (count > 1)
					gain = gain * dB_To_Gain( g_snd_obscured_loss_db );
			}
		}
	}

	
if ( flooping && snd_showstart.GetInt() == 7)
{
	static float g_drop_prev = 0;
	float drop = (count-1) * g_snd_obscured_loss_db;

	if (drop != g_drop_prev)
	{
		DevMsg( "dB drop: %1.4f \n", drop);
		g_drop_prev = drop;
	}
}

	// crossfade to new gain

	gain = SND_FadeToNewGain( ch, gain );

	return gain;
}

// convert sound db level to approximate sound source radius,
// used only for determining how much of sound is obscured by world

#define SND_RADIUS_MAX		(20.0 * 12.0)	// max sound source radius
#define SND_RADIUS_MIN		(2.0 * 12.0)	// min sound source radius

inline float dB_To_Radius ( float db )
{
	float radius = SND_RADIUS_MIN + (SND_RADIUS_MAX - SND_RADIUS_MIN) * (db - SND_DB_MIN) / (SND_DB_MAX - SND_DB_MIN);

	return radius;
}


/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize(channel_t *ch)
{
    vec_t dotRight = 0;
	vec_t dotFront = 0;
    vec_t dist;
    Vector source_vec;
	Vector source_vec_DL;
	Vector source_vec_DR;
	Vector source_doppler_left;
	Vector source_doppler_right;
	vec_t dotRightDL = 0;
	vec_t dotFrontDL = 0;
	vec_t dotRightDR = 0;
	vec_t dotFrontDR = 0;
	bool fdopplerwav = false;
	bool fplaydopplerwav = false;
	bool fvalidentity;
	float gain;
	bool fplayersound = false;

	ch->dspface = 1.0;				// default facing direction: always facing player
	ch->dspmix = 0;					// default mix 0% dsp_room fx
	ch->distmix = 0;				// default 100% left (near) wav

	if (ch->soundsource == g_pSoundServices->GetViewEntity())
	{

		// sounds coming from listener actually come from a short distance directly in front of listener

		fplayersound = true;
		
/*
		// sounds coming from listener are always spatialized 75% front, 25% back
		// are assumed to be facing forward. Channel wavtype is ignored here.

		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;

		if ( SURROUND_ON )
		{
			ch->leftvol		= ch->master_vol * 0.75;
			ch->rightvol	= ch->master_vol * 0.75;
			ch->rleftvol	= ch->master_vol * 0.25;
			ch->rrightvol	= ch->master_vol * 0.25;

			ch->dleftvol	= 0;
			ch->drightvol	= 0;
			ch->drleftvol	= 0;
			ch->drrightvol	= 0;
		}

		ch->dspmix = SND_GetDspMix( ch, 0);

		// set volume for current word being spoken

		VOX_SetChanVol(ch);

		return;
*/
	}

	// update channel's position in case ent that made the sound is moving.
	QAngle source_angles;
	source_angles.Init(0.0, 0.0, 0.0);
	Vector entOrigin = ch->origin;
	
	bool looping = false;

	CAudioSource *pSource = ch->sfx ? ch->sfx->pSource : NULL;
	if ( pSource )
	{
		looping = pSource->IsLooped();
	}
	
	SpatializationInfo_t si;
	si.info.Set( 
		ch->soundsource,
		ch->entchannel,
		ch->sfx ? ch->sfx->getname() : "",
		ch->origin,
		ch->direction,
		ch->master_vol,
		DIST_MULT_TO_SNDLVL( ch->dist_mult ),
		looping,
		ch->pitch );

	si.type = SpatializationInfo_t::SI_INSPATIALIZATION;
	si.pOrigin =	&entOrigin;
	si.pAngles =	&source_angles;
	si.pflRadius	= NULL;

	fvalidentity = g_pSoundServices->GetSoundSpatialization( ch->soundsource, si );
	if ( ch->bUpdatePositions )
	{
		AngleVectors( source_angles, &ch->direction );
		ch->origin = entOrigin;
	}
	else
	{
		VectorAngles( ch->direction, source_angles );
	}

#if 0
	// !!!UNDONE - above code assumes the ENT hasn't been removed or respawned as another ent!
	// !!!UNDONE - fix this by flagging some entities (ie: glass) as immobile.  Don't spatialize them.

	if ( !fvalidendity)
	{
		// Turn off the sound while the entity doesn't exist or is not in the PVS.

		goto ClearAllVolumes;
	}
#endif // 0

	
	fdopplerwav = ((ch->wavtype == CHAR_DOPPLER) && !fplayersound);

	if ( fdopplerwav )
	{
		Vector vnearpoint;				// point of closest approach to listener, 
										// along sound source forward direction (doppler wavs)

		vnearpoint = ch->origin;		// default nearest sound approach point

		// calculate point of closest approach for CHAR_DOPPLER wavs, replace source_vec

		fplaydopplerwav = SND_GetClosestPoint( ch, source_angles, vnearpoint );

		// if doppler sound was 'shot' away from listener, don't play it

		if ( !fplaydopplerwav )
			goto ClearAllVolumes;

		// find location of doppler left & doppler right points

		SND_GetDopplerPoints( ch, source_angles, vnearpoint, source_doppler_left, source_doppler_right);
	
		// source_vec_DL is vector from listener to doppler left point
		// source_vec_DR is vector from listener to doppler right point

		VectorSubtract(source_doppler_left, listener_origin, source_vec_DL );
		VectorSubtract(source_doppler_right, listener_origin, source_vec_DR );

		// normalized vectors to left and right doppler locations

		dist = VectorNormalize( source_vec_DL );
		VectorNormalize( source_vec_DR );

		// don't play doppler if out of range

		if (dist > DOPPLER_RANGE_MAX)
			goto ClearAllVolumes;
	}
	else
	{
		// source_vec is vector from listener to sound source

		if ( fplayersound )
			// player sounds come from 1' in front of player			
			VectorMultiply(listener_forward, 12.0, source_vec );
		else
			VectorSubtract(ch->origin, listener_origin, source_vec);

		// normalize source_vec and get distance from listener to source

		dist = VectorNormalize( source_vec );
	}
	
	// calculate dsp mix based on distance to listener (linear approximation)

	ch->dspmix = SND_GetDspMix( ch, dist );

	// calculate sound source facing direction for CHAR_DIRECTIONAL wavs

	if ( !fplayersound )
	{
		ch->dspface = SND_GetFacingDirection( ch, source_angles );
		
		// calculate mixing parameter for CHAR_DISTVAR wavs

		ch->distmix = SND_GetDistanceMix( ch, dist );
	}

	// get right and front dot products for all sound sources
	// dot: 1.0 - source is directly to right of listener, -1.0 source is directly left of listener

	if ( fdopplerwav )
	{
		// get right/front dot products for left doppler position

		dotRightDL  = DotProduct(listener_right,   source_vec_DL);
		dotFrontDL  = DotProduct(listener_forward, source_vec_DL);

		// get right/front dot products for right doppler position

		dotRightDR = DotProduct(listener_right,   source_vec_DR);
		dotFrontDR = DotProduct(listener_forward, source_vec_DR);
	}
	else
	{
		dotRight = DotProduct(listener_right,   source_vec);
		dotFront = DotProduct(listener_forward, source_vec);
	}

	// for sounds with a radius, spatialize left/right/front/rear evenly within the radius

	if ( ch->radius > 0 && dist < ch->radius && !fdopplerwav )
	{
		float interval = ch->radius * 0.5;
		float blend = dist - interval;
		if ( blend < 0 )
			blend = 0;
		blend /= interval;	

		// blend is 0.0 - 1.0, from 50% radius -> 100% radius

		// at radius * 0.5, dotRight is 0 (ie: sound centered left/right)
		// at radius dotRight == dotRight

		dotRight   *= blend;
		dotRightDL *= blend;
		dotRightDR *= blend;

		// at radius * 0.5, dotFront is 1 (ie: sound centered front/back)
		// at radius dotFront == dotFront

		dotFront   = 1.0 + (dotFront - 1.0)*blend;
		dotFrontDL = 1.0 + (dotFrontDL - 1.0)*blend;
		dotFrontDR = 1.0 + (dotFrontDR - 1.0)*blend;
	}

	// calculate gain based on distance, atmospheric attenuation, interposed objects
	// perform compression as gain approaches 1.0

	gain = SND_GetGain( ch, fplayersound, looping, dist );
	
	// don't pan sounds with no attenuation
	if ( ch->dist_mult <= 0 && !fdopplerwav )
	{
		// sound is centered left/right/front/back

		dotRight = 0.0;
		dotRightDL = 0.0;
		dotRightDR = 0.0;

		dotFront = 1.0;
		dotFrontDL = 1.0;
		dotFrontDR = 1.0;
	}

	if ( fdopplerwav )
	{
		// fill out channel volumes for both doppler locations

		g_AudioDevice->SpatializeChannel( &ch->leftvol,  ch->master_vol, source_vec_DL, gain, dotRightDL, dotFrontDL );
		g_AudioDevice->SpatializeChannel( &ch->dleftvol, ch->master_vol, source_vec_DR, gain, dotRightDR, dotFrontDR );		
	}
	else
	{
		// fill out channel volumes for single location

		g_AudioDevice->SpatializeChannel( &ch->leftvol, ch->master_vol, source_vec, gain, dotRight, dotFront );
	}

 // if playing a word, set volume
		
	VOX_SetChanVol(ch);

	// end of first time spatializing sound
	if ( cls.state == ca_active )
	{
		ch->bfirstpass = false;
	}

	return;

ClearAllVolumes:

	// Clear all volumes and return. 
	// This shuts the sound off permanently.

	ch->leftvol = ch->rightvol = ch->rleftvol = ch->rrightvol = 0;
	ch->dleftvol = ch->drightvol = ch->drleftvol = ch->drrightvol = 0;

	// end of first time spatializing sound

	ch->bfirstpass = false;

	return;
}           

// search through all channels for a channel that matches this
// soundsource, entchannel and sfx, and perform alteration on channel
// as indicated by 'flags' parameter. If shut down request and
// sfx contains a sentence name, shut off the sentence.
// returns TRUE if sound was altered,
// returns FALSE if sound was not found (sound is not playing)

int S_AlterChannel( int soundsource, int entchannel, CSfxTable *sfx, int vol, int pitch, int flags )
{
	int ch_idx;

	if ( TestSoundChar(sfx->getname(), CHAR_SENTENCE) )
	{
		// This is a sentence name.
		// For sentences: assume that the entity is only playing one sentence
		// at a time, so we can just shut off
		// any channel that has ch->isentence >= 0 and matches the
		// soundsource.

		for (ch_idx = 0 ; ch_idx < total_channels; ch_idx++)
		{
			if (channels[ch_idx].soundsource == soundsource
				&& channels[ch_idx].entchannel == entchannel
				&& channels[ch_idx].sfx != NULL
				&& channels[ch_idx].isSentence )
			{

				if (flags & SND_CHANGE_PITCH)
					channels[ch_idx].basePitch = pitch;
				
				if (flags & SND_CHANGE_VOL)
					channels[ch_idx].master_vol = vol;
				
				if (flags & SND_STOP)
				{
					S_FreeChannel(&channels[ch_idx]);
				}
			
				return TRUE;
			}
		}
		// channel not found
		return FALSE;

	}

	// regular sound or streaming sound

	for (ch_idx = 0 ; ch_idx < total_channels; ch_idx++)
    {
		if (channels[ch_idx].soundsource == soundsource
			&& channels[ch_idx].entchannel == entchannel
			&& channels[ch_idx].sfx == sfx )
		{
			if (flags & SND_CHANGE_PITCH)
				channels[ch_idx].basePitch = pitch;
			
			if (flags & SND_CHANGE_VOL)
				channels[ch_idx].master_vol = vol;
			
			if (flags & SND_STOP)
			{
				S_FreeChannel(&channels[ch_idx]);
			}
		
			return TRUE;
		}
   }

	return FALSE;
}

// set channel flags during initialization based on 
// source name

void S_SetChannelWavtype( channel_t *target_chan, CSfxTable *pSfx )
{	
	// if 1st or 2nd character of name is CHAR_DRYMIX, sound should be mixed dry with no dsp (ie: music)

	if ( TestSoundChar(pSfx->getname(), CHAR_DRYMIX) )
		target_chan->bdry = true;
	
	// get sound spatialization encoding
	
	target_chan->wavtype = 0;

	if ( TestSoundChar( pSfx->getname(), CHAR_DOPPLER ))
		target_chan->wavtype = CHAR_DOPPLER;
	
	if ( TestSoundChar( pSfx->getname(), CHAR_DIRECTIONAL ))
		target_chan->wavtype = CHAR_DIRECTIONAL;

	if ( TestSoundChar( pSfx->getname(), CHAR_DISTVARIANT ))
		target_chan->wavtype = CHAR_DISTVARIANT;

	if ( TestSoundChar( pSfx->getname(), CHAR_OMNI ))
		target_chan->wavtype = CHAR_OMNI;
}


// Sets bstereowav flag in channel if source is true stere wav
// sets default wavtype for stereo wavs to CHAR_DISTVARIANT - 
// ie: sound varies with distance (left is close, right is far)
// Must be called after S_SetChannelWavtype

void S_SetChannelStereo( channel_t *target_chan, CAudioSource *pSource )
{
	if ( !pSource )
	{
		target_chan->bstereowav = false;
		return;
	}
	
	// returns true only if source data is a stereo wav file. 
	// ie: mp3, voice, sentence are all excluded.

	target_chan->bstereowav = pSource->IsStereoWav();

	// Default stereo wavtype:
	// if playing a stereo wav file, with no preset wavtype, not mixing dry,
	// then assume wav file is stereo encoded as CHAR_DISTVARIANT - varies with distance.

	// just player standard stereo wavs on player entity - no override.

	if (target_chan->soundsource == g_pSoundServices->GetViewEntity())
		return;
	
	// default wavtype for stereo wavs is DISTVARIANT - except for drymix or sounds with 0 attenuation

	if ( target_chan->bstereowav && !target_chan->wavtype && !target_chan->bdry && target_chan->dist_mult )
		target_chan->wavtype = CHAR_DISTVARIANT;
}



// =======================================================================
// S_StartDynamicSound
// =======================================================================
// Start a sound effect for the given entity on the given channel (ie; voice, weapon etc).  
// Try to grab a channel out of the 8 dynamic spots available.
// Currently used for looping sounds, streaming sounds, sentences, and regular entity sounds.
// NOTE: volume is 0.0 - 1.0 and attenuation is 0.0 - 1.0 when passed in.
// Pitch changes playback pitch of wave by % above or below 100.  Ignored if pitch == 100

// NOTE: it's not a good idea to play looping sounds through StartDynamicSound, because
// if the looping sound starts out of range, or is bumped from the buffer by another sound
// it will never be restarted.  Use StartStaticSound (pass CHAN_STATIC to EMIT_SOUND or
// SV_StartSound.


void S_StartDynamicSound(int soundsource, int entchannel, CSfxTable *pSfx, const Vector &origin, const Vector &direction, bool bUpdatePositions, float fvol, soundlevel_t soundlevel, int flags, int pitch, bool fromserver, float delay /*=0.0f*/ )
{
	channel_t *target_chan;
	int		vol;
	int		fsentence = 0;

	if (!g_AudioDevice->IsActive())
		return;

	if (!pSfx)
		return;

	//Msg("Start sound %s\n", pSfx->getname() );
	// override the entchannel to CHAN_STREAM if this is a 
	// stream sound.
	if ( TestSoundChar(pSfx->getname(), CHAR_STREAM ))
		entchannel = CHAN_STREAM;
	
	vol = fvol*255;
	
	if (vol > 255)
	{
		DevMsg("S_StartDynamicSound: %s volume > 255", pSfx->getname() );
		vol = 255;
	}

	if ( flags & (SND_STOP|SND_CHANGE_VOL|SND_CHANGE_PITCH) )
	{
		if ( S_AlterChannel(soundsource, entchannel, pSfx, vol, pitch, flags) )
			return;
		if ( flags & SND_STOP )
			return;
		// fall through - if we're not trying to stop the sound, 
		// and we didn't find it (it's not playing), go ahead and start it up
	}

	if (pitch == 0)
	{
		DevMsg ("Warning: S_StartDynamicSound (%s) Ignored, called with pitch 0\n", pSfx->getname() );
		return;
	}

// pick a channel to play on
	target_chan = SND_PickDynamicChannel(soundsource, entchannel, pSfx);

	if (!target_chan)
		return;

	int channelIndex = (int) ( target_chan - channels );
	g_AudioDevice->ChannelReset( soundsource, channelIndex, target_chan->dist_mult );

#ifdef DEBUG_CHANNELS
	{
		char szTmp[128];
		sprintf(szTmp, "Sound %s playing on Dynamic game channel %d\n", pSfx->getname(), IWavstreamOfCh(target_chan));
		OutputDebugString(szTmp);
	}
#endif
	
	
	if ( TestSoundChar(pSfx->getname(), CHAR_SENTENCE) )
		fsentence = 1;

// spatialize
	memset (target_chan, 0, sizeof(*target_chan));
	VectorCopy(origin, target_chan->origin);
	VectorCopy(direction, target_chan->direction);
	
	// never update positions if source entity is 0

	target_chan->bUpdatePositions = bUpdatePositions && (soundsource == 0 ? 0 : 1);

	// reference_dist / (reference_power_level / actual_power_level)
	target_chan->dist_mult = SNDLVL_TO_DIST_MULT( soundlevel );

	S_SetChannelWavtype( target_chan, pSfx );

	target_chan->master_vol = vol;
	target_chan->soundsource = soundsource;
	target_chan->entchannel = entchannel;
	target_chan->basePitch = pitch;
	target_chan->isSentence = false;
	target_chan->radius = 0;
	target_chan->sfx = pSfx;
	target_chan->fromserver = fromserver;
	
	// initialize gain due to obscured sound source
	target_chan->bfirstpass = true;
	target_chan->ob_gain = 0.0;
	target_chan->ob_gain_inc = 0.0;
	target_chan->ob_gain_target = 0.0;
	target_chan->bTraced = false;

	CAudioSource *pSource = NULL;
	if (fsentence)
	{
		// this is a sentence
		// link all words and load the first word

		// NOTE: sentence names stored in the cache lookup are
		// prepended with a '!'.  Sentence names stored in the
		// sentence file do not have a leading '!'. 

		VOX_LoadSound( target_chan, PSkipSoundChars(pSfx->getname()) );
	}
	else
	{
		// regular or streamed sound fx
		pSource = S_LoadSound( pSfx, target_chan );
	}

	if (!target_chan->pMixer)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}

	S_SetChannelStereo( target_chan, pSource );

	if (snd_showstart.GetInt() == 5)	
		snd_showstart.SetValue(6);		// debug: show gain for next spatialize only

	SND_Spatialize(target_chan);

	if (snd_showstart.GetInt() > 0 && snd_showstart.GetInt() < 7 && snd_showstart.GetInt() != 4)
	{
		channel_t *ch = target_chan;

		DevMsg( "DynamicSound %s : src %d : channel %d : %d dB : vol %.2f : time %.3f\n", pSfx->getname(), soundsource, entchannel, soundlevel, fvol, g_pSoundServices->GetHostTime() );
		if (snd_showstart.GetInt() == 2 || snd_showstart.GetInt() == 5)
			DevMsg( "\t dspmix %1.2f : distmix %1.2f : dspface %1.2f : lvol %d : rvol %d : rlvol %d : rrvol %d\n", ch->dspmix, ch->distmix, ch->dspface, ch->leftvol, ch->rightvol, ch->rleftvol, ch->rrightvol );
		if (snd_showstart.GetInt() == 3)
			DevMsg( "\t x: %4f y: %4f z: %4f\n", ch->origin.x, ch->origin.y, ch->origin.z );
	}

	// If a client can't hear a sound when they FIRST receive the StartSound message,
	// the client will never be able to hear that sound. This is so that out of 
	// range sounds don't fill the playback buffer.  For streaming sounds, we bypass this optimization.
	if (!target_chan->leftvol && !target_chan->rightvol && !target_chan->rleftvol && !target_chan->rrightvol)
	{
		// Looping sounds don't use this optimization because they should stick around until they're killed.
		// Also bypass for speech (GetSentence)
		if ( !pSfx->pSource || (!pSfx->pSource->IsLooped() && !pSfx->pSource->GetSentence()) )
		{
			// if this is a streaming sound, play
			// the whole thing.

			if (entchannel != CHAN_STREAM)
			{
				// DevMsg("S_StartDynamicSound: spatialized to 0 vol & ignored %s", pSfx->getname());
				S_FreeChannel( target_chan );
				return;		// not audible at all
			}
		}
	}

	// Init client entity mouth movement vars
	SND_InitMouth(soundsource, entchannel);

	// Pre-startup delay.  Compute # of samples over which to mix in zeros from data source before
	//  actually reading first set of samples
	if ( delay != 0.0f )
	{
		Assert( target_chan->sfx );
		Assert( target_chan->sfx->pSource );

		float rate = target_chan->sfx->pSource->SampleRate();
		int delaySamples = (int)( delay * rate );

		if ( delay > 0 )
		{
			Assert( delay <= (float)MAX_SOUND_DELAY_MSEC / 1000.0f );
			target_chan->pMixer->SetStartupDelaySamples( delaySamples );
			target_chan->delayed_start = true;
		}
		else
		{
			int skipSamples = -delaySamples;
			int totalSamples = target_chan->sfx->pSource->SampleCount();
			if ( skipSamples >=  totalSamples )
			{
				S_FreeChannel( target_chan );
				return;
			}

			target_chan->pMixer->SkipSamples( 0, skipSamples, rate, 0 );
			target_chan->ob_gain_target		= 1.0f;
			target_chan->ob_gain			= 1.0f;
			target_chan->ob_gain_inc		= 0.0;
			target_chan->bfirstpass			= false;
		}
	}

	/*
// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder

// UNDONE: this should be implemented using a start delay timer in the 
// mixer, instead of skipping forward in the wave.  Either method also cause
// phasing problems for skip times of < 10 milliseconds. KB

	int		ch_idx;
	int		skip;
	channel_t *check = &channels[0];
    for (ch_idx=0 ; ch_idx < MAX_DYNAMIC_CHANNELS ; ch_idx++, check++)
    {
		if (check == target_chan)
			continue;
		if (check->sfx == pSfx && !check->pMixer->GetSamplePosition())
		{
			skip = g_pSoundServices->RandomLong(0,(long)(0.1*check->sfx->pSource->SampleRate()));		// skip up to 0.1 seconds of audio
			check->pMixer->SetSampleStart( skip );
			break;
		}
		
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : CSfxTable
//-----------------------------------------------------------------------------
CSfxTable *S_DummySfx( const char *name )
{
	dummySfx.setname( name );
	return &dummySfx;
}

/*
=================
S_StartStaticSound
=================
Start playback of a sound, loaded into the static portion of the channel array.
Currently, this should be used for looping ambient sounds, looping sounds
that should not be interrupted until complete, non-creature sentences,
and one-shot ambient streaming sounds.  Can also play 'regular' sounds one-shot,
in case designers want to trigger regular game sounds.
Pitch changes playback pitch of wave by % above or below 100.  Ignored if pitch == 100

  NOTE: volume is 0.0 - 1.0 and attenuation is 0.0 - 1.0 when passed in.
*/
void S_StartStaticSound (int soundsource, int entchannel, CSfxTable *pSfx, const Vector &origin, const Vector &direction, bool bUpdatePositions, float fvol, soundlevel_t soundlevel, int flags, int pitch, bool fromserver, float delay /*=0.0f*/ )
{
	channel_t	*ch;
	int fvox = 0;
	int vol;
	CAudioSource *pSource = NULL;

	if ( !g_AudioDevice->IsActive() )
		return;

	if (!pSfx)
		return;

//	Msg("Start static sound %s\n", pSfx->getname() );
	vol = fvol * 255;

	if (vol > 255)
	{
		DevMsg("S_StartStaticSound: %s volume > 255", pSfx->getname());
		vol = 255;
	}

	if ((flags & SND_STOP) || (flags & SND_CHANGE_VOL) || (flags & SND_CHANGE_PITCH))
	{
		if (S_AlterChannel(soundsource, entchannel, pSfx, vol, pitch, flags) || (flags & SND_STOP))
			return;
	}
	
	if (pitch == 0)
	{
		DevMsg ("Warning: S_StartStaticSound Ignored, called with pitch 0");
		return;
	}
	
	
	// First, make sure the sound source entity is even in the PVS.
	float flSoundRadius = 0.0f;	
	
	bool looping = false;

	/*
	CAudioSource *pSource = pSfx ? pSfx->pSource : NULL;
	if ( pSource )
	{
		looping = pSource->IsLooped();
	}
	*/

	SpatializationInfo_t si;
	si.info.Set( 
		soundsource,
		entchannel,
		pSfx ? pSfx->getname() : "",
		origin,
		direction,
		vol,
		soundlevel,
		looping,
		pitch );

	si.type = SpatializationInfo_t::SI_INCREATION;
	
	si.pOrigin		= NULL;
	si.pAngles		= NULL;
	si.pflRadius	= &flSoundRadius;

	g_pSoundServices->GetSoundSpatialization( soundsource, si );

	// pick a channel to play on from the static area
	ch = SND_PickStaticChannel(soundsource, pSfx); // Autolooping sounds are always fixed origin(?)

	if (!ch)
		return;

	int channelIndex = ch - channels;
	g_AudioDevice->ChannelReset( soundsource, channelIndex, ch->dist_mult );

#ifdef DEBUG_CHANNELS
	{
		char szTmp[128];
		sprintf(szTmp, "Sound %s playing on Static game channel %d\n", sfxin->name, IWavstreamOfCh(ch));
		OutputDebugString(szTmp);
	}
#endif
	
	if ( TestSoundChar(pSfx->getname(), CHAR_SENTENCE) )
	{
		// this is a sentence. link words to play in sequence.

		// NOTE: sentence names stored in the cache lookup are
		// prepended with a '!'.  Sentence names stored in the
		// sentence file do not have a leading '!'. 

		// link all words and load the first word
		VOX_LoadSound(ch, PSkipSoundChars(pSfx->getname()));
		fvox = 1;
	}
	else
	{
		// load regular or stream sound

		pSource = S_LoadSound(pSfx, ch);
		ch->sfx = pSfx;
		ch->isSentence = false;
	}

	if (!ch->pMixer)
	{
		ch->sfx = NULL;
		return;
	}

	VectorCopy (origin, ch->origin);
	VectorCopy (direction, ch->direction);

	// never update positions if source entity is 0

	ch->bUpdatePositions = bUpdatePositions && (soundsource == 0 ? 0 : 1);

	ch->master_vol = vol;
	ch->dist_mult = SNDLVL_TO_DIST_MULT( soundlevel );

	
	S_SetChannelWavtype( ch, pSfx );

	ch->basePitch = pitch;
	ch->soundsource = soundsource;
	ch->entchannel = entchannel;
	ch->fromserver = fromserver;

	// set the default radius
	ch->radius = flSoundRadius;

	S_SetChannelStereo( ch, pSource );

	// initialize gain due to obscured sound source
	ch->bfirstpass = true;
	ch->ob_gain = 0.0;
	ch->ob_gain_inc = 0.0;
	ch->ob_gain_target = 0.0;
	ch->bTraced = false;

	if (snd_showstart.GetInt() == 5)
		snd_showstart.SetValue(6);		// display gain once only

	SND_Spatialize(ch);

	// Pre-startup delay.  Compute # of samples over which to mix in zeros from data source before
	//  actually reading first set of samples
	if ( delay != 0.0f )
	{
		Assert( ch->sfx );
		Assert( ch->sfx->pSource );
		
		float rate = ch->sfx->pSource->SampleRate();

		int delaySamples = (int)( delay * rate );

		ch->pMixer->SetStartupDelaySamples( delaySamples );

		if ( delay > 0 )
		{
			Assert( delay <= (float)MAX_SOUND_DELAY_MSEC / 1000.0f );
			ch->pMixer->SetStartupDelaySamples( delaySamples );
			ch->delayed_start = true;
		}
		else
		{
			int skipSamples = -delaySamples;
			int totalSamples = ch->sfx->pSource->SampleCount();
			if ( skipSamples >=  totalSamples )
			{
				S_FreeChannel( ch );
				return;
			}

			ch->pMixer->SkipSamples( 0, skipSamples, rate, 0 );
			ch->ob_gain_target	= 1.0f;
			ch->ob_gain			= 1.0f;
			ch->ob_gain_inc		= 0.0f;
			ch->bfirstpass = false;
		}

	}

	if (snd_showstart.GetInt() > 0 && snd_showstart.GetInt() < 7 && snd_showstart.GetInt() != 4)
	{
		DevMsg( "StaticSound %s : src %d : channel %d : %d dB : vol %.2f : radius %.0f : time %.3f\n", pSfx->getname(), soundsource, entchannel, soundlevel, fvol, flSoundRadius, g_pSoundServices->GetHostTime() );
		if (snd_showstart.GetInt() == 2 || snd_showstart.GetInt() == 5)
			DevMsg( "\t dspmix %1.2f : distmix %1.2f : dspface %1.2f : lvol %d : rvol %d : rlvol %d : rrvol %d\n", ch->dspmix, ch->distmix, ch->dspface, ch->leftvol, ch->rightvol, ch->rleftvol, ch->rrightvol );
		if (snd_showstart.GetInt() == 3)
			DevMsg( "\t x: %4f y: %4f z: %4f\n", ch->origin.x, ch->origin.y, ch->origin.z );
	}

}



// Restart all the sounds on the specified channel
inline bool IsChannelLooped( int iChannel )
{
	return (channels[iChannel].sfx &&
			channels[iChannel].sfx->pSource && 
			channels[iChannel].sfx->pSource->IsLooped() );
}

int S_GetCurrentStaticSounds( SoundInfo_t *pResult, int nSizeResult, int entchannel )
{
	int nSpaceRemaining = nSizeResult;
	for (int i = MAX_DYNAMIC_CHANNELS; i < total_channels && nSpaceRemaining; i++)
	{
		if ( channels[i].entchannel == entchannel && channels[i].sfx )
		{
			pResult->Set( channels[i].soundsource, 
						  channels[i].entchannel, 
						  channels[i].sfx->getname(), 
						  channels[i].origin,
						  channels[i].direction,
						  ( (float)channels[i].master_vol / 255.0 ),
						  DIST_MULT_TO_SNDLVL( channels[i].dist_mult ),
						  IsChannelLooped( i ),
						  channels[i].basePitch);
			pResult++;
			nSpaceRemaining--;
		}
	}
	return (nSizeResult - nSpaceRemaining);
}


// Stop all sounds for entity on a channel.
void S_StopSound(int soundsource, int entchannel)
{
	int i;

	for (i=0 ; i<total_channels; i++)
	{
		if (channels[i].soundsource == soundsource
			&& channels[i].entchannel == entchannel)
		{
			S_FreeChannel(&channels[i]);
		}
	}
}

void S_StopAllSounds(qboolean clear)
{
	int		i;

	if ( !g_AudioDevice )
		return;

	if ( !g_AudioDevice->IsActive() )
		return;

	total_channels = MAX_DYNAMIC_CHANNELS;	// no statics

	for (i=0 ; i<MAX_CHANNELS ; i++) 
	{
		if (channels[i].sfx)
		{
				( 1, "%2d:Stopped sound %s\n", i, channels[i].sfx->getname() );
			S_FreeChannel(&channels[i]);
		}
	}

	Q_memset(channels, 0, MAX_CHANNELS * sizeof(channel_t));

	if (clear)
	{
		S_ClearBuffer ();
	}

	// Clear any remaining soundfade
	memset( &soundfade, 0, sizeof( soundfade ) );

	g_AudioDevice->StopAllSounds();
}

void S_StopAllSoundsC (void)
{
	S_StopAllSounds (true);
}

void S_ClearBuffer (void)
{
	g_AudioDevice->ClearBuffer();
	DSP_ClearState();
	MIX_ClearAllPaintBuffers( PAINTBUFFER_SIZE, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : percent - 
//			holdtime - 
//			intime - 
//			outtime - 
//-----------------------------------------------------------------------------
void S_SoundFade( float percent, float holdtime, float intime, float outtime )
{
	soundfade.starttime				= g_pSoundServices->GetHostTime();  

	soundfade.initial_percent		= percent;       
	soundfade.fadeouttime			= outtime;    
	soundfade.holdtime				= holdtime;   
	soundfade.fadeintime			= intime;
}

//-----------------------------------------------------------------------------
// Purpose: Modulates sound volume on the client.
//-----------------------------------------------------------------------------
void S_UpdateSoundFade(void)
{
	float	totaltime;
	float	f;
	// Determine current fade value.

	// Assume no fading remains
	soundfade.percent = 0;  

	totaltime = soundfade.fadeouttime + soundfade.fadeintime + soundfade.holdtime;

	float elapsed = g_pSoundServices->GetHostTime() - soundfade.starttime;

	// Clock wrapped or reset (BUG) or we've gone far enough
	if ( elapsed < 0.0f || elapsed >= totaltime || totaltime <= 0.0f )
	{
		return;
	}

	// We are in the fade time, so determine amount of fade.
	if ( soundfade.fadeouttime > 0.0f && ( elapsed < soundfade.fadeouttime ) )
	{
		// Ramp up
		f = elapsed / soundfade.fadeouttime;
	}
	// Inside the hold time
	else if ( elapsed <= ( soundfade.fadeouttime + soundfade.holdtime ) )
	{
		// Stay
		f = 1.0f;
	}
	else
	{
		// Ramp down
		f = ( elapsed - ( soundfade.fadeouttime + soundfade.holdtime ) ) / soundfade.fadeintime;
		// backward interpolated...
		f = 1.0f - f;
	}

	// Spline it.
	f = SimpleSpline( f );
	f = clamp( f, 0.0f, 1.0f );

	soundfade.percent = soundfade.initial_percent * f;
}


//=============================================================================

ConVar snd_duckvolume( "snd_duckvolume", "0.4", FCVAR_ARCHIVE );
ConVar snd_duckattacktime( "snd_duckattacktime", "0.25", FCVAR_ARCHIVE );
ConVar snd_duckreleasetime( "snd_duckreleasetime", "3.0", FCVAR_ARCHIVE );
static void S_UpdateVoiceDuck( int voiceChannelCount, int voiceChannelMaxVolume, float frametime )
{
	float duckTarget = 1.0;
	if ( voiceChannelCount > 0 )
	{
		voiceChannelMaxVolume = clamp(voiceChannelMaxVolume, 0, 255);
		duckTarget = RemapVal( voiceChannelMaxVolume, 0, 255, 1.0, snd_duckvolume.GetFloat() );
	}
	float rate = ( duckTarget < g_DuckScale ) ? snd_duckattacktime.GetFloat() : snd_duckreleasetime.GetFloat();
	g_DuckScale = Approach( duckTarget, g_DuckScale, frametime * ((1-snd_duckvolume.GetFloat()) / rate) );
}


/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( Vector const &origin, Vector const &forward, Vector const &right, Vector const &up )
{
	int			i;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if ( !g_AudioDevice->IsActive() )
		return;

	// Update any client side sound fade
	S_UpdateSoundFade();

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_forward);
	VectorCopy(right, listener_right);
	VectorCopy(up, listener_up);

	g_AudioDevice->UpdateListener( listener_origin, listener_forward, listener_right, listener_up );
 
	combine = NULL;

	int voiceChannelCount = 0;
	int voiceChannelMaxVolume = 0;

	// reset traceline counter - limits tracelines during spatialization

	SND_ChannelTraceReset();

	// update spatialization for static and dynamic sounds	

	ch = channels;
	for (i=0; i<total_channels; i++, ch++)
	{
		if (!ch->sfx) 
			continue;

		SND_Spatialize(ch);         // respatialize channel
		if ( ch->sfx->pSource && ch->sfx->pSource->IsVoiceSource() )
		{
			voiceChannelCount++;
			voiceChannelMaxVolume = max(voiceChannelMaxVolume, ch->leftvol+ch->rightvol);
			voiceChannelMaxVolume = max(voiceChannelMaxVolume, ch->rleftvol+ch->rrightvol);
		}
	}

	// set new target for voice ducking
	S_UpdateVoiceDuck( voiceChannelCount, voiceChannelMaxVolume, g_pSoundServices->GetHostFrametime() );

//
// debugging output
//
	if (snd_show.GetInt())
	{
		con_nprint_t np;
		np.time_to_live = 2.0f;
		np.fixed_width_font = true;
		
		total = 0;
		ch = channels;
		for (i=0 ; i<total_channels; i++, ch++)
		{
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				np.index = total + 2;
				if ( ch->fromserver )
				{
					np.color[ 0 ] = 1.0;
					np.color[ 1 ] = 0.8;
					np.color[ 2 ] = 0.1;
				}
				else
				{
					np.color[ 0 ] = 0.1;
					np.color[ 1 ] = 0.9;
					np.color[ 2 ] = 1.0;
				}
				Con_NXPrintf ( &np, "%02i left(%03i) right(%03i) %50s", total+ 1, ch->leftvol, ch->rightvol, ch->sfx->getname());
				total++;
			}
		}

		while ( total <= 128 )
		{
			Con_NPrintf( total + 2, "" );
			total++;
		}
	}

// mix some sound
	S_Update_();
}


void GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	static	float	lastsoundtime;
	int				fullsamples;

	fullsamples = g_AudioDevice->DeviceSampleCount() / g_AudioDevice->DeviceChannels();

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = g_AudioDevice->GetOutputPosition();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			paintedtime = fullsamples;
			S_StopAllSounds (true);
		}
	}
	oldsamplepos = samplepos;

	// soundtime indicates how many samples have actually been
	// played out to dma

	// when in a movie, just mix one frame worth of sound
	if ( cl_moviename[0] )
	{
		float t = g_pSoundServices->GetHostTime();
		if ( lastsoundtime != t )
		{
			soundtime += g_pSoundServices->GetHostFrametime() * g_AudioDevice->DeviceDmaSpeed();
			lastsoundtime = t;
		}
	}
	else
	{
		soundtime = (buffers*fullsamples + samplepos/g_AudioDevice->DeviceChannels());
	}

}

void S_ExtraUpdate (void)
{
	g_pSoundServices->OnExtraUpdate();

	if ( snd_noextraupdate.GetInt() || cl_moviename[0] )
		return;		// don't pollute timings
	S_Update_();
}

extern void DEBUG_StartSoundMeasure(int type, int samplecount );
extern void DEBUG_StopSoundMeasure(int type, int samplecount );

void S_Update_(void)
{
	unsigned        endtime;

	if (!g_AudioDevice->IsActive())
		return;

	DEBUG_StartSoundMeasure(4, 0);

// Get soundtime, which tells how many samples have
// been played out of the dma buffer since sound system startup.

	GetSoundtime();

// paintedtime indicates how many samples we've actually mixed
// and sent to the dma buffer since sound system startup.

	if (paintedtime < soundtime)
	{
		// if soundtime > paintedtime, then the dma buffer
		// has played out more sound than we've actually
		// mixed.  We need to call S_Update_ more often.

		// DevMsg ("S_Update_ : overflow\n"); 
		// paintedtime = soundtime;		
		
		// (kdb) above code doesn't work - should actually
		// zero out the paintbuffer to advance to the new
		// time.
	}

	// mix ahead of current position

	endtime = g_AudioDevice->PaintBegin( soundtime, paintedtime );

	DEBUG_StartSoundMeasure(2, endtime - paintedtime);
	
	MIX_PaintChannels (endtime);
	
	DEBUG_StopSoundMeasure( 2, 0 );

	SNDDMA_Submit ();

	DEBUG_StopSoundMeasure( 4, 0 );
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play( const char *pszName, bool flush = false )
{
	int			inCache;
	char		szName[256];
	CSfxTable *pSfx;
	
	Q_strcpy(szName, pszName);
	if (!Q_strrchr(pszName, '.'))
		Q_strcat(szName, ".wav");

	pSfx = S_FindName(szName, &inCache);
	
	if ( inCache && flush )
		pSfx->pSource->CacheUnload();

	S_StartDynamicSound(g_pSoundServices->GetViewEntity(), CHAN_REPLACE, pSfx, listener_origin, 1.0, SNDLVL_NONE, 0, PITCH_NORM);
}

void S_Play(void)
{
	bool flush = !strcmpi( Cmd_Argv(0), "playflush" );
	int  i = 1;
	
	i = 1;
	while (i<Cmd_Argc())
	{
		S_Play(Cmd_Argv(i), flush);
		i++;
	}
}

void S_PlayVol(void)
{
	static int hash=543;
	int i;
	float vol;
	char name[256];
	CSfxTable *pSfx;
	
	i = 1;
	while (i<Cmd_Argc())
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, Cmd_Argv(i));
		pSfx = S_PrecacheSound(name);
		vol = Q_atof(Cmd_Argv(i+1));
		S_StartDynamicSound(hash++, CHAN_AUTO, pSfx, listener_origin, vol, SNDLVL_NONE, 0, PITCH_NORM);
		i+=2;
	}
}

void S_PlayDelay()
{
	if ( Cmd_Argc() != 3 )
	{
		Msg( "Usage:  playdelay delay_in_msec (negative to skip ahead) soundname\n" );
		return;
	}

	char		szName[256];
	CSfxTable *pSfx;

	float delay = Q_atof( Cmd_Argv(1) );
	
	Q_strcpy(szName, Cmd_Argv(2));
	if (!Q_strrchr(Cmd_Argv(2), '.'))
	{
		Q_strcat(szName, ".wav");
	}

	pSfx = S_FindName(szName, NULL );
	
	S_StartDynamicSound(g_pSoundServices->GetViewEntity(), CHAN_REPLACE, pSfx, listener_origin, 1.0, SNDLVL_NONE, 0, PITCH_NORM,
		false, delay );

}

static ConCommand sndplaydelay( "sndplaydelay", S_PlayDelay );

void S_SoundList(void)
{
	int		i;
	CSfxTable	*sfx;
	CAudioSource *pSource;
	int		size, total;

	total = 0;
	int c = s_Sounds.Count();
	for (i=0 ; i<c ; i++)
	{
		sfx = s_Sounds[ i ].psfx;

		pSource = sfx->pSource;
		if ( !pSource || !pSource->IsCached() )
			continue;

		size = pSource->SampleSize() * pSource->SampleCount();
		total += size;
		if ( pSource->IsLooped() )
			Msg ("L");
		else
			Msg (" ");
		Msg("(%2db) %6i : %s\n", pSource->SampleSize(),  size, sfx->getname());
	}
	Msg ("Total resident: %i\n", total);
}

#if DEAD
void S_LocalSound (char *sound)
{
	CSfxTable *pSfx;

	if (!g_AudioDevice->IsActive())
		return;
		
	pSfx = S_PrecacheSound (sound);
	if (!pSfx)
	{
		Msg ("S_LocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartDynamicSound (g_pSoundServices->GetViewEntity(), -1, pSfx, vec3_origin, 1, 1, 0, PITCH_NORM);
}
#endif

extern unsigned g_snd_time_debug;
extern unsigned g_snd_call_time_debug;
extern unsigned g_snd_count_debug;
extern unsigned g_snd_samplecount;
extern unsigned g_snd_frametime;
extern unsigned g_snd_frametime_total;
extern int g_snd_profile_type;

// start measuring sound perf, 100 reps
// type 1 - dsp, 2 - mix, 3 - load sound, 4 - all sound
// set type via ConVar snd_profile

void DEBUG_StartSoundMeasure(int type, int samplecount )
{
	if (type != g_snd_profile_type)
		return;

	if (samplecount)
		g_snd_samplecount += samplecount;

	g_snd_call_time_debug = Plat_MSTime();
}

// show sound measurement after 25 reps - show as % of total frame
// type 1 - dsp, 2 - mix, 3 - load sound, 4 - all sound

void DEBUG_StopSoundMeasure(int type, int samplecount )
{

	if (type != g_snd_profile_type)
		return;

	if (samplecount)
		g_snd_samplecount += samplecount;

	// add total time since last frame

	g_snd_frametime_total += Plat_MSTime() - g_snd_frametime;

	// performance timing

	g_snd_time_debug += Plat_MSTime() - g_snd_call_time_debug;

	if (++g_snd_count_debug >= 100)
	{
		switch (g_snd_profile_type)
		{
		case 1: 
			Msg("dsp: (%2.2f) millisec   ", ((float)g_snd_time_debug) / 100.0); 
			Msg("(%2.2f) pct of frame \n", 100.0 * ((float)g_snd_time_debug) / ((float)g_snd_frametime_total)); 
			break;
		case 2: 
			Msg("mix+dsp:(%2.2f) millisec   ", ((float)g_snd_time_debug) / 100.0);
			Msg("(%2.2f) pct of frame \n", 100.0 * ((float)g_snd_time_debug) / ((float)g_snd_frametime_total)); 
			break;
		case 3: 
			//if ( (((float)g_snd_time_debug) / 100.0) < 0.01 )
			//	break;
			Msg("snd load: (%2.2f) millisec   ", ((float)g_snd_time_debug) / 100.0); 
			Msg("(%2.2f) pct of frame \n", 100.0 * ((float)g_snd_time_debug) / ((float)g_snd_frametime_total)); 
			break;
		case 4: 
			Msg("sound: (%2.2f) millisec   ", ((float)g_snd_time_debug) / 100.0); 
			Msg("(%2.2f) pct of frame \n", 100.0 * ((float)g_snd_time_debug) / ((float)g_snd_frametime_total)); 
			break;
		}
		
		g_snd_count_debug = 0;
		g_snd_time_debug = 0;
		g_snd_samplecount = 0;	
		g_snd_frametime_total = 0;
	}

	g_snd_frametime = Plat_MSTime();
}

// speak a sentence from console; works by passing in "!sentencename"
// or "sentence"

extern ConVar dsp_room;

void S_Say (void)
{
	CSfxTable *pSfx;
	char sound[256];

	if (!g_AudioDevice->IsActive())
		return;

	if (!sound)
		return;
	
	Q_strcpy(sound, Cmd_Argv(1));		
	
	// DEBUG - test performance of dsp code
	if (!Q_strcmp(sound, "dsp"))
	{
		unsigned time;
		int i;
		int count = 10000;
		int idsp; 

		for (i = 0; i < PAINTBUFFER_SIZE; i++)
		{
			paintbuffer[i].left = g_pSoundServices->RandomLong(0,2999);
			paintbuffer[i].right = g_pSoundServices->RandomLong(0,2999);
		}

		Msg ("Start profiling 10,000 calls to DSP\n");
		
		idsp = dsp_room.GetInt();
		
		// get system time

		time = Plat_MSTime();
		
		for (i = 0; i < count; i++)
		{
			// SX_RoomFX(PAINTBUFFER_SIZE, TRUE, TRUE);

			DSP_Process(idsp, paintbuffer, NULL, PAINTBUFFER_SIZE);

		}
		// display system time delta 
		Msg("%d milliseconds \n", Plat_MSTime() - time);
		return;

	} else if (!Q_strcmp(sound, "paint"))
	{
		unsigned time;
		int count = 10000;
		static int hash=543;
		int psav = paintedtime;

		Msg ("Start profiling MIX_PaintChannels\n");
		
		pSfx = S_PrecacheSound("ambience/labdrone1.wav");
		S_StartDynamicSound(hash++, CHAN_AUTO, pSfx, listener_origin, 1.0, SNDLVL_NONE, 0, PITCH_NORM);

		// get system time
		time = Plat_MSTime();

		// paint a boatload of sound

		MIX_PaintChannels(paintedtime + 512*count);		

		// display system time delta 
		Msg("%d milliseconds \n", Plat_MSTime() - time);
		paintedtime = psav;
		return;
	}
	// DEBUG

	if (!TestSoundChar(sound, CHAR_SENTENCE))
	{
		// build a fake sentence name, then play the sentence text

		Q_strcpy(sound, "xxtestxx ");
		Q_strcat(sound, Cmd_Argv(1));

		int addIndex = g_Sentences.AddToTail();
		sentence_t *pSentence = &g_Sentences[addIndex];
		pSentence->pName = sound;
		pSentence->length = 0;

		// insert null terminator after sentence name
		sound[8] = 0;

		pSfx = S_PrecacheSound ("!xxtestxx");
		if (!pSfx)
		{
			Msg ("S_Say: can't cache %s\n", sound);
			return;
		}

		S_StartDynamicSound (g_pSoundServices->GetViewEntity(), CHAN_REPLACE, pSfx, vec3_origin, 1, SNDLVL_NORM, 0, PITCH_NORM);
		
		// remove last
		g_Sentences.Remove( g_Sentences.Size() - 1 );
	}
	else
	{
		pSfx = S_FindName(sound, NULL);
		if (!pSfx)
		{
			Msg ("S_Say: can't find sentence name %s\n", sound);
			return;
		}

		S_StartDynamicSound (g_pSoundServices->GetViewEntity(), CHAN_REPLACE, pSfx, vec3_origin, 1, SNDLVL_NORM, 0, PITCH_NORM);
	}
}
