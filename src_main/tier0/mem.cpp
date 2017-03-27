//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Memory allocation!
//
// $NoKeywords: $
//=============================================================================

#include "tier0/mem.h"
#include <malloc.h>
#include "tier0/dbg.h"


enum 
{
	MAX_STACK_DEPTH = 32
};

static unsigned char *s_pBuf = NULL;
static int s_pBufStackDepth[MAX_STACK_DEPTH];
static int s_nBufDepth = -1;
static int s_nBufCurSize = 0;
static int s_nBufAllocSize = 0;


//-----------------------------------------------------------------------------
// Other DLL-exported methods for particular kinds of memory
//-----------------------------------------------------------------------------
void *MemAllocScratch( int nMemSize )
{	
	// Minimally allocate 1M scratch
	if (s_nBufAllocSize < s_nBufCurSize + nMemSize)
	{
		s_nBufAllocSize = s_nBufCurSize + nMemSize;
		if (s_nBufAllocSize < 1024 * 1024)
		{
			s_nBufAllocSize = 1024 * 1024;
		}

		if (s_pBuf)
		{
#ifdef _WIN32
			if (s_nBufDepth < 0)
			{
				s_pBuf = (unsigned char*)realloc( s_pBuf, s_nBufAllocSize );
			}
			else
			{
				s_pBuf = (unsigned char*)_expand( s_pBuf, s_nBufAllocSize );
				Assert( s_pBuf );
			}
#elif _LINUX
			s_pBuf = (unsigned char*)realloc( s_pBuf, s_nBufAllocSize );
			Assert( s_pBuf );	
#endif
		}
		else
		{
			s_pBuf = (unsigned char*)malloc( s_nBufAllocSize );
		}
	}

	int nBase = s_nBufCurSize;
	s_nBufCurSize += nMemSize;
	++s_nBufDepth;
	Assert( s_nBufDepth < MAX_STACK_DEPTH );
	s_pBufStackDepth[s_nBufDepth] = nMemSize;

	return &s_pBuf[nBase];
}

void MemFreeScratch()
{
	Assert( s_nBufDepth >= 0 );
	s_nBufCurSize -= s_pBufStackDepth[s_nBufDepth];
	--s_nBufDepth;
}


#ifdef _LINUX
void ZeroMemory( void *mem, size_t length )
{
	memset( mem, 0x0, length );
}
#endif
