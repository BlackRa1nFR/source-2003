//=========== (C) Copyright 2000 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================

#include "mempool.h"

#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include "tier0/dbg.h"
#include <ctype.h>
#include "tier0/memdbgon.h"

#undef max
#define max(x,y) (((x)>(y)) ? (x) : (y))

MemoryPoolReportFunc_t CMemoryPool::g_ReportFunc = 0;

//-----------------------------------------------------------------------------
// Error reporting...  (debug only)
//-----------------------------------------------------------------------------

void CMemoryPool::SetErrorReportFunc( MemoryPoolReportFunc_t func )
{
	g_ReportFunc = func;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

CMemoryPool::CMemoryPool(int blockSize, int numElements, int growMode)
{
	m_BlockSize = blockSize < sizeof(void*) ? sizeof(void*) : blockSize;
	m_BlocksPerBlob = numElements;
	m_PeakAlloc = 0;
	m_GrowMode = growMode;
	Init();
	AddNewBlob();
}

//-----------------------------------------------------------------------------
// Purpose: Frees the memory contained in the mempool, and invalidates it for
//			any further use.
// Input  : *memPool - the mempool to shutdown
//-----------------------------------------------------------------------------
CMemoryPool::~CMemoryPool()
{
	if (m_BlocksAllocated > 0)
	{
		ReportLeaks();
	}
	Clear();
}


//-----------------------------------------------------------------------------
// Resets the pool
//-----------------------------------------------------------------------------
void CMemoryPool::Init()
{
	m_NumBlobs = 0;
	m_BlocksAllocated = 0;
	m_pHeadOfFreeList = 0;
	m_BlobHead.m_pNext = m_BlobHead.m_pPrev = &m_BlobHead;
}


//-----------------------------------------------------------------------------
// Frees everything
//-----------------------------------------------------------------------------
void CMemoryPool::Clear()
{
	// Free everything..
	CBlob *pNext;
	for( CBlob *pCur = m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur = pNext )
	{
		pNext = pCur->m_pNext;
		free( pCur );
	}
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Reports memory leaks 
//-----------------------------------------------------------------------------

void CMemoryPool::ReportLeaks()
{
	if (!g_ReportFunc)
		return;

	g_ReportFunc("Memory leak: mempool blocks left in memory: %d\n", m_BlocksAllocated);

#ifdef _DEBUG
	// walk and destroy the free list so it doesn't intefere in the scan
	while (m_pHeadOfFreeList != NULL)
	{
		void *next = *((void**)m_pHeadOfFreeList);
		memset(m_pHeadOfFreeList, 0, m_BlockSize);
		m_pHeadOfFreeList = next;
	}

	g_ReportFunc("Dumping memory: \'");

	for( CBlob *pCur=m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur=pCur->m_pNext )
	{
		// scan the memory block and dump the leaks
		char *scanPoint = (char *)pCur->m_Data;
		char *scanEnd = pCur->m_Data + pCur->m_NumBytes;
		bool needSpace = false;

		while (scanPoint < scanEnd)
		{
			// search for and dump any strings
			if (isprint(*scanPoint))
			{
				g_ReportFunc("%c", *scanPoint);
				needSpace = true;
			}
			else if (needSpace)
			{
				needSpace = false;
				g_ReportFunc(" ");
			}

			scanPoint++;
		}
	}

	g_ReportFunc("\'\n");
#endif // _DEBUG
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMemoryPool::AddNewBlob()
{
	int sizeMultiplier;

	if( m_GrowMode == GROW_SLOW )
	{
		sizeMultiplier = 1;
	}
	else if( m_GrowMode == GROW_NONE )
	{
		// Can only have one allocation when we're in this mode
		if( m_NumBlobs != 0 )
		{
			Assert( !"CMemoryPool::AddNewBlob: mode == GROW_NONE" );
			return;
		}
	}
	
	// GROW_FAST and GROW_NONE use this.
	sizeMultiplier = m_NumBlobs + 1;

	// maybe use something other than malloc?
	int nElements = m_BlocksPerBlob * sizeMultiplier;
	int blobSize = m_BlockSize * nElements;
	CBlob *pBlob = (CBlob*)malloc( sizeof(CBlob) + blobSize - 1 );
	Assert( pBlob );
	
	// Link it in at the end of the blob list.
	pBlob->m_NumBytes = blobSize;
	pBlob->m_pNext = &m_BlobHead;
	pBlob->m_pPrev = pBlob->m_pNext->m_pPrev;
	pBlob->m_pNext->m_pPrev = pBlob->m_pPrev->m_pNext = pBlob;

	// setup the free list
	m_pHeadOfFreeList = pBlob->m_Data;
	Assert (m_pHeadOfFreeList);

	void **newBlob = (void**)m_pHeadOfFreeList;
	for (int j = 0; j < nElements-1; j++)
	{
		newBlob[0] = (char*)newBlob + m_BlockSize;
		newBlob = (void**)newBlob[0];
	}

	// null terminate list
	newBlob[0] = NULL;
	m_NumBlobs++;
}


void* CMemoryPool::Alloc()
{
	return Alloc( m_BlockSize );
}


//-----------------------------------------------------------------------------
// Purpose: Allocs a single block of memory from the pool.  
// Input  : amount - 
//-----------------------------------------------------------------------------
void *CMemoryPool::Alloc( unsigned int amount )
{
	void *returnBlock;

	if ( amount > (unsigned int)m_BlockSize )
		return NULL;

	if( !m_pHeadOfFreeList )
	{
		// returning NULL is fine in GROW_NONE
		if( m_GrowMode == GROW_NONE )
		{
			//Assert( !"CMemoryPool::Alloc: tried to make new blob with GROW_NONE" );
			return NULL;
		}

		// overflow
		AddNewBlob();

		// still failure, error out
		if( !m_pHeadOfFreeList )
		{
			Assert( !"CMemoryPool::Alloc: ran out of memory" );
			return NULL;
		}
	}
	m_BlocksAllocated++;
	m_PeakAlloc = max(m_PeakAlloc, m_BlocksAllocated);

	returnBlock = m_pHeadOfFreeList;

	// move the pointer the next block
	m_pHeadOfFreeList = *((void**)m_pHeadOfFreeList);

	return returnBlock;
}

//-----------------------------------------------------------------------------
// Purpose: Frees a block of memory
// Input  : *memBlock - the memory to free
//-----------------------------------------------------------------------------
void CMemoryPool::Free( void *memBlock )
{
	if ( !memBlock )
		return;  // trying to delete NULL pointer, ignore

#ifdef _DEBUG
	// check to see if the memory is from the allocated range
	bool bOK = false;
	for( CBlob *pCur=m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur=pCur->m_pNext )
	{
		if (memBlock >= pCur->m_Data && (char*)memBlock < (pCur->m_Data + pCur->m_NumBytes))
		{
			bOK = true;
		}
	}
	Assert (bOK);
#endif // _DEBUG

#ifdef _DEBUG	
	// invalidate the memory
	memset( memBlock, 0xDD, m_BlockSize );
#endif

	m_BlocksAllocated--;

	// make the block point to the first item in the list
	*((void**)memBlock) = m_pHeadOfFreeList;

	// the list head is now the new block
	m_pHeadOfFreeList = memBlock;
}



