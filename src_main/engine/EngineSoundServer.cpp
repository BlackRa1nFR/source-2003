//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "engine/IEngineSound.h"
#include "tier0/dbg.h"
#include "vox.h"
#include "server.h"
#include "edict.h"
#include "sound.h"
#include "vengineserver_impl.h"
#include "enginesingleuserfilter.h"


//-----------------------------------------------------------------------------
//
// Server-side implementation of the engine sound interface
//
//-----------------------------------------------------------------------------
class CEngineSoundServer : public IEngineSound
{
public:
	// constructor, destructor
	CEngineSoundServer();
	virtual ~CEngineSoundServer();

	virtual bool PrecacheSound( const char *pSample, bool preload = false );

	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, float flAttenuation, int iFlags, int iPitch, 
		const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime = 0.0f );

	virtual void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
		const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime = 0.0f );

	virtual void EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
		float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch,
		const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime = 0.0f );

	virtual void StopSound( int iEntIndex, int iChannel, const char *pSample );

	// Set the room type for a player
	virtual void SetRoomType( IRecipientFilter& filter, int roomType );
	
	// emit an "ambient" sound that isn't spatialized - specify left/right volume
	// only available on the client, assert on server
	virtual void EmitAmbientSound( const char *pSample, float flVolume, int iPitch, int flags, float soundtime = 0.0f );

private:
	void EmitSoundInternal( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
		const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime = 0.0f );
};


//-----------------------------------------------------------------------------
// Client-server neutral sound interface accessor
//-----------------------------------------------------------------------------
static CEngineSoundServer s_EngineSoundServer;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CEngineSoundServer, IEngineSound, 
	IENGINESOUND_SERVER_INTERFACE_VERSION, s_EngineSoundServer );

IEngineSound *EngineSoundServer()
{
	return &s_EngineSoundServer;
}



//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CEngineSoundServer::CEngineSoundServer()
{
}

CEngineSoundServer::~CEngineSoundServer()
{
}


//-----------------------------------------------------------------------------
// Precache a particular sample
//-----------------------------------------------------------------------------
bool CEngineSoundServer::PrecacheSound( const char *pSample, bool preload /*=false*/ )
{
	int		i;

	if ( pSample && TestSoundChar(pSample, CHAR_SENTENCE) )
	{
		return true;
	}

	if ( pSample[0] <= ' ' )
	{
		Host_Error( "CEngineSoundServer::PrecacheSound:  Bad string: %s", pSample );
	}
	
	// add the sound to the precache list
	// Start at 1, since 0 is used to indicate an error in the sound precache
	i = SV_FindOrAddSound( pSample, preload );
	if (i >= 0)
		return true;

	Host_Error ("CEngineSoundServer::PrecacheSound: '%s' overflow", pSample);
	return false;
}


//-----------------------------------------------------------------------------
// Stops a sound
//-----------------------------------------------------------------------------
void CEngineSoundServer::EmitSoundInternal( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
	AssertMsg( pDirection == NULL, "Direction specification not currently supported on server sounds" );
	AssertMsg( bUpdatePositions, "Non-updated positions not currently supported on server sounds" );

	if (flVolume < 0 || flVolume > 1)
	{
		Warning ("EmitSound: volume out of bounds = %f", flVolume);
		return;
	}

	if (iSoundLevel < 0 || iSoundLevel > 255)
	{
		Warning ("EmitSound: soundlevel out of bounds = %d", iSoundLevel);
		return;
	}

	if (iPitch < 0 || iPitch > 255)
	{
		Warning ("EmitSound: pitch out of bounds = %i", iPitch);
		return;
	}

	edict_t *pEdict = (iEntIndex >= 0) ? &sv.edicts[iEntIndex] : NULL; 
	SV_StartSound( filter, pEdict, iChannel, pSample, flVolume, iSoundLevel, 
		iFlags, iPitch, pOrigin, soundtime );
}


