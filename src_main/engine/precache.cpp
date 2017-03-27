//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "quakedef.h"
#include "precache.h"

//-----------------------------------------------------------------------------
// Purpose: Print out flag names
// Input  : flags - 
// Output : char const
//-----------------------------------------------------------------------------
char const *GetFlagString( int flags )
{
	static char ret[ 512 ];
	ret[ 0 ] = 0;

	bool first = true;

	if ( !flags )
		return "None";

	if ( flags & RES_FATALIFMISSING )
	{
		if ( !first )
		{
			strcat( ret, " | " );
		}
		strcat( ret, "RES_FATALIFMISSING" );
		first = false;
	}

	if ( flags & RES_PRELOAD )
	{
		if ( !first )
		{
			strcat( ret, " | " );
		}
		strcat( ret, "RES_PRELOAD" );
		first = false;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPrecacheItem::CPrecacheItem( void )
{
	Init( TYPE_UNK, NULL );
	ResetStats();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPrecacheItem::ResetStats( void )
{
	m_uiRefcount = 0;
	m_flFirst = 0.0f;
	m_flMostRecent = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPrecacheItem::Reference( void )
{
	m_uiRefcount++;
	m_flMostRecent = realtime;
	if ( !m_flFirst )
	{
		m_flFirst = realtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//			*ptr - 
//-----------------------------------------------------------------------------
void CPrecacheItem::Init( int type, void const *ptr )
{
	m_nType = type;
	u.model = ( model_t * )ptr;
	if ( ptr )
	{
		ResetStats();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : model_t
//-----------------------------------------------------------------------------
model_t *CPrecacheItem::GetModel( void )
{
	if ( !u.model )
		return NULL;

	Assert( m_nType == TYPE_MODEL );

	Reference();

	return u.model;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CPrecacheItem::GetGeneric( void )
{
	if ( !u.generic )
		return NULL;

	Assert( m_nType == TYPE_GENERIC );

	Reference();

	return u.generic;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CSfxTable
//-----------------------------------------------------------------------------
CSfxTable *CPrecacheItem::GetSound( void )
{
	if ( !u.sound )
		return NULL;

	Assert( m_nType == TYPE_SOUND );

	Reference();

	return u.sound;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CPrecacheItem::GetName( void )
{
	if ( !u.name )
		return NULL;

	Assert( m_nType == TYPE_SOUND );

	Reference();

	return u.name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
char const *CPrecacheItem::GetDecal( void )
{
	if ( !u.name )
		return NULL;

	Assert( m_nType == TYPE_DECAL );

	Reference();

	return u.name;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pmodel - 
//-----------------------------------------------------------------------------
void CPrecacheItem::SetModel( model_t const *pmodel )
{
	Init( TYPE_MODEL, pmodel );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pname - 
//-----------------------------------------------------------------------------
void CPrecacheItem::SetGeneric( char const *pname )
{
	Init( TYPE_GENERIC, pname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *psound - 
//-----------------------------------------------------------------------------
void CPrecacheItem::SetSound( CSfxTable const *psound )
{
	Init( TYPE_SOUND, psound );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CPrecacheItem::SetName( char const *name )
{
	Init( TYPE_SOUND, name );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *decalname - 
//-----------------------------------------------------------------------------
void CPrecacheItem::SetDecal( char const *decalname )
{
	Init( TYPE_DECAL, decalname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CPrecacheItem::GetFirstReference( void )
{
	return m_flFirst;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CPrecacheItem::GetMostRecentReference( void )
{
	return m_flMostRecent;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : unsigned int
//-----------------------------------------------------------------------------
unsigned int CPrecacheItem::GetReferenceCount( void )
{
	return m_uiRefcount;
}