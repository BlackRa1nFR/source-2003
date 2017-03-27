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
// Soundent.h - the entity that spawns when the world 
// spawns, and handles the world's active and free sound
// lists.
****/

#ifndef SOUNDENT_H
#define SOUNDENT_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	MAX_WORLD_SOUNDS	= 64 // maximum number of sounds handled by the world at one time.
};

enum
{
	SOUND_NONE				= 0,
	SOUND_COMBAT			= 0x00000001,
	SOUND_WORLD				= 0x00000002,
	SOUND_PLAYER			= 0x00000004,
	SOUND_DANGER			= 0x00000008,
	SOUND_BULLET_IMPACT		= 0x00000010,
	SOUND_CARCASS			= 0x00000020,
	SOUND_MEAT				= 0x00000040,
	SOUND_GARBAGE			= 0x00000080,
	SOUND_THUMPER			= 0x00000100, // keeps certain creatures at bay
	SOUND_BUGBAIT			= 0x00000200, // gets the antlion's attention
	SOUND_PHYSICS_DANGER	= 0x00000400,
	ALL_SOUNDS				= 0xFFFFFFFF,
};

enum
{
	SOUNDLIST_EMPTY = -1
};


//=========================================================
// CSound - an instance of a sound in the world.
//=========================================================
class CSound
{
	DECLARE_SIMPLE_DATADESC();

public:
	bool	DoesSoundExpire() const;
	float	SoundExpirationTime() const;
	void	SetSoundOrigin( const Vector &vecOrigin ) { m_vecOrigin = vecOrigin; }
	const	Vector& GetSoundOrigin( void ) { return m_vecOrigin; }
	const	Vector& GetSoundReactOrigin( void );
	bool	FIsSound( void );
	bool	FIsScent( void );
	bool	IsSoundType( int nSoundFlags ) const;
	int		SoundType( ) const;
	int		Volume( ) const;
	int		NextSound() const;
	void	Reset ( void );

	EHANDLE	m_hOwner;		// sound's owner
	int		m_iVolume;		// how loud the sound is
	int		m_iType;		// what type of sound this is
	int		m_iNextAudible;	// temporary link that NPCs use to build a list of audible sounds

private:
	void	Clear ( void );

	float	m_flExpireTime;	// when the sound should be purged from the list
	short	m_iNext;		// index of next sound in this list ( Active or Free )
	bool	m_bNoExpirationTime;
	int		m_ownerChannelIndex;

	Vector	m_vecOrigin;	// sound's location in space

#ifdef DEBUG
	int		m_iMyIndex;		// debugging
#endif

	friend class CSoundEnt;
};

inline bool CSound::DoesSoundExpire() const
{
	return m_bNoExpirationTime == false;
}

inline float CSound::SoundExpirationTime() const
{
	return m_bNoExpirationTime ? FLT_MAX : m_flExpireTime;
}

inline bool CSound::IsSoundType( int nSoundFlags ) const
{
	return (m_iType & nSoundFlags) != 0;
}

inline int CSound::SoundType( ) const
{
	return m_iType;
}

inline int CSound::Volume( ) const
{
	return m_iVolume;
}

inline int CSound::NextSound() const
{
	return m_iNext;
}



//=========================================================
// CSoundEnt - a single instance of this entity spawns when
// the world spawns. The SoundEnt's job is to update the 
// world's Free and Active sound lists.
//=========================================================
class CSoundEnt : public CPointEntity
{
	DECLARE_DATADESC();

public:
	DECLARE_CLASS( CSoundEnt, CPointEntity );

	// Construction, destruction
	static bool InitSoundEnt();
	static void ShutdownSoundEnt();

	CSoundEnt();
	virtual ~CSoundEnt();

	virtual void OnRestore();
	void Precache ( void );
	void Spawn( void );
	void Think( void );
	void Initialize ( void );
	int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static void		InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration );
	static void		InsertSound ( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner, int soundChannelIndex = 0 );
	static void		FreeSound ( int iSound, int iPrevious );
	static int		ActiveList( void );// return the head of the active list
	static int		FreeList( void );// return the head of the free list
	static CSound*	SoundPointerForIndex( int iIndex );// return a pointer for this index in the sound list
	static CSound*	GetLoudestSoundOfType( int iType, const Vector &vecEarPosition );
	static int		ClientSoundIndex ( edict_t *pClient );

	bool	IsEmpty( void );
	int		ISoundsInList ( int iListType );
	int		IAllocSound ( void );
	int		FindOrAllocateSound( CBaseEntity *pOwner, int soundChannelIndex );
	
private:
	int		m_iFreeSound;	// index of the first sound in the free sound list
	int		m_iActiveSound; // indes of the first sound in the active sound list
	int		m_cLastActiveSounds; // keeps track of the number of active sounds at the last update. (for diagnostic work)
	bool	m_fShowReport; // if true, dump information about free/active sounds.
	CSound	m_SoundPool[ MAX_WORLD_SOUNDS ];
};


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline bool CSoundEnt::IsEmpty( void ) 
{ 
	return m_iActiveSound == SOUNDLIST_EMPTY; 
}


#endif //SOUNDENT_H