//-----------------------------------------------------------------------------
// Plays a sentence
//-----------------------------------------------------------------------------
void CEngineSoundServer::EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, 
	int iSentenceIndex, float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch,
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
	if ( iSentenceIndex >= 0 )
	{
		char pName[8];
		Q_snprintf( pName, sizeof(pName), "!%d", iSentenceIndex );
		EmitSoundInternal( filter, iEntIndex, iChannel, pName, flVolume, iSoundLevel, 
			iFlags, iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );
	}
}


//-----------------------------------------------------------------------------
// Emits a sound
//-----------------------------------------------------------------------------
void CEngineSoundServer::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, float flAttenuation, int iFlags, int iPitch, 
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
	EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, ATTN_TO_SNDLVL( flAttenuation ), iFlags, 
		iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );
}
	
	
void CEngineSoundServer::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
	if ( pSample && TestSoundChar(pSample, CHAR_SENTENCE) )
	{
		int iSentenceIndex = -1;
		VOX_LookupString( PSkipSoundChars(pSample), &iSentenceIndex );
		if (iSentenceIndex >= 0)
		{
			EmitSentenceByIndex( filter, iEntIndex, iChannel, iSentenceIndex, flVolume,
				iSoundLevel, iFlags, iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );
		}
		else
		{
			DevWarning( 2, "Unable to find %s in sentences.txt\n", PSkipSoundChars(pSample) );
		}
	}
	else
	{
		EmitSoundInternal( filter, iEntIndex, iChannel, pSample, flVolume, iSoundLevel,
			iFlags, iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );
	}
}

void CEngineSoundServer::SetRoomType( IRecipientFilter& filter, int roomType )
{
	bf_write *buf = g_pVEngineServer->MessageBegin( filter, svc_roomtype );
		buf->WriteShort( (short)roomType );
	g_pVEngineServer->MessageEnd();
}


void CEngineSoundServer::EmitAmbientSound( const char *pSample, float flVolume, int iPitch, int flags, float soundtime /*= 0.0f*/ )
{
	AssertMsg( 0, "Not supported" );
}

//-----------------------------------------------------------------------------
// Stops a sound
//-----------------------------------------------------------------------------
void CEngineSoundServer::StopSound( int iEntIndex, int iChannel, const char *pSample )
{
	CEngineRecipientFilter filter;
	filter.AddAllPlayers();
	filter.MakeReliable();

	EmitSound( filter, iEntIndex, iChannel, pSample, 0, SNDLVL_NONE, SND_STOP, PITCH_NORM,
		NULL, NULL, true );
}



//-----------------------------------------------------------------------------
// FIXME: Move into the CEngineSoundServer class?
//-----------------------------------------------------------------------------
void Host_RestartAmbientSounds()
{
	if (!sv.active)
	{
		return;
	}
	
	
#ifndef SWDS
	const int 	NUM_INFOS = 64;
	SoundInfo_t soundInfo[NUM_INFOS];

	int nSounds = S_GetCurrentStaticSounds( soundInfo, NUM_INFOS, CHAN_STATIC );
	
	for ( int i = 0; i < nSounds; i++)
	{
		if (soundInfo[i].looping && 
			soundInfo[i].entity != -1 )
		{
			Msg("Restarting sound %s...\n", soundInfo[i].name);
			S_StopSound(soundInfo[i].entity, soundInfo[i].channel);
			CEngineRecipientFilter filter;
			filter.AddAllPlayers();

			SV_StartSound( filter, EDICT_NUM(soundInfo[i].entity), 
						   CHAN_STATIC, 
						   soundInfo[i].name, 
						   soundInfo[i].volume,
						   soundInfo[i].soundlevel,
						   0,                    // @Q (toml 05-09-02): Is this correct, or will I need to squirrel away the original flags?
						   soundInfo[i].pitch );
		}
	}
#endif
}



