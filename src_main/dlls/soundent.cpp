/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "cbase.h"
#include "soundent.h"
#include "game.h"
#include "world.h"


//-----------------------------------------------------------------------------
// Some enumerations needed by CSoundEnt
//-----------------------------------------------------------------------------

// identifiers passed to functions that can operate on either list, to indicate which list to operate on.
#define SOUNDLISTTYPE_FREE		1
#define SOUNDLISTTYPE_ACTIVE	2



LINK_ENTITY_TO_CLASS( soundent, CSoundEnt );

static CSoundEnt *g_pSoundEnt = NULL;

BEGIN_SIMPLE_DATADESC( CSound )

	DEFINE_FIELD( CSound, m_hOwner,				FIELD_EHANDLE ),
	DEFINE_FIELD( CSound, m_iVolume,			FIELD_INTEGER ),
	DEFINE_FIELD( CSound, m_iType,				FIELD_INTEGER ),
//	DEFINE_FIELD( CSound, m_iNextAudible,		FIELD_INTEGER ),
	DEFINE_FIELD( CSound, m_bNoExpirationTime,	FIELD_BOOLEAN ),
	DEFINE_FIELD( CSound, m_flExpireTime,		FIELD_TIME ),
	DEFINE_FIELD( CSound, m_iNext,				FIELD_SHORT ),
	DEFINE_FIELD( CSound, m_ownerChannelIndex,	FIELD_INTEGER ),
	DEFINE_FIELD( CSound, m_vecOrigin,			FIELD_POSITION_VECTOR ),
//	DEFINE_FIELD( CSound, m_iMyIndex,			FIELD_INTEGER ),

END_DATADESC()


