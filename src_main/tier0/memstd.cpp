//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Memory allocation!
//
// $NoKeywords: $
//=============================================================================

#ifndef _DEBUG

#include <malloc.h>
#include "tier0/dbg.h"
#include "tier0/memalloc.h"


//-----------------------------------------------------------------------------
// NOTE! This should never be called directly from leaf code
// Just use new,delete,malloc,free etc. They will call into this eventually
//-----------------------------------------------------------------------------
class CStdMemAlloc : public IMemAlloc
{
public:
	// Release versions
	virtual void *Alloc( size_t nSize );
	virtual void *Realloc( void *pMem, size_t nSize );
	virtual void  Free( void *pMem );
    virtual void *Expand( void *pMem, size_t nSize );

	// Debug versions
    virtual void *Alloc( size_t nSize, const char *pFileName, int nLine );
    virtual void *Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine );
    virtual void  Free( void *pMem, const char *pFileName, int nLine );
    virtual void *Expand( void *pMem, size_t nSize, const char *pFileName, int nLine );

	// Returns size of a particular allocation
	virtual size_t GetSize( void *pMem );

    // Force file + line information for an allocation
    virtual void PushAllocDbgInfo( const char *pFileName, int nLine );
    virtual void PopAllocDbgInfo();

	virtual long CrtSetBreakAlloc( long lNewBreakAlloc );
	virtual	int CrtSetReportMode( int nReportType, int nReportMode );
	virtual int CrtIsValidHeapPointer( const void *pMem );
	virtual int CrtCheckMemory( void );
	virtual int CrtSetDbgFlag( int nNewFlag );
	virtual void CrtMemCheckpoint( _CrtMemState *pState );
	void* CrtSetReportFile( int nRptType, void* hFile );
	void* CrtSetReportHook( void* pfnNewHook );
	int CrtDbgReport( int nRptType, const char * szFile,
			int nLine, const char * szModule, const char * pMsg );
	virtual int heapchk();

	virtual void DumpStats() {}

private:
};


//-----------------------------------------------------------------------------
// Singleton...
//-----------------------------------------------------------------------------
static CStdMemAlloc s_StdMemAlloc;

#ifndef TIER0_VALIDATE_HEAP
IMemAlloc *g_pMemAlloc = &s_StdMemAlloc;
#else
IMemAlloc *g_pActualAlloc = &s_StdMemAlloc;
#endif


//-----------------------------------------------------------------------------
// Release versions
//-----------------------------------------------------------------------------
void *CStdMemAlloc::Alloc( size_t nSize )
{
	return malloc( nSize );
}

void *CStdMemAlloc::Realloc( void *pMem, size_t nSize )
{
	return realloc( pMem, nSize );
}

void CStdMemAlloc::Free( void *pMem )
{
	free( pMem );
}

void *CStdMemAlloc::Expand( void *pMem, size_t nSize )
{
#ifdef _WIN32
	return _expand( pMem, nSize );
#elif _LINUX
	return realloc( pMem, nSize );
#endif
}


//-----------------------------------------------------------------------------
// Debug versions
//-----------------------------------------------------------------------------
void *CStdMemAlloc::Alloc( size_t nSize, const char *pFileName, int nLine )
{
	return malloc( nSize );
}

void *CStdMemAlloc::Realloc( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
	return realloc( pMem, nSize );
}

void  CStdMemAlloc::Free( void *pMem, const char *pFileName, int nLine )
{
	free( pMem );
}

void *CStdMemAlloc::Expand( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
#ifdef _WIN32
	return _expand( pMem, nSize );
#elif _LINUX
	return realloc( pMem, nSize );
#endif
}


//-----------------------------------------------------------------------------
// Returns size of a particular allocation
//-----------------------------------------------------------------------------
size_t CStdMemAlloc::GetSize( void *pMem )
{
#ifdef _WIN32
	return _msize( pMem );
#elif _LINUX
	Assert( "GetSize() not implemented");
#endif
}


//-----------------------------------------------------------------------------
// Force file + line information for an allocation
//-----------------------------------------------------------------------------
void CStdMemAlloc::PushAllocDbgInfo( const char *pFileName, int nLine )
{
}

void CStdMemAlloc::PopAllocDbgInfo()
{
}

//-----------------------------------------------------------------------------
// FIXME: Remove when we make our own heap! Crt stuff we're currently using
//-----------------------------------------------------------------------------
long CStdMemAlloc::CrtSetBreakAlloc( long lNewBreakAlloc )
{
	return 0;
}

int CStdMemAlloc::CrtSetReportMode( int nReportType, int nReportMode )
{
	return 0;
}

int CStdMemAlloc::CrtIsValidHeapPointer( const void *pMem )
{
	return 1;
}

int CStdMemAlloc::CrtCheckMemory( void )
{
	return 1;
}

int CStdMemAlloc::CrtSetDbgFlag( int nNewFlag )
{
	return 0;
}

void CStdMemAlloc::CrtMemCheckpoint( _CrtMemState *pState )
{
}

// FIXME: Remove when we have our own allocator
void* CStdMemAlloc::CrtSetReportFile( int nRptType, void* hFile )
{
	return 0;
}

void* CStdMemAlloc::CrtSetReportHook( void* pfnNewHook )
{
	return 0;
}

int CStdMemAlloc::CrtDbgReport( int nRptType, const char * szFile,
		int nLine, const char * szModule, const char * pMsg )
{
	return 0;
}

int CStdMemAlloc::heapchk()
{
#ifdef _WIN32
	return _HEAPOK;
#elif _LINUX
	return 1;
#endif
}


#endif
