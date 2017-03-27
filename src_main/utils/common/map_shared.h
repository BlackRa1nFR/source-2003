//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef MAP_SHARED_H
#define MAP_SHARED_H
#ifdef _WIN32
#pragma once
#endif


#include "ChunkFile.h"
#include "bsplib.h"
#include "cmdlib.h"


struct LoadEntity_t
{
	entity_t *pEntity;
	int nID;
	int nBaseFlags;
	int nBaseContents;
};


class CMapError
{
public:

	void BrushState( int entityIndex, int brushIndex )
	{
		m_entityIndex = entityIndex;
		m_brushIndex = brushIndex;
	}

	void BrushSide( int side )
	{
		m_sideIndex = side;
	}

	void TextureState( const char *pTextureName )
	{
		strncpy( m_textureName, pTextureName, 80 );
	}

	void ClearState( void )
	{
		BrushState( -1, 0 );
		BrushSide( 0 );
		TextureState( "Not a Parse error!" );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook the map parse errors and report brush/ent/texture state
	// Input  : *pErrorString - 
	//-----------------------------------------------------------------------------
	void ReportError( const char *pErrorString )
	{
		Error( "%s\nEntity %i, Brush %i, Side %i\nTexture: %s\n", pErrorString, m_entityIndex, m_brushIndex, m_sideIndex, m_textureName );
	}

	//-----------------------------------------------------------------------------
	// Purpose: Hook the map parse errors and report brush/ent/texture state without exiting.
	// Input  : pWarningString - 
	//-----------------------------------------------------------------------------
	void ReportWarning( const char *pWarningString )
	{
		printf( "Entity %i, Brush %i, Side %i: %s\n", m_entityIndex, m_brushIndex, m_sideIndex, pWarningString );
	}

private:

	int		m_entityIndex;
	int		m_brushIndex;
	int		m_sideIndex;
	char	m_textureName[80];
};



extern CMapError g_MapError;
extern int g_nMapFileVersion;


// Shared mapload code.
ChunkFileResult_t LoadEntityKeyCallback( const char *szKey, const char *szValue, LoadEntity_t *pLoadEntity );

// Used by VRAD incremental lighting - only load ents from the file and
// fill in the global entities/num_entities array.
bool LoadEntsFromMapFile( char const *pFilename );


#endif // MAP_SHARED_H
