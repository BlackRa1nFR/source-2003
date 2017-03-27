//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <string.h>
#include "net_synctags.h"
#include "bitbuf.h"
#include "convar.h"


ConVar net_synctags( "net_synctags", "0", 0, "Insert tokens into the net stream to find client/server mismatches." );


void SyncTag_Write( bf_write *pBuf, const char *pTag )
{
	if ( net_synctags.GetInt() )
	{
		pBuf->WriteString( pTag );
	}
}


void SyncTag_Read( bf_read *pBuf, const char *pWantedTag )
{
	if ( net_synctags.GetInt() )
	{
		char testTag[512];
		pBuf->ReadString( testTag, sizeof( testTag ) );
		
		if ( stricmp( testTag, pWantedTag ) != 0 )
		{
			Error( "SyncTag_Read: out-of-sync at tag %s", pWantedTag );
		}
	}
}

