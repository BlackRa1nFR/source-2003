//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef PRECACHE_H
#define PRECACHE_H
#ifdef _WIN32
#pragma once
#endif

#include "networkstringtabledefs.h"

#pragma pack(2)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct CPrecacheUserData
{
	unsigned short flags		: 2;
	unsigned short filesize		: 14;  // (filesize>>10)
};

#pragma pack()

class CSfxTable;
struct model_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPrecacheItem
{
public:
					CPrecacheItem( void );

	// accessorts
	model_t			*GetModel( void );
	char const		*GetGeneric( void );
	CSfxTable		*GetSound( void );
	char const		*GetName( void );
	char const		*GetDecal( void );

	void			SetModel( model_t const *pmodel );
	void			SetGeneric( char const *pname );
	void			SetSound( CSfxTable const *psound );
	void			SetName( char const *pname );
	void			SetDecal( char const *decalname );

	float			GetFirstReference( void );
	float			GetMostRecentReference( void );
	unsigned int	GetReferenceCount( void );

private:
	enum
	{
		TYPE_UNK = 0,
		TYPE_MODEL,
		TYPE_SOUND,
		TYPE_GENERIC,
		TYPE_DECAL
	};

	void			Init( int type, void const *ptr );
	void			ResetStats( void );
	void			Reference( void );

	int				m_nType;
	union precacheptr
	{
		model_t		*model;
		char const	*generic;
		CSfxTable	*sound;
		char const	*name;
	} u;

	unsigned int	m_uiRefcount;
	float			m_flFirst;
	float			m_flMostRecent;
};

#define PRECACHE_DATASIZE ( sizeof( CPrecacheUserData ) )

#define MODEL_PRECACHE_TABLENAME	"modelprecache"
#define GENERIC_PRECACHE_TABLENAME	"genericprecache"
#define SOUND_PRECACHE_TABLENAME	"soundprecache"
#define DECAL_PRECACHE_TABLENAME	"decalprecache"

char const *GetFlagString( int flags );

#endif // PRECACHE_H
