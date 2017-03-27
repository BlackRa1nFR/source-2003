//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ==========
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "stringpool.h"
#include "igamesystem.h"
#include "gamestringpool.h"

//-----------------------------------------------------------------------------
// Purpose: The actual storage for pooled per-level strings
//-----------------------------------------------------------------------------
class CGameStringPool : public CStringPool,	public CBaseGameSystem
{
	virtual void LevelShutdownPostEntity() 
	{
		FreeAll();
	}
};

static CGameStringPool g_GameStringPool;


//-----------------------------------------------------------------------------
// String system accessor
//-----------------------------------------------------------------------------
IGameSystem *GameStringSystem()
{
	return &g_GameStringPool;
}


//-----------------------------------------------------------------------------
// Purpose: The public accessor for the level-global pooled strings
//-----------------------------------------------------------------------------
string_t AllocPooledString( const char * pszValue )
{
	if (*pszValue)
		return MAKE_STRING( g_GameStringPool.Allocate( pszValue ) );
	return NULL_STRING;
}

string_t FindPooledString( const char *pszValue )
{
	return MAKE_STRING( g_GameStringPool.Find( pszValue ) );
}