//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound :: Clear ( void )
{
	m_vecOrigin		= vec3_origin;
	m_iType			= 0;
	m_iVolume		= 0;
	m_flExpireTime	= 0;
	m_bNoExpirationTime = false;
	m_iNext			= SOUNDLIST_EMPTY;
	m_iNextAudible	= 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound,
// but doesn't expire or unlink it. 
//=========================================================
void CSound :: Reset ( void )
{
	m_vecOrigin		= vec3_origin;
	m_iType			= 0;
	m_iVolume		= 0;
	m_iNext			= SOUNDLIST_EMPTY;
}

//=========================================================
// FIsSound - returns true if the sound is an Audible sound
//=========================================================
bool CSound :: FIsSound ( void )
{
	switch( m_iType )
	{
	case SOUND_COMBAT:
	case SOUND_WORLD:
	case SOUND_PLAYER:
	case SOUND_DANGER:
	case SOUND_THUMPER:
	case SOUND_BULLET_IMPACT:
	case SOUND_BUGBAIT:
	case SOUND_PHYSICS_DANGER:
		return true;
		break;

	default:
		return false;
		break;
	}
}

//=========================================================
// FIsScent - returns true if the sound is actually a scent
// do we really need this function? If a sound isn't a sound,
// it must be a scent. (sjb)
//=========================================================
bool CSound :: FIsScent ( void )
{
	switch( m_iType )
	{
	case SOUND_CARCASS:
	case SOUND_MEAT:
	case SOUND_GARBAGE:
		return true;
		break;

	default:
		return false;
		break;
	}
}


//---------------------------------------------------------
// This function returns the spot the listener should be
// interested in if he hears the sound. MOST of the time,
// this spot is the same as the sound's origin. But sometimes
// (like with bullet impacts) the entity that owns the 
// sound is more interesting than the actual location of the
// sound effect.
//---------------------------------------------------------
const Vector &CSound::GetSoundReactOrigin( void )
{
	switch( m_iType )
	{
	case SOUND_BULLET_IMPACT:
	case SOUND_PHYSICS_DANGER:
		if( m_hOwner )
		{
			// We really want the origin of this sound's 
			// owner.
			return m_hOwner->GetAbsOrigin();
		}
		else
		{
			// If the owner is somehow invalid, we'll settle
			// for the sound's origin rather than a crash.
			return GetSoundOrigin();
		}
		break;

	default:
		return GetSoundOrigin();
		break;
	}
}



//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CSoundEnt )

	DEFINE_FIELD( CSoundEnt, m_iFreeSound,			FIELD_INTEGER ),
	DEFINE_FIELD( CSoundEnt, m_iActiveSound,		FIELD_INTEGER ),
	DEFINE_FIELD( CSoundEnt, m_cLastActiveSounds,	FIELD_INTEGER ),
	DEFINE_FIELD( CSoundEnt, m_fShowReport,			FIELD_BOOLEAN ),
	DEFINE_EMBEDDED_ARRAY( CSoundEnt, m_SoundPool, MAX_WORLD_SOUNDS ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Class factory methods
//-----------------------------------------------------------------------------
bool CSoundEnt::InitSoundEnt()
{
	///!!!LATER - do we want a sound ent in deathmatch? (sjb)
	g_pSoundEnt = (CSoundEnt*)CBaseEntity::Create( "soundent", vec3_origin, vec3_angle, GetWorldEntity() );
	if ( !g_pSoundEnt )
	{
		Warning( "**COULD NOT CREATE SOUNDENT**\n" );
		return false;
	}
	return true;
}

void CSoundEnt::ShutdownSoundEnt()
{
	if ( g_pSoundEnt )
	{
		g_pSoundEnt->FreeList();
		g_pSoundEnt = NULL;
	}
}


//-----------------------------------------------------------------------------
// Construction, destruction
//-----------------------------------------------------------------------------
CSoundEnt::CSoundEnt()
{
}

CSoundEnt::~CSoundEnt()
{
}


//=========================================================
// Spawn 
//=========================================================
void CSoundEnt :: Spawn( void )
{
	SetSolid( SOLID_NONE );
	Relink();
	Initialize();

	SetNextThink( gpGlobals->curtime + 1 );
}

void CSoundEnt :: OnRestore()
{
	BaseClass::OnRestore();

	// Make sure the singleton points to the restored version of this.
	if ( g_pSoundEnt )
	{
		Assert( g_pSoundEnt != this );
		UTIL_Remove( g_pSoundEnt );
	}
	g_pSoundEnt = this;
}


//=========================================================
// Think - at interval, the entire active sound list is checked
// for sounds that have ExpireTimes less than or equal
// to the current world time, and these sounds are deallocated.
//=========================================================
void CSoundEnt :: Think ( void )
{
	int iSound;
	int iPreviousSound;

	SetNextThink( gpGlobals->curtime + 0.3 );// how often to check the sound list.

	iPreviousSound = SOUNDLIST_EMPTY;
	iSound = m_iActiveSound; 

	while ( iSound != SOUNDLIST_EMPTY )
	{
		if ( m_SoundPool[ iSound ].m_flExpireTime <= gpGlobals->curtime && (!m_SoundPool[ iSound ].m_bNoExpirationTime) )
		{
			int iNext = m_SoundPool[ iSound ].m_iNext;

			// move this sound back into the free list
			FreeSound( iSound, iPreviousSound );

			iSound = iNext;
		}
		else
		{
			iPreviousSound = iSound;
			iSound = m_SoundPool[ iSound ].m_iNext;
		}
	}

	if ( m_fShowReport )
	{
		DevMsg( 2, "Soundlist: %d / %d  (%d)\n", ISoundsInList( SOUNDLISTTYPE_ACTIVE ),ISoundsInList( SOUNDLISTTYPE_FREE ), ISoundsInList( SOUNDLISTTYPE_ACTIVE ) - m_cLastActiveSounds );
		m_cLastActiveSounds = ISoundsInList ( SOUNDLISTTYPE_ACTIVE );
	}

}

//=========================================================
// Precache - dummy function
//=========================================================
void CSoundEnt :: Precache ( void )
{
}

//=========================================================
// FreeSound - clears the passed active sound and moves it 
// to the top of the free list. TAKE CARE to only call this
// function for sounds in the Active list!!
//=========================================================
void CSoundEnt :: FreeSound ( int iSound, int iPrevious )
{
	if ( !g_pSoundEnt )
	{
		// no sound ent!
		return;
	}

	if ( iPrevious != SOUNDLIST_EMPTY )
	{
		// iSound is not the head of the active list, so
		// must fix the index for the Previous sound
		g_pSoundEnt->m_SoundPool[ iPrevious ].m_iNext = g_pSoundEnt->m_SoundPool[ iSound ].m_iNext;
	}
	else 
	{
		// the sound we're freeing IS the head of the active list.
		g_pSoundEnt->m_iActiveSound = g_pSoundEnt->m_SoundPool [ iSound ].m_iNext;
	}

	// make iSound the head of the Free list.
	g_pSoundEnt->m_SoundPool[ iSound ].m_iNext = g_pSoundEnt->m_iFreeSound;
	g_pSoundEnt->m_iFreeSound = iSound;
}

//=========================================================
// IAllocSound - moves a sound from the Free list to the 
// Active list returns the index of the alloc'd sound
//=========================================================
int CSoundEnt :: IAllocSound( void )
{
	int iNewSound;

	if ( m_iFreeSound == SOUNDLIST_EMPTY )
	{
		// no free sound!
		Msg( "Free Sound List is full!\n" );
		return SOUNDLIST_EMPTY;
	}

	// there is at least one sound available, so move it to the
	// Active sound list, and return its SoundPool index.
	
	iNewSound = m_iFreeSound;// copy the index of the next free sound

	m_iFreeSound = m_SoundPool[ m_iFreeSound ].m_iNext;// move the index down into the free list. 

	m_SoundPool[ iNewSound ].m_iNext = m_iActiveSound;// point the new sound at the top of the active list.

	m_iActiveSound = iNewSound;// now make the new sound the top of the active list. You're done.

#ifdef DEBUG
	m_SoundPool[ iNewSound ].m_iMyIndex = iNewSound;
#endif // DEBUG

	return iNewSound;
}

//=========================================================
// InsertSound - Allocates a free sound and fills it with 
// sound info.
//=========================================================
void CSoundEnt :: InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration )
{
	int	iThisSound;

	if ( !g_pSoundEnt )
	{
		// no sound ent!
		return;
	}

	iThisSound = g_pSoundEnt->IAllocSound();

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		Msg( "Could not AllocSound() for InsertSound() (DLL)\n" );
		return;
	}

	CSound *pSound;

	pSound = &g_pSoundEnt->m_SoundPool[ iThisSound ];

	pSound->SetSoundOrigin( vecOrigin );
	pSound->m_iType = iType;
	pSound->m_iVolume = iVolume;
	pSound->m_flExpireTime = gpGlobals->curtime + flDuration;
	pSound->m_bNoExpirationTime = false;
	pSound->m_hOwner = NULL;
}

