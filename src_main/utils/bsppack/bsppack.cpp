//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "ibsppack.h"
#include "bsplib.h"
#include "cmdlib.h"

class CBSPPack : public IBSPPack
{
public:
	void LoadBSPFile( char *filename );
	void WriteBSPFile( char *filename );
	void ClearPackFile( void );
	void AddFileToPack( const char *relativename, const char *fullpath );
	void AddBufferToPack( const char *relativename, void *data, int length, bool bTextMode );
};

void CBSPPack::LoadBSPFile( char *filename )
{
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	CmdLib_InitFileSystem( filename );
	::LoadBSPFile( filename );
}

void CBSPPack::WriteBSPFile( char *filename )
{
	::WriteBSPFile( filename );
}

void CBSPPack::ClearPackFile( void )
{
	::ClearPackFile();
}

void CBSPPack::AddFileToPack( const char *relativename, const char *fullpath )
{
	::AddFileToPack( relativename, fullpath );
}

void CBSPPack::AddBufferToPack( const char *relativename, void *data, int length, bool bTextMode )
{
	::AddBufferToPack( relativename, data, length, bTextMode );
}

EXPOSE_SINGLE_INTERFACE( CBSPPack, IBSPPack, IBSPPACK_VERSION_STRING );

