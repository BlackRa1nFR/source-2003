//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "tier0/platform.h"
#include "tier0/vcrmode.h"
#include "tier0/memalloc.h"
#include "tier0/dbg.h"

#include <sys/time.h>
#include <unistd.h>

extern VCRMode g_VCRMode;

double Plat_FloatTime()
{
        struct timeval  tp;
        static int      secbase = 0;

        gettimeofday( &tp, NULL );

        if ( !secbase )
        {
                secbase = tp.tv_sec;
                return ( tp.tv_usec / 1000000.0 );
        }

	if (g_VCRMode == VCR_Disabled)
		return (( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );
	
	return g_pVCR->Hook_Sys_FloatTime( ( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );
}

unsigned long Plat_MSTime()
{
        struct timeval  tp;
        static int      secbase = 0;

        gettimeofday( &tp, NULL );

        if ( !secbase )
        {
                secbase = tp.tv_sec;
                return ( tp.tv_usec / 1000000.0 );
        }

	return (unsigned long)(( tp.tv_sec - secbase ) + tp.tv_usec / 1000000.0 );

}



bool vtune( bool resume )
{
}


// -------------------------------------------------------------------------------------------------- //
// Memory stuff.
// -------------------------------------------------------------------------------------------------- //

void Plat_DefaultAllocErrorFn( unsigned long size )
{
}

Plat_AllocErrorFn g_AllocError = Plat_DefaultAllocErrorFn;

void* Plat_Alloc( unsigned long size )
{
	void *pRet = g_pMemAlloc->Alloc( size );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
		g_AllocError( size );
		return 0;
	}
}


void* Plat_Realloc( void *ptr, unsigned long size )
{
	void *pRet = g_pMemAlloc->Realloc( ptr, size );
	if ( pRet )
	{
		return pRet;
	}
	else
	{
		g_AllocError( size );
		return 0;
	}
}


void Plat_Free( void *ptr )
{
	g_pMemAlloc->Free( ptr );
}


void Plat_SetAllocErrorFn( Plat_AllocErrorFn fn )
{
	g_AllocError = fn;
}


void Plat_SetThreadName( unsigned long dwThreadID, const char *pName )
{
	Assert( "Plat_SetThreadName not implemented");
}