int CSoundEnt::FindOrAllocateSound( CBaseEntity *pOwner, int soundChannelIndex )
{
	int iSound = m_iActiveSound; 

	while ( iSound != SOUNDLIST_EMPTY )
	{
		CSound &sound = m_SoundPool[iSound];
		
		if ( sound.m_ownerChannelIndex == soundChannelIndex && sound.m_hOwner == pOwner )
		{
			return iSound;
		}

		iSound = sound.m_iNext;
	}

	return IAllocSound();
}

//=========================================================
//=========================================================
void CSoundEnt :: InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner, int soundChannelIndex )
{
	int	iThisSound;

	if ( !g_pSoundEnt )
	{
		// no sound ent!
		return;
	}

	iThisSound = g_pSoundEnt->FindOrAllocateSound( pOwner, soundChannelIndex );

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		Msg( "Could not AllocSound() for InsertSound() (DLL)\n" );
		return;
	}

	CSound *pSound;

	pSound = &g_pSoundEnt->m_SoundPool[ iThisSound ];

	pSound->SetSoundOrigin( vecOrigin );
	pSound->m_iType = iType;
	pSound->m_iVolume = iVolume;
	pSound->m_flExpireTime = gpGlobals->curtime + flDuration;
	pSound->m_bNoExpirationTime = false;
	pSound->m_hOwner.Set( pOwner );
	pSound->m_ownerChannelIndex = soundChannelIndex;
}


//=========================================================
// Initialize - clears all sounds and moves them into the 
// free sound list.
//=========================================================
void CSoundEnt :: Initialize ( void )
{
  	int i;
	int iSound;

	m_cLastActiveSounds;
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;

	for ( i = 0 ; i < MAX_WORLD_SOUNDS ; i++ )
	{// clear all sounds, and link them into the free sound list.
		m_SoundPool[ i ].Clear();
		m_SoundPool[ i ].m_iNext = i + 1;
	}

	m_SoundPool[ i - 1 ].m_iNext = SOUNDLIST_EMPTY;// terminate the list here.

	
	// now reserve enough sounds for each client
	for ( i = 0 ; i < gpGlobals->maxClients ; i++ )
	{
		iSound = IAllocSound();

		if ( iSound == SOUNDLIST_EMPTY )
		{
			Msg( "Could not AllocSound() for Client Reserve! (DLL)\n" );
			return;
		}

		m_SoundPool[ iSound ].m_bNoExpirationTime = true;
	}

	if ( displaysoundlist.GetInt() == 1 )
	{
		m_fShowReport = true;
	}
	else
	{
		m_fShowReport = false;
	}
}

