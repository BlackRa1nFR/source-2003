//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================
#include <string.h>
#include "characterset.h"

//-----------------------------------------------------------------------------
// Purpose: builds a simple lookup table of a group of important characters
// Input  : *pParseGroup - pointer to the buffer for the group
//			*pGroupString - null terminated list of characters to flag
//-----------------------------------------------------------------------------
void CharacterSetBuild( characterset_t *pSetBuffer, const char *pszSetString )
{
	int i = 0;

	// Test our pointers
	if ( !pSetBuffer || !pszSetString )
		return;

	memset( pSetBuffer->set, 0, sizeof(pSetBuffer->set) );

	while ( pszSetString[i] )
	{
		pSetBuffer->set[ pszSetString[i] ] = 1;
		i++;
	}

}
