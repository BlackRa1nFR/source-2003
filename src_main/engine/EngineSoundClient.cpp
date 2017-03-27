//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "engine/IEngineSound.h"
#include "tier0/dbg.h"
#include "sound.h"
#include "client.h"
#include "vox.h"
#include "IClientEntity.h"
#include "IClientEntityList.h"
#include "enginesingleuserfilter.h"
//-----------------------------------------------------------------------------
//
// Client-side implementation of the engine sound interface
//
//-----------------------------------------------------------------------------
class CEngineSoundClient : public IEngineSound
{
public:
	// constructor, destructor
	CEngineSoundClient();
	virtual ~CEngineSoundClient();

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

	virtual void SetRoomType( IRecipientFilter& filter, int roomType );

	virtual void EmitAmbientSound( const char *pSample, float flVolume, 
		int iPitch, int flags, float soundtime = 0.0f );

private:
	void EmitSoundInternal( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
		float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
		const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime = 0.0f );

};


//-----------------------------------------------------------------------------
// Client-server neutral sound interface accessor
//-----------------------------------------------------------------------------
static CEngineSoundClient s_EngineSoundClient;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CEngineSoundClient, IEngineSound, 
	IENGINESOUND_CLIENT_INTERFACE_VERSION, s_EngineSoundClient );

IEngineSound *EngineSoundClient()
{
	return &s_EngineSoundClient;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CEngineSoundClient::CEngineSoundClient()
{
}

CEngineSoundClient::~CEngineSoundClient()
{
}


//-----------------------------------------------------------------------------
// Precache a particular sample
//-----------------------------------------------------------------------------
bool CEngineSoundClient::PrecacheSound( const char *pSample, bool preload /*= false*/ )
{
	CSfxTable *pTable = S_PrecacheSound( pSample );
	return (pTable != NULL);
}


//-----------------------------------------------------------------------------
// Actually does the work of emitting a sound
//-----------------------------------------------------------------------------
void CEngineSoundClient::EmitSoundInternal( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch, 
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
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

	if (iEntIndex < 0)
		iEntIndex = cl.viewentity;

	// See if local player is a recipient
	int c = filter.GetRecipientCount();
	for ( int i = 0; i < c ; i++ )
	{
		int index = filter.GetRecipientIndex( i );
		if ( index == cl.playernum + 1 )
			break;
	}

	// Local player not receiving sound
	if ( i >= c )
		return;

	CSfxTable *pSound = S_PrecacheSound(pSample);
	if (!pSound)
		return;

	// Point at origin if they didn't specify a sound source.
	Vector vecDummyOrigin;
	if (!pOrigin)
	{
		// Try to use the origin of the entity
		IClientEntity *pEnt = entitylist->GetClientEntity( iEntIndex );
		if (pEnt)
		{
			vecDummyOrigin = pEnt->GetRenderOrigin();
		}
		else
		{
			vecDummyOrigin.Init();
		}

		pOrigin = &vecDummyOrigin;
	}

	Vector vecDirection;
	if (!pDirection)
	{
		IClientEntity *pEnt = entitylist->GetClientEntity( iEntIndex );
		if (pEnt)
		{
			QAngle angles = pEnt->GetAbsAngles();
			AngleVectors( angles, &vecDirection );
		}
		else
		{
			vecDirection.Init();
		}

		pDirection = &vecDirection;
	}

	float delay = 0.0f;
	if ( soundtime != 0.0f )
	{
		delay = soundtime - cl.mtime[ 0 ];
		delay = clamp( delay, 0, (float)MAX_SOUND_DELAY_MSEC / 1000.0f );
	}


	if ( iChannel == CHAN_STATIC )
	{
		S_StartStaticSound( iEntIndex, iChannel, pSound, *pOrigin, *pDirection, bUpdatePositions, 
			flVolume, iSoundLevel, iFlags, iPitch, false, delay );
	}
	else
	{
		S_StartDynamicSound( iEntIndex, iChannel, pSound, *pOrigin, *pDirection, bUpdatePositions, 
			flVolume, iSoundLevel, iFlags, iPitch, false, delay );
	}
}


//-----------------------------------------------------------------------------
// Plays a sentence
//-----------------------------------------------------------------------------
void CEngineSoundClient::EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, 
	int iSentenceIndex, float flVolume, soundlevel_t iSoundLevel, int iFlags, int iPitch,
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePosition, float soundtime /*= 0.0f*/ )
{
	if ( iSentenceIndex >= 0 )
	{
		char pName[8];
		Q_snprintf( pName, sizeof(pName), "!%d", iSentenceIndex );
		EmitSoundInternal( filter, iEntIndex, iChannel, pName, flVolume, iSoundLevel, 
			iFlags, iPitch, pOrigin, pDirection, bUpdatePosition, soundtime );
	}
}


//-----------------------------------------------------------------------------
// Emits a sound
//-----------------------------------------------------------------------------
void CEngineSoundClient::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
	float flVolume, float flAttenuation, int iFlags, int iPitch, 
	const Vector *pOrigin, const Vector *pDirection, bool bUpdatePositions, float soundtime /*= 0.0f*/ )
{
	EmitSound( filter, iEntIndex, iChannel, pSample, flVolume, ATTN_TO_SNDLVL( flAttenuation ), iFlags, 
		iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );

}


void CEngineSoundClient::EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
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
			DevWarning( 2, "Unable to find %s in sentences.txt\n", PSkipSoundChars(pSample));
		}
	}
	else
	{
		EmitSoundInternal( filter, iEntIndex, iChannel, pSample, flVolume, iSoundLevel,
			iFlags, iPitch, pOrigin, pDirection, bUpdatePositions, soundtime );
	}
}


//-----------------------------------------------------------------------------
// Stops a sound
//-----------------------------------------------------------------------------
void CEngineSoundClient::StopSound( int iEntIndex, int iChannel, const char *pSample )
{
	CEngineSingleUserFilter filter( cl.playernum + 1 );
	EmitSound( filter, iEntIndex, iChannel, pSample, 0, SNDLVL_NONE, SND_STOP, PITCH_NORM,
		NULL, NULL, true );
}


void CEngineSoundClient::SetRoomType( IRecipientFilter& filter, int roomType )
{
	extern ConVar dsp_room;
	dsp_room.SetValue( roomType );
}


void CEngineSoundClient::EmitAmbientSound( const char *pSample, float flVolume, 
										  int iPitch, int flags, float soundtime /*= 0.0f*/ )
{
	float delay = 0.0f;
	if ( soundtime != 0.0f )
	{
		delay = soundtime - cl.mtime[ 0 ];
		delay = clamp( delay, 0, (float)MAX_SOUND_DELAY_MSEC / 1000.0f );
	}

	CSfxTable *pSound = S_PrecacheSound(pSample);
	S_StartStaticSound( SOUND_FROM_LOCAL_PLAYER, CHAN_STATIC, pSound, vec3_origin, flVolume, SNDLVL_NONE, flags, iPitch, false, delay );
}