//=========================================================
// ISoundsInList - returns the number of sounds in the desired
// sound list.
//=========================================================
int CSoundEnt :: ISoundsInList ( int iListType )
{
	int i;
	int iThisSound = SOUNDLIST_EMPTY;

	if ( iListType == SOUNDLISTTYPE_FREE )
	{
		iThisSound = m_iFreeSound;
	}
	else if ( iListType == SOUNDLISTTYPE_ACTIVE )
	{
		iThisSound = m_iActiveSound;
	}
	else
	{
		Msg( "Unknown Sound List Type!\n" );
	}

	if ( iThisSound == SOUNDLIST_EMPTY )
	{
		return 0;
	}

	i = 0;

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		i++;

		iThisSound = m_SoundPool[ iThisSound ].m_iNext;
	}

	return i;
}

//=========================================================
// ActiveList - returns the head of the active sound list
//=========================================================
int CSoundEnt :: ActiveList ( void )
{
	if ( !g_pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return g_pSoundEnt->m_iActiveSound;
}

//=========================================================
// FreeList - returns the head of the free sound list
//=========================================================
int CSoundEnt :: FreeList ( void )
{
	if ( !g_pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return g_pSoundEnt->m_iFreeSound;
}

//=========================================================
// SoundPointerForIndex - returns a pointer to the instance
// of CSound at index's position in the sound pool.
//=========================================================
CSound*	CSoundEnt :: SoundPointerForIndex( int iIndex )
{
	if ( !g_pSoundEnt )
	{
		return NULL;
	}

	if ( iIndex > ( MAX_WORLD_SOUNDS - 1 ) )
	{
		Msg( "SoundPointerForIndex() - Index too large!\n" );
		return NULL;
	}

	if ( iIndex < 0 )
	{
		Msg( "SoundPointerForIndex() - Index < 0!\n" );
		return NULL;
	}

	return &g_pSoundEnt->m_SoundPool[ iIndex ];
}

//=========================================================
// Clients are numbered from 1 to MAXCLIENTS, but the client
// reserved sounds in the soundlist are from 0 to MAXCLIENTS - 1,
// so this function ensures that a client gets the proper index
// to his reserved sound in the soundlist.
//=========================================================
int CSoundEnt :: ClientSoundIndex ( edict_t *pClient )
{
	int iReturn = ENTINDEX( pClient ) - 1;

#ifdef _DEBUG
	if ( iReturn < 0 || iReturn > gpGlobals->maxClients )
	{
		Msg( "** ClientSoundIndex returning a bogus value! **\n" );
	}
#endif // _DEBUG

	return iReturn;
}

//-----------------------------------------------------------------------------
// Purpose: Return the loudest sound of the specified type at "earposition"
//-----------------------------------------------------------------------------
CSound*	CSoundEnt::GetLoudestSoundOfType( int iType, const Vector &vecEarPosition )
{
	CSound *pLoudestSound = NULL;

	int iThisSound; 
	int	iBestSound = SOUNDLIST_EMPTY;
	float flBestDist = MAX_COORD_RANGE*MAX_COORD_RANGE;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound;

	iThisSound = ActiveList();

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = SoundPointerForIndex( iThisSound );

		if ( pSound && pSound->m_iType == iType )
		{
			flDist = ( pSound->GetSoundOrigin() - vecEarPosition ).Length();

			//FIXME: This doesn't match what's in Listen()
			//flDist = UTIL_DistApprox( pSound->GetSoundOrigin(), vecEarPosition );

			if ( flDist <= pSound->m_iVolume && flDist < flBestDist )
			{
				pLoudestSound = pSound;

				iBestSound = iThisSound;
				flBestDist = flDist;
			}
		}

		iThisSound = pSound->m_iNext;
	}

	return pLoudestSound;
}


//-----------------------------------------------------------------------------
// Purpose: Inserts an AI sound into the world sound list.
//-----------------------------------------------------------------------------
class CAISound : public CPointEntity
{
public:
	DECLARE_CLASS( CAISound, CPointEntity );

	DECLARE_DATADESC();

	// data
	int m_iSoundType;

	// Input handlers
	void InputInsertSound( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( ai_sound, CAISound );

BEGIN_DATADESC( CAISound )

	DEFINE_KEYFIELD( CAISound, m_iSoundType, FIELD_INTEGER, "soundtype" ),

	DEFINE_INPUTFUNC( CAISound, FIELD_INTEGER, "InsertSound", InputInsertSound ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAISound::InputInsertSound( inputdata_t &inputdata )
{
	int iVolume;

	iVolume = inputdata.value.Int();

	g_pSoundEnt->InsertSound( m_iSoundType, GetAbsOrigin(), iVolume, 0.3, this );
}


