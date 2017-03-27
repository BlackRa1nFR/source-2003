//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "bitbuf.h"
#include "bitbuf_errorhandler.h"
#include "tier0/dbg.h"


void EngineBitBufErrorHandler( BitBufErrorType errorType, const char *pDebugName )
{
	if ( !pDebugName )
	{
		pDebugName = "(unknown)";
	}

	static int printCount[BITBUFERROR_NUM_ERRORS];
	static bool bInitted = false;
	if ( !bInitted )
	{
		for ( int i=0; i < BITBUFERROR_NUM_ERRORS; i++ )
			printCount[i] = 0;

		bInitted = true;
	}

	// Only print an error a couple times..
	if ( printCount[errorType] < 3 )
	{
		if ( errorType == BITBUFERROR_VALUE_OUT_OF_RANGE )
		{
			Warning( "Error in bitbuf [%s]: out of range value. Debug in bitbuf_errorhandler.cpp\n", pDebugName );
		}
		else if ( errorType == BITBUFERROR_BUFFER_OVERRUN )
		{
			Warning( "Error in bitbuf [%s]: buffer overrun. Debug in bitbuf_errorhandler.cpp\n", pDebugName );
		}

		++printCount[errorType];
	}
}


void InstallBitBufErrorHandler()
{
	SetBitBufErrorHandler( EngineBitBufErrorHandler );
}






